#include "Includes.hxx"

inja::json ReflInfo;
tree ReflectorClass = NULL_TREE;

// ============
subhook::Hook Hook_GC_Collect_cp_parser;
void get_the_parser_helper(void* _parser) {
    subhook::ScopedHookRemove _remove(&Hook_GC_Collect_cp_parser);
    the_parser = (cp_parser*) _parser;
    gt_ggc_mx_cp_parser(_parser);
}
void get_the_parser() {
    assert(Hook_GC_Collect_cp_parser.Install(
        (void*) gt_ggc_mx_cp_parser,
        (void*) get_the_parser_helper,
        subhook::HookFlag64BitOffset)
    );
    ggc_collect(GGC_COLLECT_FORCE);
    Hook_GC_Collect_cp_parser.Remove();
}
bool first_time = true;
// ============

void traverse (tree ns, auto fn) { // from CodeSynthesis
    tree decl;
    cp_binding_level* level (NAMESPACE_LEVEL (ns));
    for (decl = level->names;
         decl != 0;
         decl = TREE_CHAIN (decl))
    {
        if (DECL_IS_UNDECLARED_BUILTIN(decl) or (
            DECL_BUILT_IN_CLASS (decl) == BUILT_IN_NORMAL or
            DECL_BUILT_IN_CLASS (decl) == BUILT_IN_FRONTEND or
            DECL_BUILT_IN_CLASS (decl) == BUILT_IN_MD
        ))
            continue;

        fn (decl);
    }/*
    for(decl = level->
        decl != 0;
        decl = TREE_CHAIN (decl))
    {
        if (DECL_IS_BUILTIN (decl))
            continue;

        print_decl (decl);
        traverse (decl);
    }*/
}

enum class AccessModifier : uint8_t {
    Private, Protected, Public
};
AccessModifier get_access_modifier(tree t) {
    return TREE_PRIVATE(t)   ? AccessModifier::Private :
           TREE_PROTECTED(t) ? AccessModifier::Protected :
                               AccessModifier::Public;
}

tree patched_finish_struct (tree t, tree attributes) {
    subhook::ScopedHookRemove _remove(&TheHook);
#ifdef DEBUG
    printf("[* patched_finish_struct Called! *]\n");
#endif
    if (first_time) {
        get_the_parser();
        first_time = false;
    }

    auto Rslt = finish_struct(t, attributes);
    //printf("Full name: %s\n", type_as_string(Rslt, TFF_SCOPE));
    // Get rid of those types that we don't need to reflect
    if (c_str_prefix("std", type_as_string(Rslt, TFF_SCOPE))
            // ignore std since there are way too many classes and will make the program bloated
            // if we store all the reflection information
    or  c_str_prefix("timex", type_as_string(Rslt, TFF_SCOPE))
    or  c_str_prefix("_", type_as_string(Rslt, TFF_SCOPE))
            // ignore all those builtin stuff
    or  type_as_string(Rslt, TFF_UNQUALIFIED_NAME)[0] == '$'
            // ignore the classes created by ourselves
    or  c_str_prefix("._anon", IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(Rslt))))
    or  c_str_prefix("<unnamed", type_as_string(Rslt, TFF_SCOPE))
            // ignore anonymous stuff
    )
        return Rslt;

    std::string full_name = type_as_string(Rslt, TFF_SCOPE);
    std::string normalized_name = full_name;
    replace_all(normalized_name, "::", "$");
    auto& $this = ReflInfo["Types"][full_name];
    $this["FullName"] = full_name;
    $this["NormalizedName"] = normalized_name;

#ifdef DEBUG
    printf("\n\n");
    printf("Type name: %s\n", IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(Rslt))));
    printf("Full name: %s\n", type_as_string(Rslt, TFF_SCOPE)); // struct or class? use TFF_CLASS_KEY_OR_ENUM
    if (TREE_CODE(TYPE_CONTEXT(Rslt)) != TRANSLATION_UNIT_DECL) {
        // nested class
        printf("Nested in:\n");
        tree _cls = TYPE_CONTEXT(Rslt); int i = 0;
        for (;;) {
            if (_cls == NULL_TREE or _cls == error_mark_node)
                break;
            if (TREE_CODE(_cls) == TRANSLATION_UNIT_DECL)
                break;
            printf("    (%d): %s\n", i, type_as_string(_cls, TFF_CLASS_KEY_OR_ENUM)); ++i;
            _cls = TYPE_CONTEXT(_cls);
        }
    }
#endif

    // TODO: Inheritance check
    
    if (TREE_CODE(TYPE_FIELDS(Rslt)) != TYPE_DECL) {
#ifdef DEBUG
        printf("Members:\n");
#endif
        tree field = TYPE_FIELDS(Rslt); int i = 0;
        for (;;) {
            if (TREE_CODE(field) == TYPE_DECL)
                break;
#ifdef DEBUG
            printf("    (%d): %s (access modifier \"%s\", type \"%s\", static \"%s\")\n",
                i,
                decl_as_string(field, TFF_SCOPE), TREE_PRIVATE(field) ? "private" : (TREE_PROTECTED(field) ? "protected" : "public"),
                type_as_string(TREE_TYPE(field), TFF_SCOPE),
                TREE_STATIC(field) ? "true" : "false"
            );
#endif
            std::string field_name = decl_as_string(field, TFF_UNQUALIFIED_NAME);
            if (TREE_CODE(field) == FUNCTION_DECL) {
                field_name = field_name.substr(0, field_name.find("("));
            }
            auto& $field = $this["Members"][field_name];
            bool is_field = TREE_CODE(field) == VAR_DECL or TREE_CODE(field) == FIELD_DECL;
            bool is_method = TREE_CODE(field) == FUNCTION_DECL;
            $field["Name"] = field_name;
            $field["IsField"] = is_field;
                // static -> VAR_DECL
                // non-static -> FIELD_DECL
            $field["IsMethod"] = is_method;
            $field["IsStatic"] = (bool) TREE_STATIC(field);
            if (not TREE_STATIC(field) and not TREE_CODE(field) == FUNCTION_DECL)
                $field["Offset"] = wi::to_wide(byte_position(field)).to_shwi();
                    // offset is unsigned, so we can use wi::to_wide without adding additional code
            else
                $field["Offset"] = 0;
            // TODO: private/protected static and function handling
            $field["IsPublic"] = get_access_modifier(field) == AccessModifier::Public;
            if (is_field)
                $field["MemberType"] = "Field";
            if (is_method)
                $field["MemberType"] = "Method";
            // TODO: add ctor, dtor detection
            field = TREE_CHAIN(field); ++i;
        }
    }

    return Rslt;
}
const char* patched_cpp_read_main_file(cpp_reader *pfile, const char *fname, bool injecting) {
    subhook::ScopedHookRemove _remove(&HookToCppReadMainFile);
#ifdef DEBUG
    printf("[* patched_cpp_read_main_file Called! *]\n");
#endif
    auto Ret = cpp_read_main_file(pfile, fname, injecting);
    cpp_push_default_include(pfile, "/home/nullptr07/Xfw/Src/Boilerplate.cpp");
    return Ret;
}

void handle_Xfw_TUnitEnd(cpp_reader*) {
    /*debug(global_namespace);
    traverse(global_namespace, [] (tree t1) {
        std::string name;
        if (DECL_P(t1))
            name = decl_as_string(t1, TFF_SCOPE);
        else if (TYPE_P(t1))
            name = type_as_string(t1, TFF_SCOPE);
        std::cout << name << std::endl;
    });*/
    std::string code = inja::render(R"code(
struct $Reflector {
    {% for _1, type in Types %}
    static $Type $Get_{{ type.NormalizedName }}_type() {
        static $Type ${{ type.NormalizedName }}_type = $Type {
            .$Members = {
                {% if existsIn(type, "Members") %}
                    {% for _2, member in type.Members %}
                    {
                        "{{ member.Name }}", $MemberInfo (
                            $CONSTANT_VALUE($MemberTypes::${{ member.MemberType }}),
                        {% if member.MemberType == "Field" %}
                            "{{ member.Name }}",
                            [] (auto obj /* in std::any */) {
                            {% if member.IsStatic %}
                                return {{ type.FullName }}::{{ member.Name }};
                            {% else %}
                                return std::any_cast<{{ type.FullName }}>(obj).{{ member.Name }};
                            {% endif %}
                            }
                        {% else if member.MemberType == "Method" %}
                            "{{ member.Name }}",
                            [] (auto _obj /* in std::any */, auto _args /* in std::vector<std::any> */) {
                            {% if member.IsStatic %}
                                // assert(_args.size() ==
                                return $Dyncall(&{{ type.FullName }}::{{ member.Name }},;
                            {% else %}
                                return (void*)0;//&(reinterpret_cast<{{ type.FullName }}*>(_obj)->{{ member.Name }});
                            {% endif %}
                            }
                        {% endif %}
                        )
                    },
                    {% endfor %}
                {% endif %}
            }
        };
        return ${{ type.NormalizedName }}_type;
    }
    {% endfor %}
};

$Type $TypeOf(auto Val) {
    using TVal = std::decay_t<decltype(Val)>;
{% for _1, type in Types %}
    if constexpr (std::is_same_v<TVal, {{ type.FullName }}>) {
        return $Reflector::$Get_{{ type.NormalizedName }}_type();
    }
{% endfor %}
}
)code", ReflInfo);
    //std::cout << code;
#ifdef DEBUG
    std::cout << code;
#endif
    write_to_file("/tmp/tmpcode1.cpp", "%s", code.c_str());

#define CPP_OPTION(PFILE, OPTION) ((PFILE)->opts.OPTION)

    cpp_reader* parser = cpp_create_reader (CLK_CXX23, ident_hash, line_table);
    CPP_OPTION(parser, warn_dollars) = false;
    parser->bracket_include = parse_in->bracket_include;
    parser->cb.diagnostic = parse_in->cb.diagnostic;
    subhook::ScopedHookRemove _remove(&HookToCppReadMainFile);
    cpp_read_main_file (parser, "/tmp/tmpcode1.cpp", false);

    cpp_reader* old_parse_in = parse_in; parse_in = parser;
    cp_lexer* lexer = cp_lexer_alloc ();
    cp_token token{};
    cp_token eof = the_parser->lexer->buffer->pop();
    do {
        cp_lexer_get_preprocessor_token (C_LEX_STRING_NO_JOIN, &token);
        vec_safe_push(the_parser->lexer->buffer, token);
    } while (token.type != CPP_EOF);
    vec_safe_push(the_parser->lexer->buffer, eof);
    lexer->last_token = reinterpret_cast<cp_token_position>(lexer->buffer->length() - 1);

    parse_in = old_parse_in;
    cp_lexer_destroy(lexer);
    cpp_finish(parser, nullptr);
    cpp_destroy(parser);
    remove("/tmp/tmpcode1.cpp");
    TheHook.Remove();
}

/*void patched_set_global_friend(tree scope) {
    subhook::ScopedHookRemove _remove(&SetGlobalFriendHook);
    debug_tree (scope);
    set_global_friend(scope);
}
 */

void finish_type_callback(void* event_data, void* user_data) {
    tree t = (tree) event_data;

    if (t == NULL or TREE_CODE(t) == ERROR_MARK)
        return;

    if (c_str_prefix("std", type_as_string(t, TFF_SCOPE))
        // ignore std since there are way too many classes and will make the program bloated
        // if we store all the reflection information
        or c_str_prefix("timex", type_as_string(t, TFF_SCOPE))
        or c_str_prefix("_", type_as_string(t, TFF_SCOPE))
        // ignore all those builtin stuff
        // ignore the classes created by ourselves
        or c_str_prefix("._anon", IDENTIFIER_POINTER(DECL_NAME(TYPE_NAME(t))))
        or c_str_prefix("<unnamed", type_as_string(t, TFF_SCOPE))
        // ignore anonymous stuff
            )
        return;
    else {
        if (ReflectorClass == NULL_TREE) {
            traverse(global_namespace, [](tree t1) {
                if (std::string(decl_as_string(t1, TFF_SCOPE)) == "$Reflector") {
                    ReflectorClass = TREE_TYPE(t1);
                }
            });
        } else {
            if (TREE_CODE(t) == RECORD_TYPE) {
                make_friend_class(t, ReflectorClass, false);
            }
        }
    }
}

// use register_callback (plugin_name, PLUGIN_FINISH_DECL, plugin_finish_decl, NULL);
[[maybe_unused]] int plugin_init(plugin_name_args* plugin_info, [[maybe_unused]] plugin_gcc_version* version) {
	std::cout << "X-Framework GCC plugin loaded" << std::endl;
    remove("/tmp/tmpcode.cpp");
    remove("/tmp/tmpcode1.cpp");

    assert(TheHook.Install(
        (void*) finish_struct,
        (void*) patched_finish_struct,
        subhook::HookFlags::HookFlag64BitOffset
    ));
    assert(HookToCppReadMainFile.Install(
        (void*) cpp_read_main_file,
        (void*) patched_cpp_read_main_file,
        subhook::HookFlags::HookFlag64BitOffset
    ));
    /*assert(SetGlobalFriendHook.Install(
        (void*) set_global_friend,
        (void*) patched_set_global_friend,
        subhook::HookFlags::HookFlag64BitOffset
    ));*/
    register_callback(plugin_info->full_name, PLUGIN_FINISH, [] (void*, void*) {
        remove("/tmp/tmpcode.cpp");
        remove("/tmp/tmpcode1.cpp");
    }, nullptr);
    register_callback(plugin_info->full_name, PLUGIN_PRAGMAS, [] (void*, void*) {
        c_register_pragma(nullptr, "Xfw_TUnitEnd", handle_Xfw_TUnitEnd);
    }, nullptr);
    register_callback(plugin_info->full_name, PLUGIN_FINISH_TYPE, finish_type_callback, nullptr);

    return 0;
}

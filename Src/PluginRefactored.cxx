#include "Includes.hxx"
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

// ============
enum class AccessModifier : uint8_t {
    Private, Protected, Public
};
AccessModifier get_access_modifier(tree t) {
    return TREE_PRIVATE(t)   ? AccessModifier::Private :
           TREE_PROTECTED(t) ? AccessModifier::Protected :
           AccessModifier::Public;
}
struct ReflInfoTy {
    std::string Name;
    bool IsField, IsMethod;
    bool IsStatic;
    bool IsPublic;
    int32_t Offset;
};
std::vector<ReflInfoTy> ReflInfo;
// ============

tree patched_finish_struct (tree t, tree attributes) {
    subhook::ScopedHookRemove _remove(&TheHook);
#ifdef DEBUG
    printf("[* patched_finish_struct Called! *]\n");
#endif
    // If this is the first time we get called, we want to get the variable the_parser
    if (first_time) { get_the_parser(); first_time = false; }

    auto Rslt = finish_struct(t, attributes);
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

    // full name is literally the full name of struct including the scope (like "MyNamespace::MyStruct")
    std::string full_name = type_as_string(Rslt, TFF_SCOPE);
    // however, if we want to declare a variable that contains the reflection info and have the name of the struct
    // we need to normalize the name so that it won't contain ::s (like "MyNamespace$$MyStruct")
    std::string normalized_name = full_name;
    replace_all(normalized_name, "::", "$$");

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

    if (TREE_CODE(TYPE_FIELDS(Rslt)) != TYPE_DECL) { // if this struct is NOT empty...
#ifdef DEBUG
        printf("Members:\n");
#endif
        tree field = TYPE_FIELDS(Rslt); int i = 0;
        for (;;) { // then we want to iterate over its members
            if (TREE_CODE(field) == TYPE_DECL) // if we reached the end, then exit the loop
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
            $field["Name"] = field_name;
            $field["IsField"] = TREE_CODE(field) == VAR_DECL or TREE_CODE(field) == FIELD_DECL;
            // static -> VAR_DECL
            // non-static -> FIELD_DECL
            $field["IsMethod"] = TREE_CODE(field) == FUNCTION_DECL;
            $field["IsStatic"] = (bool) TREE_STATIC(field);
            if (not TREE_STATIC(field) and not TREE_CODE(field) == FUNCTION_DECL)
                $field["Offset"] = wi::to_wide(byte_position(field)).to_shwi();
                // offset is unsigned, so we can use wi::to_wide without adding additional code
            else
                $field["Offset"] = 0;
            // TODO: private/protected static and function handling
            $field["IsPublic"] = get_access_modifier(field) == AccessModifier::Public;
            field = TREE_CHAIN(field); ++i;
        }





        tree member = TYPE_FIELDS(Rslt); int i = 0;
        for (;;) { // then we want to iterate over its members
            if (TREE_CODE(member) == TYPE_DECL) // if we reached the end, then exit the loop
                break;
#ifdef DEBUG
            printf("    (%d): %s (access modifier \"%s\", type \"%s\", static \"%s\")\n",
                i,
                decl_as_string(field, TFF_SCOPE), TREE_PRIVATE(field) ? "private" : (TREE_PROTECTED(field) ? "protected" : "public"),
                type_as_string(TREE_TYPE(field), TFF_SCOPE),
                TREE_STATIC(field) ? "true" : "false"
            );
#endif
            // this is the unqualified member name (like "MyField")
            std::string member_name = decl_as_string(field, TFF_UNQUALIFIED_NAME);
            if (TREE_CODE(member) == FUNCTION_DECL) { // if this is a member function declaration...
                // then we need to remove the stuff after '(' to obtain real name
                member_name = member_name.substr(0, member_name.find('('));
            }
            ReflInfo.push_back(ReflInfoTy {
                .Name = member_name,
                // static -> VAR_DECL
                // non-static -> FIELD_DECL
                .IsField = TREE_CODE(field) == VAR_DECL or TREE_CODE(field) == FIELD_DECL,
                .IsMethod = TREE_CODE(field) == FUNCTION_DECL
            });
            auto& $field = $this["Members"][field_name];
            $field["Name"] = field_name;
            $field["IsField"] = TREE_CODE(field) == VAR_DECL or TREE_CODE(field) == FIELD_DECL;
            // static -> VAR_DECL
            // non-static -> FIELD_DECL
            $field["IsMethod"] = TREE_CODE(field) == FUNCTION_DECL;
            $field["IsStatic"] = (bool) TREE_STATIC(field);
            if (not TREE_STATIC(field) and not TREE_CODE(field) == FUNCTION_DECL)
                $field["Offset"] = wi::to_wide(byte_position(field)).to_shwi();
                // offset is unsigned, so we can use wi::to_wide without adding additional code
            else
                $field["Offset"] = 0;
            // TODO: private/protected static and function handling
            $field["IsPublic"] = get_access_modifier(field) == AccessModifier::Public;
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
    write_to_file("/tmp/tmpcode.cpp", "%s", R"code(
#include <functional>
#include <any>
#include <vector>
#include <map>
#include <utility>
#include <string>
#include <cstddef>
#define $FWD(x) std::forward<decltype(x)>(x)
#define $CONSTANT(...) \
  union { static constexpr auto value() { return __VA_ARGS__; } }
#define $CONSTANT_VALUE(...) \
  [] { using R = $CONSTANT(__VA_ARGS__); return R{}; }()
enum class $MemberTypes : uint8_t {
    $Field = 0,
    $Method = 1,
    $Ctor = 2,
    $Dtor = 3
};
struct $Dynamic {

};
struct $MemberInfo {
private:
    void _ConstructField(
        std::string _Name,
        std::function<void*(void*)> _GetValue
    ) {
        $Name = _Name;
        $MemberType = $MemberTypes::$Field;
        $GetValue = _GetValue;
        $Invoke = [] (auto) -> void* {
            throw "Calling $Invoke w/ a $MemberInfo that is holding a $Field!";
        };
    }
    void _ConstructMethod(
        std::string _Name,
        std::function<void*(void*, std::vector<void*>)> _Invoke
    ) {
        $Name = _Name;
        $MemberType = $MemberTypes::$Method;
        $GetValue = [] (auto) -> void* {
            throw "Calling $GetValue w/ a $MemberInfo that is holding a $Method!";
        };
        $Invoke = _Invoke;
    }
public:
    std::string $Name;
    $MemberTypes $MemberType;
    std::function<void*(void*)> $GetValue;
    std::function<void*(void*, std::vector<void*>)> $Invoke;
    int8_t $NumArgs;

    template <typename $>
    constexpr $MemberInfo($, auto&&... _Args) {
        if constexpr ($::value() == $MemberTypes::$Field) {
            _ConstructField($FWD(_Args)...);
        } else if constexpr ($::value() == $MemberTypes::$Method) {
            _ConstructMethod($FWD(_Args)...);
        } else {
            static_assert("Unknown $MemberTypes");
        }
    }

};
struct $Type {
    std::map<std::string, $MemberInfo> $Members;
};
$Type $TypeOf(auto Val);
#line 1
)code");
    cpp_push_default_include(pfile, "/tmp/tmpcode.cpp");

    return Ret;
}

void handle_Xfw_TUnitEnd(cpp_reader*) {
    std::string code = inja::render(R"code(
## for _1, type in Types
$Type ${{ type.NormalizedName }}_type = $Type {
    .$Members = {
## if existsIn(type, "Members")
## for _2, member in type.Members
        {
            "{{ member.Name }}", $MemberInfo (
                $CONSTANT_VALUE($MemberTypes::${{ member.MemberType }})
## if member.MemberType == "Field"
                "{{ member.Name }}",
                [] (void* _obj) {
## if not member.IsPublic
                    throw "The field {{ member.Name }} is not public!";
## endif
## if member.IsStatic
                    return &({{ type.FullName }}::{{ member.Name }});
## else
                    return &(reinterpret_cast<{{ type.FullName }}*>(_obj)->{{ member.Name }});
## endif
                }
## else if member.MemberType == "Method"
                "{{ member.Name }}",
                [] (void* _obj, std::vector<void*> _args) {
## if not member.IsPublic
                    throw "The method {{ member.Name }} is not public!";
## endif
## if member.IsStatic
                    assert(_args.size() ==
                    auto _Ret = {{ type.FullName }}::{{ member.Name }};
## else
                    return &(reinterpret_cast<{{ type.FullName }}*>(_obj)->{{ member.Name }});
## endif
                }
## endif
            )
        },
## endfor
## endif
    }
};
## endfor
$Type $TypeOf(auto Val) {
    using TVal = std::decay_t<decltype(Val)>;
## for _1, type in Types
    if constexpr (std::is_same_v<TVal, {{ type.FullName }}>) {
        return ${{ type.NormalizedName }}_type;
    }
## endfor
}
)code", ReflInfo);
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
    register_callback(plugin_info->full_name, PLUGIN_FINISH, [] (void*, void*) {
        remove("/tmp/tmpcode.cpp");
        remove("/tmp/tmpcode1.cpp");
    }, nullptr);
    register_callback(plugin_info->full_name, PLUGIN_PRAGMAS, [] (void*, void*) {
        c_register_pragma(nullptr, "Xfw_TUnitEnd", handle_Xfw_TUnitEnd);
    }, nullptr);

    return 0;
}

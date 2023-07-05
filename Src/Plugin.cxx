#include "Includes.hxx"

tree ReflectorClass = NULL_TREE;
std::vector<std::string> source_code;

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
    }
}

enum class AccessModifier : uint8_t {
    Private, Protected, Public
};
enum class MemberType : uint8_t {
    Field, Method
};
AccessModifier get_access_modifier(tree t) {
    return TREE_PRIVATE(t)   ? AccessModifier::Private :
           TREE_PROTECTED(t) ? AccessModifier::Protected :
           AccessModifier::Public;
}
struct Member {
    std::string Name;
    std::string MemType;
    AccessModifier Accessibility;
    bool IsStatic;
    std::string Type;
    std::string ReturnType;
    int TypeSize;

    std::string InjectedCode = ""; // code to be injected in the epilogue before everything
    std::string ReflectionCode = ""; // code that has the implementation of actual reflection functions (like $Invoke)
};
struct Type {
    std::string FullName;
    std::string NormalizedName;
    std::vector<Member> Members;
};
// $TypeOf(MyClass).$Members["MyFn"].$Invoke(_myobj, {1, 2, 3});
static std::vector<Type> Types{};
std::string Epilogue;

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
        or  std::string(type_as_string(Rslt, TFF_SCOPE)).contains("dylib")
        or  std::string(type_as_string(Rslt, TFF_SCOPE)).contains("fmt")
            )
        return Rslt;

    //std::cout << type_as_string(Rslt, TFF_SCOPE) << std::endl;

    if (CLASSTYPE_TEMPLATE_INFO(Rslt))
        return Rslt;

    std::string full_name = type_as_string(Rslt, TFF_SCOPE);
    std::string normalized_name = full_name;
    replace_all(normalized_name, "::", "$");

    Type type;
    std::vector<Member> members;

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
        tree member_tree = TYPE_FIELDS(Rslt); int i = 0;
        for (;;) {
            Member member;

            if (TREE_CODE(member_tree) == TYPE_DECL)
                break;

            if (TREE_CODE(member_tree) == VAR_DECL || TREE_CODE(member_tree) == FIELD_DECL || TREE_CODE(member_tree) == FUNCTION_DECL) {

            } else {
                member_tree = TREE_CHAIN(member_tree);
                ++i;
                continue;
            }
#ifdef DEBUG
            printf("    (%d): %s (access modifier \"%s\", type \"%s\", static \"%s\")\n",
                i,
                decl_as_string(member_tree, TFF_SCOPE), TREE_PRIVATE(member_tree) ? "private" : (TREE_PROTECTED(member_tree) ? "protected" : "public"),
                type_as_string(TREE_TYPE(member_tree), TFF_SCOPE),
                TREE_STATIC(member_tree) ? "true" : "false"
            );
#endif

            std::string field_name = decl_as_string(member_tree, TFF_UNQUALIFIED_NAME);
            if (TREE_CODE(member_tree) == FUNCTION_DECL) {
                field_name = field_name.substr(0, field_name.find("("));
            }
            bool is_field = TREE_CODE(member_tree) == VAR_DECL or TREE_CODE(member_tree) == FIELD_DECL;
            bool is_method = TREE_CODE(member_tree) == FUNCTION_DECL;
            member.Name = field_name;
            if (is_field)
                member.MemType = "Field";
            else if (is_method)
                member.MemType = "Method";
            member.IsStatic = not (bool) DECL_NONSTATIC_MEMBER_P(member_tree);
            member.Accessibility = get_access_modifier(member_tree);

            if (is_field) {
                member.Type = type_as_string(TREE_TYPE(member_tree), TFF_SCOPE); // For member_tree, "Type" is the member_tree declaration type
                //member.TypeSize = wi::to_wide(TYPE_SIZE(TREE_TYPE(member_tree))).to_shwi();
                if (TREE_CODE(TREE_TYPE(member_tree)) == ARRAY_TYPE) {
                    tree elemType = TREE_TYPE(TREE_TYPE(member_tree)); //debug_tree(TREE_TYPE(member_tree));
                    std::string asteriks = "*";
                    //while (not (TREE_CODE(TREE_TYPE(member_tree)) != ARRAY_TYPE)) {
                    //    elemType = TREE_TYPE(member_tree);
                    //    asteriks += "*";
                    //}
                    member.Type = std::string(type_as_string(elemType, TFF_SCOPE)) + asteriks;
                }
                member.ReflectionCode = format(R"code([] (void* obj) -> std::any {{
    return std::ref({type_name}{access_op}{field_name});
}}
)code"_cf,
    "type"_a = member.Type,
    "type_name"_a = (member.IsStatic ? full_name : format("(*({}*)obj)"_cf, full_name)),
    "access_op"_a = (member.IsStatic ? "::" : "."),
    "field_name"_a = field_name);
                //if (member.IsStatic)
                    //member.InjectedCode = format("{0} {1}::{2};\n", member.Type, full_name, field_name);
            }

            if (is_method) {
                if (field_name == full_name) {// TODO: ctor/dtor handling
                    member_tree = TREE_CHAIN(member_tree); ++i;
                    continue;
                }

                member.Type = type_as_string(TREE_TYPE(TREE_TYPE(member_tree)), TFF_SCOPE); // For method, "Type" is the return type
                if (TYPE_SIZE(TREE_TYPE(TREE_TYPE(member_tree))) != NULL_TREE) {
                    member.TypeSize = wi::to_wide(TYPE_SIZE(TREE_TYPE(TREE_TYPE(member_tree)))).to_shwi();
                } // otherwise, the return type is void
                //debug_tree(FUNCTION_ARG_CHAIN(member_tree));
                std::string signature = decl_as_string(member_tree, TFF_RETURN_TYPE);
                auto first = signature.find('(') + 1;
                auto last = signature.find_last_of(')');
                std::string argListStr = signature.substr(first, last-first);
                std::vector<std::string> argList{};
                if (not argListStr.empty())
                    argList = split(argListStr, ", ");
                std::string callStr = "";
                for (int j=0; j<argList.size(); ++j)
                    callStr += format("std::any_cast<{}>(args[{}]){}",
                                      argList[j], j, j == argList.size() - 1 ? "" : ", ");


                // return void -> nullptr
                // return value (copyable) -> copy
                // return value (non-copyable) -> move
                // return reference -> wrap with std::ref, std::cref

                if (member.Type == "void") {
                    member.ReflectionCode = format(R"code([] (void* obj, std::vector<std::any> args) -> std::any {{
    {type_name}{access_op}{fn_name}({call_args});
    return nullptr;
}})code",
    "type_name"_a = (member.IsStatic ? full_name : format("(*({}*)obj)"_cf, full_name)),
    "access_op"_a = (member.IsStatic ? "::" : "."),
    "fn_name"_a = field_name,
    "call_args"_a = callStr);
                } else if (member.Type.ends_with('&')) {
                    member.ReflectionCode = format(R"code([] (void* obj, std::vector<std::any> args) -> std::any {{
    return std::{ref_method}({type_name}{access_op}{fn_name}({call_args}));
}})code",
    "type_name"_a = (member.IsStatic ? full_name : format("(*({}*)obj)"_cf, full_name)),
    "access_op"_a = (member.IsStatic ? "::" : "."),
    "fn_name"_a = field_name,
    "call_args"_a = callStr,
    "ref_method"_a = (member.Type.starts_with("const") ? "cref" : "ref"));
                } else {
                    member.ReflectionCode = format(R"code([] (void* obj, std::vector<std::any> args) -> std::any {{
    return {type_name}{access_op}{fn_name}({call_args});
}})code",
    "type_name"_a = (member.IsStatic ? full_name : format("(*({}*)obj)"_cf, full_name)),
    "access_op"_a = (member.IsStatic ? "::" : "."),
    "fn_name"_a = field_name,
    "call_args"_a = callStr);
                }
            }

            // TODO: add ctor, dtor detection
            cont_loop:
            members.push_back(member);
            member_tree = TREE_CHAIN(member_tree); ++i;
        }
    }
    Types.push_back(Type {
            .FullName = full_name,
            .NormalizedName = normalized_name,
            .Members = members
    });

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
    //std::cout << ReflInfo.dump() << std::endl;
    std::string indent = "";
#define INDENT indent += "    "
#define UNINDENT indent.erase(indent.length() - 4 )
    Epilogue += "#line 1 \"<epilogue>\"\n";
    for (const Type& type : Types)
        for (const Member& member : type.Members)
            Epilogue += member.InjectedCode;
    Epilogue += "struct $Reflector {\n";
    for (const Type& type : Types) {
        INDENT;
        Epilogue += indent + format("static $Type $Get_{0}_type() {{\n", type.NormalizedName);      INDENT;
        Epilogue += indent + format("static $Type ${0}_type = $Type {{\n", type.NormalizedName);    INDENT;
        Epilogue += indent + ".$Members = {\n"; INDENT;
        for (const Member& member : type.Members) {
            Epilogue += indent + "{\n";               INDENT;
            Epilogue += indent + format(R"code(
{indent}"{mem_name}", $MemberInfo (
{indent_1}$CONSTANT_VALUE($MemberTypes::${mem_type}),
{indent_1}"{mem_name}",
{indent_1}{reflection_code}
{indent})

)code"_cf, "mem_name"_a = member.Name, "indent"_a = indent, "indent_1"_a = indent + "    ", "mem_type"_a = member.MemType, "reflection_code"_a = member.ReflectionCode);
            UNINDENT; Epilogue += indent + "},\n";
        }
        UNINDENT; Epilogue += indent + "}\n";
        UNINDENT; Epilogue += indent + "};\n";
        Epilogue += indent + format("return ${0}_type;\n", type.NormalizedName);
        UNINDENT; Epilogue += indent + "}\n"; UNINDENT;
    }
    Epilogue += "};\n"; indent = ""; INDENT;
    Epilogue += "$Type $TypeOf(auto Val) {\n";
    Epilogue += indent + "using TVal = std::decay_t<decltype(Val)>;\n";
    for (const Type& type : Types) {
        Epilogue += indent + format("if constexpr (std::is_same_v<TVal, {0}>) {{\n", type.FullName); INDENT;
        Epilogue += indent + format("return $Reflector::$Get_{0}_type();\n", type.NormalizedName); UNINDENT;
        Epilogue += indent + "}\n";
    }
    Epilogue += "}";
    //std::cout << Epilogue << std::endl;
#ifdef DEBUG
    //std::cout << code;
#endif
    write_to_file("/tmp/tmpcode1.cpp", "%s", Epilogue.c_str());

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
            traverse(global_namespace, [] (tree t1) {
                if (std::string(decl_as_string(t1, TFF_SCOPE)) == "$Reflector") {
                    ReflectorClass = TREE_TYPE(t1);
                }
            });
            //set_global_friend(ReflectorClass);
        } else {
            //if (TREE_CODE(t) == RECORD_TYPE) {
                make_friend_class(t, ReflectorClass, false);
            //}
        }
    }
}

static std::set<std::string> extern_strs{};
static std::set<std::string> include_strs{};

std::string getTreeName(tree node) {
    auto code = TREE_CODE(node);
    tree_code_class tclass = TREE_CODE_CLASS(code);

    if (tclass == tcc_declaration)
    {
        if (DECL_NAME (node))
            return format("{}", IDENTIFIER_POINTER (DECL_NAME (node)));
        else if (code == LABEL_DECL
                 && LABEL_DECL_UID (node) != -1)
        {
            return format("L.{}", (int) LABEL_DECL_UID (node));
        }
        else
        {
            return format("{}.{}", code == CONST_DECL ? 'C' : 'D',
                         DECL_UID (node));
        }
    }
    else if (tclass == tcc_type)
    {
        if (TYPE_NAME (node))
        {
            if (TREE_CODE (TYPE_NAME (node)) == IDENTIFIER_NODE)
                return format("{}", IDENTIFIER_POINTER (TYPE_NAME (node)));
            else if (TREE_CODE (TYPE_NAME (node)) == TYPE_DECL
                     && DECL_NAME (TYPE_NAME (node)))
                return format("{}",
                         IDENTIFIER_POINTER (DECL_NAME (TYPE_NAME (node))));
        }
    }
    if (code == IDENTIFIER_NODE)
        return format("{}", IDENTIFIER_POINTER (node));
    return "???";
}

std::tuple<
    std::string, // return type
    std::vector<std::string> // arglist
> getFnInfo(tree fn) {
    tree args = TYPE_ARG_TYPES(TREE_TYPE(fn));
    tree elem = args;
    std::vector<std::string> args_vec;
    while (elem != NULL) {
        tree val = TREE_VALUE(elem);
        args_vec.emplace_back(type_as_string(val, TFF_SCOPE));
        elem = TREE_CHAIN(elem);
    }
    args_vec.pop_back(); // pop the 'void'

    tree ret = fndecl_declared_return_type(fn);
    std::string ret_str = type_as_string(ret, TFF_SCOPE);

    return std::make_tuple(ret_str, args_vec);
}
std::map<std::string, std::string> getFnTemplArgsInfo(tree fn) {
    tree args = TI_ARGS(DECL_TEMPLATE_INFO(fn));
    auto len = TREE_VEC_LENGTH(args);
    std::map<std::string, std::string> templ_args{};

    for (int i=0; i<len; ++i) {
        tree t = TREE_VEC_ELT(args, i);
        if (TREE_CODE(t) == 276) { // dunno why TEMPLATE_PARM_INDEX == 278 but in reality it is 276...
            t = TEMPLATE_PARM_DECL(t);
            std::string ty = type_as_string(TREE_TYPE(t), TFF_SCOPE);
            std::string id = decl_as_string(t, TFF_SCOPE);
            templ_args[id] = ty;
        } else if (TREE_CODE(t) == 278) { // ditto
            t = TEMPLATE_TYPE_DECL(t);
            templ_args[type_as_string(TREE_TYPE(t), TFF_SCOPE)] = "typename";
        }
    }
    return templ_args;
}

tree addr_expr_replacement(tree* the_tree, int* walk_subtrees, void* context) {
    int code =  TREE_CODE(*the_tree);
    fmt::println(
            "{}: TREE_CODE={}, TREE_CLASS={}, DECL_P={}, IS_NAMESPACE={}, DECL_NAME_IS_NULL={}",
            getTreeName(*the_tree), code, (int)TEMPLATE_DECL, get_tree_code_name(TREE_CODE(*the_tree)),
            (TREE_CODE_CLASS_STRING(TREE_CODE_CLASS(TREE_CODE(*the_tree)))), DECL_P (*the_tree),
            TREE_CODE (*the_tree) == NAMESPACE_DECL, DECL_NAME (*the_tree) == NULL_TREE);
    // COMPONENT_REF, INDIRECT_REF, BASELINK
    //debug_tree(*the_tree);

    if ((!DECL_P (*the_tree)
        || TREE_CODE (*the_tree) == NAMESPACE_DECL
        || DECL_NAME (*the_tree) == NULL_TREE)
        && TREE_CODE(*the_tree) != 275 /*275 is TEMPLATE_DECL for some reasons*/) {
        //debug_tree(*the_tree);
        *walk_subtrees = 1;
        return NULL_TREE;
    }
    *walk_subtrees = 0;

    tree t = *the_tree;
    tree current_fn = (tree) context;

    if (std::string(DECL_SOURCE_FILE(t)) == "./Build/Test.cpp") {
    } else {
        if (std::string(DECL_SOURCE_FILE(t)) != "/usr/include/c++/12/bits/ostream.tcc")
            include_strs.insert(fmt::format("#include \"{}\"", DECL_SOURCE_FILE(t)));
    }

    if (DECL_CONTEXT(t) == current_fn)
        return NULL;

    if (TREE_CODE(t) == VAR_DECL or
        (TREE_CODE(t) == FUNCTION_DECL and not DECL_FUNCTION_MEMBER_P(t))) {
        if (TREE_CODE(t) == VAR_DECL) {
            // TODO: handle templated VAR_DECL case
            extern_strs.insert(fmt::format("extern {0} {1};", type_as_string(TREE_TYPE(t), TFF_SCOPE), decl_as_string(t, TFF_SCOPE)));
        } else if (TREE_CODE(t) == FUNCTION_DECL) {
            if (DECL_TEMPLATE_INFO(t) != NULL_TREE) {
                // templated function
                // STRIP_TEMPLATE?
                auto [ret_str, args_vec] = getFnInfo(t);
                //decl_as_string(op, TFF_NO_TEMPLATE_BINDINGS | TFF_SCOPE | TFF_TEMPLATE_NAME | TFF_RETURN_TYPE) does not include namespaces for arguments
                extern_strs.insert(fmt::format("extern template {0} {1}({2});", ret_str,
                                                  decl_as_string(t,
                                                                 TFF_NO_TEMPLATE_BINDINGS |
                                                                 TFF_SCOPE |
                                                                 TFF_TEMPLATE_NAME |
                                                                 TFF_NO_FUNCTION_ARGUMENTS),
                                                  fmt::join(args_vec, ", ")));
            } else {
                // non templated function
                extern_strs.insert(fmt::format("extern {0};", decl_as_string(t, TFF_SCOPE | TFF_RETURN_TYPE)));
            }
        }
    } else if (TREE_CODE(t) == 275 /*template_decl*/) {
        /*debug_tree(DECL_TEMPLATE_RESULT(t));
        auto [ret_str, args_vec] = getFnInfo(DECL_TEMPLATE_RESULT(t));
        //decl_as_string(op, TFF_NO_TEMPLATE_BINDINGS | TFF_SCOPE | TFF_TEMPLATE_NAME | TFF_RETURN_TYPE) does not include namespaces for arguments
        extern_strs.insert(fmt::format("extern template {0} {1}({2});", ret_str,
                                       decl_as_string(t,
                                                      TFF_NO_TEMPLATE_BINDINGS |
                                                      TFF_SCOPE |
                                                      TFF_TEMPLATE_NAME |
                                                      TFF_NO_FUNCTION_ARGUMENTS),
                                       fmt::join(args_vec, ", ")));*/
    }

    // build_call_expr(tree fndecl: function get called, int n: # of arguments, ...: expression for argument expressions)

    return NULL;
}

bool once = false;
void finish_function_callback(void* event_data, void* user_data) {
    tree t = (tree) event_data;
    tree* f;



    //if (once) return;
    //once = true;

    //if (DECL_NAME(t) != NULL)
    //    std::cout << "$$: " << std::string(decl_as_string(t, TFF_UNQUALIFIED_NAME)) << std::endl;

    if (DECL_NAME(t) != NULL and
        std::string(decl_as_string(t, TFF_UNQUALIFIED_NAME)).contains("main")) {
        tree& stmt_list = DECL_SAVED_TREE(t);
        tree_stmt_iterator iter = tsi_start(stmt_list);
        int i = 0;
        while (iter != tsi_last(stmt_list)) {
            //debug_tree(tsi_stmt(iter));
            if (i == 3)
                f = tsi_stmt_ptr(iter);
            tsi_next(&iter);
            ++i;
        }
        tree& expr_stmt = TREE_OPERAND(*f, 0);
        tree& call_expr = TREE_OPERAND(expr_stmt, 0);
        tree& fn = CALL_EXPR_FN(call_expr);

        //tree call_test2 = lookup_name(get_identifier("call_test2$"));
        //debug_tree(call_test2);
        //TREE_TYPE(TREE_TYPE(fn)) = TREE_TYPE(call_test2);
        //TREE_OPERAND(fn, 0) = call_test2;
    } else if (DECL_NAME(t) != NULL and
               std::string(decl_as_string(t, TFF_UNQUALIFIED_NAME)).contains("call_test2") and
               not std::string(decl_as_string(t, TFF_UNQUALIFIED_NAME)).contains("$")) {
        tree* stmt_list;

        if (TREE_CODE(DECL_SAVED_TREE(t)) == STATEMENT_LIST)
            stmt_list = &DECL_SAVED_TREE(t);
        else
            stmt_list = &BIND_EXPR_BODY(DECL_SAVED_TREE(t));

        tree_stmt_iterator iter = tsi_start(*stmt_list);
        int i = 0;
        int start_line = 0, start_col = 0;
        int end_line = 0, end_col = 0;
        while (iter != tsi_last(*stmt_list)) {
            //debug_tree(TREE_OPERAND(tsi_stmt(iter), 0));
            if (i == 0) {
                f = tsi_stmt_ptr(iter);

                start_line = LOCATION_LINE(EXPR_LOCATION_RANGE(*f).m_start);
                start_col = LOCATION_COLUMN(EXPR_LOCATION_RANGE(*f).m_start);
            }
            tsi_next(&iter);
            ++i;
        }
        f = tsi_stmt_ptr(iter);
        end_line = LOCATION_LINE(EXPR_LOCATION_RANGE(*f).m_finish);
        end_col = LOCATION_COLUMN(EXPR_LOCATION_RANGE(*f).m_finish);
        //debug_tree(TREE_OPERAND(TREE_OPERAND(tsi_stmt(iter), 0), 0));
        //std::cout << DECL_SOURCE_FILE(lookup_qualified_name(lookup_name(get_identifier("std")), "cout", LOOK_want::NORMAL, false)) << std::endl;
        //debug_tree(lookup_qualified_name(lookup_name(get_identifier("std")), "is_convertible", LOOK_want::NORMAL, false));
        //lookup_template_class()
        //debug_tree(lookup_name(get_identifier("foobar")));

        auto templ_arg_map = getFnTemplArgsInfo(t);
        auto [ret_str, args_vec] = getFnInfo(t);
        std::string func_name = decl_as_string(t,
                                               TFF_NO_TEMPLATE_BINDINGS |
                                               TFF_SCOPE |
                                               TFF_TEMPLATE_NAME |
                                               TFF_NO_FUNCTION_ARGUMENTS);
        std::string transformed_templ_args, templ_args_in_usings;
        std::vector<std::string> templ_args_name;
        for (const auto& [arg_name, arg_type] : templ_arg_map) {
            transformed_templ_args += format("{} {}, ", arg_type == "typename" ? "const char*" : arg_type, arg_name);
            templ_args_in_usings += format("{} {} = {};", arg_type == "typename" ? "using" : ("constexpr " + arg_type), arg_name, "{}");
            templ_args_name.emplace_back(arg_name);
        }
        transformed_templ_args = transformed_templ_args.substr(0, transformed_templ_args.size() - 2);

        std::string actual_args = fmt::format("{}", fmt::join(args_vec, ", "));
        std::string actual_code = "";
        actual_code += source_code[start_line - 1].substr(start_col - 1) + "\n";
        for (int j=start_line; j <= end_line - 2; ++j) {
           actual_code += source_code[j] + "\n";
        }
        actual_code += source_code[end_line - 1].substr(0) + "\n";
        // TODO: find a proper way to get beginning and ending location for a function body

        walk_tree_fn wtree = (walk_tree_fn) addr_expr_replacement;
        cp_walk_tree(stmt_list, wtree, t, NULL);

        fmt::print("=======include_strs========\n{}==================\n", fmt::join(include_strs, "\n"));

        fmt::print("start: (line = {}, col = {})\nend: (line = {}, col = {})\n", start_line, start_col, end_line, end_col);

        //debug_tree(DECL_SAVED_TREE(t));

        std::string final_code = format(R"code(
#line 1 "<tmp_code>"
void write_to_file(const char* path, const char* fmt, auto&&... Args) {{
    FILE* file = fopen(path, "a"); if (file == NULL) return;
    fprintf(file, fmt, std::forward<decltype(Args)>(Args)...);
    fclose(file);
}}
{ret_type} {func_name}$({transformed_templ_args} {actual_args}) {{
    remove("/tmp/tmpcode3.cxx");
    remove("/tmp/tmpobj3.o");
    remove("/tmp/libtmp.so");
    std::string $code = fmt::format(R"code2(
{includes}
{externs}
extern "C" {ret_type} {func_name}({actual_args}) {{{{
    {templ_args_in_usings}
    {actual_code}
}}}}
)code2", {templ_args_name});
    write_to_file("/tmp/tmpcode3.cxx", "%s", $code.c_str());
    system("g++-13 /tmp/tmpcode3.cxx -fPIC -c -o /tmp/tmpobj3.o && gcc-13 -shared /tmp/tmpobj3.o -o /tmp/libtmp.so && rm -rf /tmp/tmpobj3.o");
    dylib lib ("/tmp", "tmp");
    auto fn = lib.get_function<void()>("{func_name}");
    fn();

    remove("/tmp/tmpcode3.cxx");
    remove("/tmp/tmpobj3.o");
    remove("/tmp/libtmp.so");
}}
#line 1 "Test.cpp"
)code"_cf, "includes"_a = fmt::join(include_strs, "\n"), "externs"_a = fmt::join(extern_strs, "\n"), "ret_type"_a=ret_str, "func_name"_a=func_name, "transformed_templ_args"_a=transformed_templ_args, "actual_args"_a=actual_args,
"templ_args_in_usings"_a=templ_args_in_usings, "actual_code"_a=actual_code, "end_line_no"_a=end_line+1, "templ_args_name"_a=fmt::join(templ_args_name, ", "));

        std::cerr << final_code;

        write_to_file("/tmp/tmpcode4.cpp", "%s", final_code.c_str());

#define CPP_OPTION(PFILE, OPTION) ((PFILE)->opts.OPTION)

        cpp_reader* parser = cpp_create_reader (CLK_CXX23, ident_hash, line_table);
        CPP_OPTION(parser, warn_dollars) = false;
        parser->bracket_include = parse_in->bracket_include;
        parser->cb.diagnostic = parse_in->cb.diagnostic;
        subhook::ScopedHookRemove _remove(&HookToCppReadMainFile);
        cpp_read_main_file (parser, "/tmp/tmpcode4.cpp", false);

        cpp_reader* old_parse_in = parse_in; parse_in = parser;
        cp_lexer* lexer = cp_lexer_alloc ();
        cp_token token{};
        //cp_token eof = the_parser->lexer->buffer->pop();
        int cur_idx = 0;
        for (int j=0; j< vec_safe_length(the_parser->lexer->buffer); ++j)
            if ((*(the_parser->lexer->buffer))[j].location == the_parser->lexer->next_token->location)
                cur_idx = j;
        int j = 0;
        do {
            cp_lexer_get_preprocessor_token (C_LEX_STRING_NO_JOIN, &token);
            if (token.type == CPP_EOF)
                break;
            vec_safe_insert(the_parser->lexer->buffer,cur_idx+j, token);
            if (j == 0)
                the_parser->lexer->next_token = &((*(the_parser->lexer->buffer))[cur_idx + j]);
            j++;
        } while (token.type != CPP_EOF);
        //vec_safe_push(the_parser->lexer->buffer, eof);
        lexer->last_token = reinterpret_cast<cp_token_position>(lexer->buffer->length() - 1);

        parse_in = old_parse_in;
        cp_lexer_destroy(lexer);
        cpp_finish(parser, nullptr);
        cpp_destroy(parser);
        remove("/tmp/tmpcode4.cpp");
    }
}

void start_unit_cb(void*, void*) {

    if (source_code.empty())
        source_code = read_file(parse_in->main_file->path);
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
    /*=assert(SetGlobalFriendHook.Install(
        (void*) set_global_friend,
        (void*) patched_set_global_friend,
        subhook::HookFlags::HookFlag64BitOffset
    ));*/

    register_callback(plugin_info->full_name, PLUGIN_START_UNIT, start_unit_cb, nullptr);
    register_callback(plugin_info->full_name, PLUGIN_FINISH, [] (void*, void*) {
        remove("/tmp/tmpcode.cpp");
        remove("/tmp/tmpcode1.cpp");
    }, nullptr);
    register_callback(plugin_info->full_name, PLUGIN_PRAGMAS, [] (void*, void*) {
        c_register_pragma(nullptr, "Xfw_TUnitEnd", handle_Xfw_TUnitEnd);
    }, nullptr);
    register_callback(plugin_info->full_name, PLUGIN_FINISH_TYPE, finish_type_callback, nullptr);
    register_callback(plugin_info->full_name, PLUGIN_FINISH_PARSE_FUNCTION, finish_function_callback, nullptr);

    return 0;
}

#ifndef XFW_PCH_HPP
#define XFW_PCH_HPP

#include <cstdio>
#include <utility>
#include <map>
#include <iostream>
#include <filesystem>
#include <climits>
#include <array>
#include <set>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/compile.h>

#define DBG_MACRO_NO_WARNING
#include <dbg.h>
#include <subhook/subhook.c>

#include <gcc-plugin.h>
#include <print-tree.h>
#include <langhooks.h>
#include <cp/cp-tree.h>
#include <cp/cxx-pretty-print.h>
#include <cp/name-lookup.h>
#include <cp/type-utils.h>
#include <c-family/c-common.h>
#include <c-family/c-objc.h>
#include <c-family/c-pragma.h>
#include <c-family/c-pretty-print.h>
#include <cppbuiltin.h>
#include <cppdefault.h>
#include <cpplib.h>
#include <stringpool.h>
#include <toplev.h>
#include <timevar.h>

// Who the f defined true & false???
#undef false
#undef true

struct GTY(()) tree_check {
tree value;
vec<deferred_access_check, va_gc> *checks;
tree qualifying_scope;
};
struct GTY (()) cp_token {
enum cpp_ttype type : 8;
enum rid keyword : 8;
unsigned char flags;
bool implicit_extern_c : 1;
bool error_reported : 1;
bool purged_p : 1;
bool tree_check_p : 1;
bool main_source_p : 1;
location_t location;
union cp_token_value {
    struct tree_check* GTY((tag ("true"))) tree_check_value;
    tree GTY((tag ("false"))) value;
} GTY((desc ("%1.tree_check_p"))) u;
};
typedef struct cp_token *cp_token_position;
struct GTY (()) cp_lexer {
vec<cp_token, va_gc> *buffer;
cp_token_position GTY ((skip)) last_token;
cp_token_position GTY ((skip)) next_token;
vec<cp_token_position> GTY ((skip)) saved_tokens;
enum cpp_ttype saved_type : 8;
enum rid saved_keyword : 8;
struct cp_lexer *next;
bool debugging_p;
bool in_pragma;
bool in_omp_attribute_pragma;
bool orphan_p;
};
struct GTY(()) cp_token_cache {
cp_token * GTY((skip)) first;
cp_token * GTY ((skip)) last;
};
typedef cp_token_cache *cp_token_cache_ptr;
struct cp_token_ident {
    unsigned int ident_len;
    const char *ident_str;
    unsigned int before_len;
    const char *before_str;
    unsigned int after_len;
    const char *after_str;
};
struct GTY(()) cp_default_arg_entry {
tree class_type;
tree decl;
};
struct GTY(()) cp_unparsed_functions_entry {
vec<cp_default_arg_entry, va_gc> *funs_with_default_args;
vec<tree, va_gc> *funs_with_definitions;
vec<tree, va_gc> *nsdmis;
vec<tree, va_gc> *noexcepts;
};
enum cp_parser_status_kind {
    CP_PARSER_STATUS_KIND_NO_ERROR,
    CP_PARSER_STATUS_KIND_ERROR,
    CP_PARSER_STATUS_KIND_COMMITTED
};
struct GTY (()) cp_parser_context {
enum cp_parser_status_kind status;
tree object_type;
struct cp_parser_context *next;
};
struct cp_omp_declare_simd_data {
    bool error_seen;
    bool fndecl_seen;
    bool variant_p;
    location_t loc;
    vec<cp_token_cache_ptr> tokens;
    tree *attribs[2];
};
struct cp_oacc_routine_data : cp_omp_declare_simd_data { tree clauses; };
struct GTY(()) cp_parser {
cp_lexer *lexer;
tree scope;
tree object_scope;
tree qualifying_scope;
cp_parser_context *context;
bool allow_gnu_extensions_p;
bool greater_than_is_operator_p;
bool default_arg_ok_p;
bool integral_constant_expression_p;
bool allow_non_integral_constant_expression_p;
bool non_integral_constant_expression_p;
#define LOCAL_VARS_FORBIDDEN (1 << 0)
#define THIS_FORBIDDEN (1 << 1)
#define LOCAL_VARS_AND_THIS_FORBIDDEN (LOCAL_VARS_FORBIDDEN | THIS_FORBIDDEN)
unsigned char local_variables_forbidden_p;
bool in_unbraced_linkage_specification_p;
bool in_declarator_p;
bool in_template_argument_list_p;
#define IN_SWITCH_STMT		1
#define IN_ITERATION_STMT	2
#define IN_OMP_BLOCK		4
#define IN_OMP_FOR		8
#define IN_IF_STMT             16
unsigned char in_statement;
bool in_switch_statement_p;
bool in_type_id_in_expr_p;
bool translate_strings_p;
bool in_function_body;
unsigned char in_transaction;
bool colon_corrects_to_scope_p;
bool colon_doesnt_start_class_def_p;
bool objective_c_message_context_p;
const char *type_definition_forbidden_message;
const char *type_definition_forbidden_message_arg;
vec<cp_unparsed_functions_entry, va_gc> *unparsed_queues;
unsigned num_classes_being_defined;
unsigned num_template_parameter_lists;
cp_omp_declare_simd_data * GTY((skip)) omp_declare_simd;
cp_oacc_routine_data * GTY((skip)) oacc_routine;
bool auto_is_implicit_function_template_parm_p;
bool fully_implicit_function_template_p;
bool omp_attrs_forbidden_p;
tree implicit_template_parms;
cp_binding_level* implicit_template_scope;
bool in_result_type_constraint_p;
int prevent_constrained_type_specifiers;
location_t innermost_linkage_specification_location;
};
struct _cpp_file {
    const char *name;
    const char *path;
    const char *pchname;
    const char *dir_name;
    struct _cpp_file *next_file;
    const uchar *buffer;
    const uchar *buffer_start;
    const cpp_hashnode *cmacro;
    cpp_dir *dir;
    struct stat st;
    int fd;
    int err_no;
    unsigned short stack_count;
    bool once_only : 1;
    bool dont_read : 1;
    bool buffer_valid : 1;
    bool implicit_preinclude : 1;
    int header_unit : 2;
};
#if HAVE_ICONV
#include <iconv.h>
#else
#define HAVE_ICONV 0
typedef int iconv_t;  /* dummy */
#endif

typedef bool (*convert_f) (iconv_t, const unsigned char *, size_t, struct _cpp_strbuf *);
struct cset_converter {
    convert_f func;
    iconv_t cd;
    int width;
    const char* from;
    const char* to;
};
struct _cpp_line_note {
    const unsigned char *pos;
    unsigned int type;
};
struct cpp_buffer {
    const unsigned char *cur;
    const unsigned char *line_base;
    const unsigned char *next_line;
    const unsigned char *buf;
    const unsigned char *rlimit;
    const unsigned char *to_free;
    _cpp_line_note *notes;
    unsigned int cur_note;
    unsigned int notes_used;
    unsigned int notes_cap;
    struct cpp_buffer *prev;
    struct _cpp_file *file;
    const unsigned char *timestamp;
    struct if_stack *if_stack;
    bool need_line : 1;
    bool warned_cplusplus_comments : 1;
    bool from_stage3 : 1;
    bool return_at_eof : 1;
    unsigned char sysp;
    struct cpp_dir dir;
    struct cset_converter input_cset_desc;
};
struct _cpp_buff {
    struct _cpp_buff *next;
    unsigned char *base, *cur, *limit;
};
struct lexer_state {
    unsigned char in_directive;
    unsigned char directive_wants_padding;
    unsigned char skipping;
    unsigned char angled_headers;
    unsigned char in_expression;
    unsigned char save_comments;
    unsigned char va_args_ok;
    unsigned char poisoned_ok;
    unsigned char prevent_expansion;
    unsigned char parsing_args;
    unsigned char discarding_output;
    unsigned int skip_eval;
    unsigned char in_deferred_pragma;
    unsigned char directive_file_token;
    unsigned char pragma_allow_expansion;
};
struct spec_nodes {
    cpp_hashnode *n_defined;
    cpp_hashnode *n_true;
    cpp_hashnode *n_false;
    cpp_hashnode *n__VA_ARGS__;
    cpp_hashnode *n__VA_OPT__;
    enum {M_EXPORT, M_MODULE, M_IMPORT, M__IMPORT, M_HWM};
    cpp_hashnode *n_modules[M_HWM][2];
};
union utoken {
    const cpp_token *token;
    const cpp_token **ptoken;
};
struct macro_context{
    cpp_hashnode *macro_node;
    location_t *virt_locs;
    location_t *cur_virt_loc;
};
enum context_tokens_kind {
    TOKENS_KIND_INDIRECT,
    TOKENS_KIND_DIRECT,
    TOKENS_KIND_EXTENDED
};
struct cpp_context {
    cpp_context *next, *prev;
    union {
        struct {
            union utoken first;
            union utoken last;
        } iso;
        struct {
            const unsigned char *cur;
            const unsigned char *rlimit;
        } trad;
    } u;
    _cpp_buff *buff;
    union {
        macro_context *mc;
        cpp_hashnode *macro;
    } c;
    enum context_tokens_kind tokens_kind;
};
struct tokenrun {
    tokenrun *next, *prev;
    cpp_token *base, *limit;
};
struct cpp_reader {
    cpp_buffer *buffer;
    cpp_buffer *overlaid_buffer;
    struct lexer_state state;
    class line_maps *line_table;
    location_t directive_line;
    _cpp_buff *a_buff;
    _cpp_buff *u_buff;
    _cpp_buff *free_buffs;
    struct cpp_context base_context;
    struct cpp_context *context;
    const struct directive *directive;
    cpp_token directive_result;
    location_t invocation_location;
    cpp_hashnode *top_most_macro_node;
    bool about_to_expand_macro_p;
    struct cpp_dir *quote_include;
    struct cpp_dir *bracket_include;
    struct cpp_dir no_search_path;
    struct _cpp_file *all_files;
    struct _cpp_file *main_file;
    struct htab *file_hash;
    struct htab *dir_hash;
    struct file_hash_entry_pool *file_hash_entries;
    struct htab *nonexistent_file_hash;
    struct obstack nonexistent_file_ob;
    bool quote_ignores_source_dir;
    bool seen_once_only;
    const cpp_hashnode *mi_cmacro;
    const cpp_hashnode *mi_ind_cmacro;
    bool mi_valid;
    cpp_token *cur_token;
    tokenrun base_run, *cur_run;
    unsigned int lookaheads;
    unsigned int keep_tokens;
    unsigned char *macro_buffer;
    unsigned int macro_buffer_len;
    struct cset_converter narrow_cset_desc;
    struct cset_converter utf8_cset_desc;
    struct cset_converter char16_cset_desc;
    struct cset_converter char32_cset_desc;
    struct cset_converter wide_cset_desc;
    const unsigned char *date;
    const unsigned char *time;
    time_t time_stamp;
    int time_stamp_kind;
    cpp_token avoid_paste;
    cpp_token endarg;
    class mkdeps *deps;
    struct obstack hash_ob;
    struct obstack buffer_ob;
    struct pragma_entry *pragmas;
    struct cpp_callbacks cb;
    struct ht *hash_table;
    struct op *op_stack, *op_limit;
    struct cpp_options opts;
    struct spec_nodes spec_nodes;
    bool our_hashtable;
    struct {
        unsigned char *base;
        unsigned char *limit;
        unsigned char *cur;
        location_t first_line;
    } out;
    const unsigned char *saved_cur, *saved_rlimit, *saved_line_base;
    struct cpp_savedstate *savedstate;
    unsigned int counter;
    cpp_comment_table comments;
    struct def_pragma_macro *pushed_macros;
    location_t forced_token_location;
    location_t main_loc;
};
typedef struct c_binding* c_binding_ptr;
class c_struct_parse_info {
public:
    auto_vec<tree> struct_types;
    auto_vec<c_binding_ptr> fields;
    auto_vec<tree> typedefs_seen;
};
int plugin_is_GPL_compatible;

#define CP_LEXER_BUFFER_SIZE ((256 * 1024) / sizeof (cp_token))
#define CP_SAVED_TOKEN_STACK 5

cp_lexer* cp_lexer_alloc() {
    cp_lexer *lexer = ggc_cleared_alloc<cp_lexer> ();
    lexer->debugging_p = false;
    lexer->saved_tokens.create (CP_SAVED_TOKEN_STACK);
    vec_alloc (lexer->buffer, CP_LEXER_BUFFER_SIZE);
    return lexer;
}
void cp_lexer_destroy(cp_lexer *lexer) {
    if (lexer->buffer)
        vec_free (lexer->buffer);
    else {
        lexer->last_token->type = lexer->saved_type;
        lexer->last_token->keyword = lexer->saved_keyword;
    }
    lexer->saved_tokens.release ();
    ggc_free (lexer);
}
static void cp_lexer_get_preprocessor_token (unsigned flags, cp_token *token) {
    static int is_extern_c = 0;
    token->type = c_lex_with_flags (&token->u.value, &token->location, &token->flags, flags);
    token->keyword = RID_MAX;
    token->purged_p = false;
    token->error_reported = false;
    token->tree_check_p = false;
    token->main_source_p = line_table->depth <= 1;

    is_extern_c += pending_lang_change;
    pending_lang_change = 0;
    token->implicit_extern_c = is_extern_c > 0;

    if (token->type == CPP_NAME) {
        if (IDENTIFIER_KEYWORD_P (token->u.value)) {
            token->type = CPP_KEYWORD;
            token->keyword = C_RID_CODE (token->u.value);
        }
        else {
            if (warn_cxx11_compat
                && ((C_RID_CODE (token->u.value) >= RID_FIRST_CXX11
                     && C_RID_CODE (token->u.value) <= RID_LAST_CXX11)
                    || C_RID_CODE (token->u.value) == RID_ALIGNOF
                    || C_RID_CODE (token->u.value) == RID_ALIGNAS
                    || C_RID_CODE (token->u.value)== RID_THREAD)) {
                warning_at (
                        token->location, OPT_Wc__11_compat,
                        "identifier %qE is a keyword in C++11",
                        token->u.value
                );
                C_SET_RID_CODE (token->u.value, RID_MAX);
            }
            if (warn_cxx20_compat
                && C_RID_CODE (token->u.value) >= RID_FIRST_CXX20
                && C_RID_CODE (token->u.value) <= RID_LAST_CXX20) {
                warning_at (
                        token->location, OPT_Wc__20_compat,
                        "identifier %qE is a keyword in C++20",
                        token->u.value
                );
                C_SET_RID_CODE (token->u.value, RID_MAX);
            }
            token->keyword = RID_MAX;
        }
    }
    else if (token->type == CPP_AT_NAME) {
        token->type = CPP_KEYWORD;
        switch (C_RID_CODE (token->u.value)) {
            case RID_CLASS:     token->keyword = RID_AT_CLASS; break;
            case RID_PRIVATE:   token->keyword = RID_AT_PRIVATE; break;
            case RID_PROTECTED: token->keyword = RID_AT_PROTECTED; break;
            case RID_PUBLIC:    token->keyword = RID_AT_PUBLIC; break;
            case RID_THROW:     token->keyword = RID_AT_THROW; break;
            case RID_TRY:       token->keyword = RID_AT_TRY; break;
            case RID_CATCH:     token->keyword = RID_AT_CATCH; break;
            case RID_SYNCHRONIZED: token->keyword = RID_AT_SYNCHRONIZED; break;
            default:            token->keyword = C_RID_CODE (token->u.value);
        }
    }
}

cp_parser* the_parser;
void (*debug_cppsr_ptr)(cp_parser*) = (void(*)(cp_parser*)) 0x00000000007b5f30;
void (*debug_cppsr_ref)(cp_parser&) = (void(*)(cp_parser&)) 0x00000000007b5f30;
void (*debug_cpptk_ref)(cp_token&) = (void(*)(cp_token&)) 0x00000000007b6000;
void (*debug_cpptk_ptr)(cp_token*) = (void(*)(cp_token*)) 0x00000000007b6030;
void debug(cp_parser* p) { debug_cppsr_ptr(p); }
void debug(cp_parser& p) { debug_cppsr_ref(p); }
void debug(cp_token& t) { debug_cpptk_ref(t); }
void debug(cp_token* t) { debug_cpptk_ptr(t); }

#endif //XFW_PCH_HPP

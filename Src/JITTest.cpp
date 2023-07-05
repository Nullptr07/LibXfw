#include <libgccjit++.h>
#include <subhook/subhook.h>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <iostream>
#include <fstream>
#include "polyhook2/Detour/x64Detour.hpp"
#include "polyhook2/ZydisDisassembler.hpp"
#include "polyhook2/PolyHookOsIncludes.hpp"

using namespace gccjit;

static void
create_code (gccjit::context ctxt)
{
    /* Let's try to inject the equivalent of this C code:
       void
       greet (const char *name)
       {
          printf ("hello %s\n", name);
       }
    */
    gccjit::type void_type = ctxt.get_type (GCC_JIT_TYPE_VOID);
    gccjit::type const_char_ptr_type =
            ctxt.get_type (GCC_JIT_TYPE_CONST_CHAR_PTR);
    gccjit::param param_name =
            ctxt.new_param (const_char_ptr_type, "name");
    std::vector<gccjit::param> func_params;
    func_params.push_back (param_name);
    gccjit::function func =
            ctxt.new_function (GCC_JIT_FUNCTION_EXPORTED,
                               void_type,
                               "greet",
                               func_params, 0);

    gccjit::param param_format =
            ctxt.new_param (const_char_ptr_type, "format");
    std::vector<gccjit::param> printf_params;
    printf_params.push_back (param_format);
    gccjit::function printf_func =
            ctxt.new_function (GCC_JIT_FUNCTION_IMPORTED,
                               ctxt.get_type (GCC_JIT_TYPE_INT),
                               "printf",
                               printf_params, 1);

    gccjit::block block = func.new_block ();
    block.add_eval (ctxt.new_call (printf_func,
                                   ctxt.new_rvalue ("hello %s\n"),
                                   param_name));
    block.end_with_return ();
}

template <class T1, class T2>
T1 union_cast(T2 v)
{
    static_assert(sizeof(T1) >= sizeof(T2), "Bad union_cast!");
    union UT {T1 t1; T2 t2;} u {};
    u.t2 = v;
    return u.t1;
}

uint64_t tramp_var;

void hooked_fn(context* _this, const char* path) {
    std::cout << "[-] Called hooked_fn!" << std::endl;
    auto ptr = union_cast<decltype(&context::dump_reproducer_to_file)>(tramp_var);
    (_this->*ptr)(path);

    std::ifstream f(path);
    if (f.is_open())
        std::cout << f.rdbuf();
}

int main() {
    context ctxt = context::acquire();
    ctxt.set_bool_option(GCC_JIT_BOOL_OPTION_DUMP_SUMMARY, 1);
    ctxt.set_logfile(stderr, 0, 0);
    //ctxt.add_command_line_option("-x c++");

    PLH::ZydisDisassembler disas(PLH::Mode::x64);
    auto memptr = &context::dump_reproducer_to_file;
    void* ptr = (void*&) memptr;
    PLH::x64Detour detour((uint64_t) ptr, reinterpret_cast<uint64_t>(&hooked_fn), &tramp_var);

    create_code(ctxt);
    ctxt.compile();
    ctxt.release();

    //subhook::Hook hookDump(union_cast<void*>(ptr), reinterpret_cast<void *>(hooked_fn), subhook::HookFlags::HookFlag64BitOffset);
}
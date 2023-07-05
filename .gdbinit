python
import os
run_dbg_gcc = os.getenv("RUN_DEBUG_GCC")

if run_dbg_gcc:
    gdb.execute("file /home/nullptr07/gcc/gcc-build/bin/g++")
    gdb.execute("symbol-file /home/nullptr07/gcc/gcc-build/gcc/cc1plus")
else:
    gdb.execute("file g++-13")
    gdb.execute("add-symbol-file /usr/libexec/gcc/x86_64-linux-gnu/13/cc1plus")

gdb.execute("add-symbol-file /home/nullptr07/Xfw/Build/Plugin.so")
end

dir /home/nullptr07/gcc/gcc-build/../gcc-src/gcc
dir /home/nullptr07/gcc/gcc-build/../gcc-src/gcc/ada
dir /home/nullptr07/gcc/gcc-build/../gcc-src/gcc/c
dir /home/nullptr07/gcc/gcc-build/../gcc-src/gcc/cp
dir /home/nullptr07/gcc/gcc-build/../gcc-src/gcc/d
dir /home/nullptr07/gcc/gcc-build/../gcc-src/gcc/fortran
dir /home/nullptr07/gcc/gcc-build/../gcc-src/gcc/go
dir /home/nullptr07/gcc/gcc-build/../gcc-src/gcc/jit
dir /home/nullptr07/gcc/gcc-build/../gcc-src/gcc/lto
dir /home/nullptr07/gcc/gcc-build/../gcc-src/gcc/objc
dir /home/nullptr07/gcc/gcc-build/../gcc-src/gcc/objcp

python
import os
run_dbg_gcc = os.getenv("RUN_DEBUG_GCC")
if run_dbg_gcc:
    gdb.execute("source /home/nullptr07/gcc/gcc-build/../gcc-src/gcc/gdbinit.in")
    import sys
    sys.path.append('/home/nullptr07/gcc/gcc-build/../gcc-src/gcc')
    import gdbhooks
end

set detach-on-fork off
set follow-fork-mode child
set args -std=c++2b -fplugin=/home/nullptr07/Xfw/Build/Plugin.so -std=c++2b /home/nullptr07/Xfw/Build/Test.cpp -o /home/nullptr07/Xfw/Build/Test.o
delete
#echo ┌────────────────────────────────────────────────────────────────┐\n
#echo │            ALL BREAKPOINTS CLEARED, HAPPY DEBUGGING!           │\n
#echo └────────────────────────────────────────────────────────────────┘\n

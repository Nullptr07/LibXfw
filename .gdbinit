file /home/nullptr07/gcc/gcc-build/bin/g++
symbol-file /home/nullptr07/gcc/gcc-build/gcc/cc1plus
add-symbol-file /home/nullptr07/Xfw/Build/Plugin.so

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
source /home/nullptr07/gcc/gcc-build/../gcc-src/gcc/gdbinit.in
python import sys; sys.path.append('/home/nullptr07/gcc/gcc-build/../gcc-src/gcc'); import gdbhooks
set detach-on-fork off
set follow-fork-mode child
set args -std=c++2b -fplugin=/home/nullptr07/Xfw/Build/Plugin.so -std=c++2b /home/nullptr07/Xfw/Build/Test.cpp -o /home/nullptr07/Xfw/Build/Test.o
delete
#echo ┌────────────────────────────────────────────────────────────────┐\n
#echo │            ALL BREAKPOINTS CLEARED, HAPPY DEBUGGING!           │\n
#echo └────────────────────────────────────────────────────────────────┘\n

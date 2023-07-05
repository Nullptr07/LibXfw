CXX := g++-12
LD := g++-12
CXXFLAGS := \
	-std=c++2b -O3 -no-pie -fno-PIE -fPIC -ffast-math -flto=8 -Winvalid-pch \
	-I /usr/lib/gcc/x86_64-linux-gnu/13/plugin/include \
	-I /home/nullptr07/Xfw/Deps \
	-lfmt -I/home/nullptr07/fmt/include/
DEBUG_FLAGS := \
	-g -O0 -DDEBUG
LDFLAGS := \
	-flto=8
CXX_COMPILATION_TIME_PROFILING_FLAGS := \
	-ftime-report -H -ftime-report-details
THIS_FILE := $(lastword $(MAKEFILE_LIST))

PCH_SRC := \
	Deps/pch.hpp
PCH_DBG_SRC := \
	Deps/pch_dbg.hpp
PCH_OUT := \
	Deps/pch.gch
PCH_OUT_DBG := \
	Deps/pch_dbg.gch
MAIN_SRC := \
	Src/Plugin.cxx

all: $(PCH_OUT)
	$(CXX) $(CXXFLAGS) $(patsubst %, -include %, $(PCH_SRC)) -c $(MAIN_SRC) -o ./Build/Plugin.o
	$(LD) $(LDFLAGS) -shared Build/Plugin.o -lfmt -Wl,-lfmt -o Build/Plugin.so -flto -Wl,-flto

.PHONY: jit_test
jit_test:
	$(CXX) $(CXXFLAGS) \
		-I /usr/lib/gcc/x86_64-linux-gnu/13/include/ \
		Src/JITTest.cpp -c -o Build/JITTest.o \
		-I/home/nullptr07/Xfw/Deps/PolyHook_2_0/_install/include

	$(LD) $(LDFLAGS) \
		-L/usr/lib/gcc/x86_64-linux-gnu/13 \
		-L/home/nullptr07/Xfw/Deps/PolyHook_2_0/_install/lib \
		-L/lib/x86_64-linux-gnu/ \
		Build/JITTest.o \
		-lasmtk -lasmjit  -lZycore -lZydis -lPolyHook_2\
		-o Build/JITTest2.o \
		-Wl,-Bdynamic -lgccjit

.PHONY: jit_test_dbg
jit_test_dbg:
	$(CXX) $(CXXFLAGS) \
		-O0 -g3 \
		-I /usr/lib/gcc/x86_64-linux-gnu/13/include/ \
		Src/JITTest.cpp -c -o Build/JITTest.o \
		-I/home/nullptr07/Xfw/Deps/PolyHook_2_0/_install/include

	$(LD) $(LDFLAGS) \
		-L/usr/lib/gcc/x86_64-linux-gnu/13 \
		-L/home/nullptr07/Xfw/Deps/PolyHook_2_0/_install/lib \
		-L/lib/x86_64-linux-gnu/ \
		Build/JITTest.o \
		-lasmtk -lasmjit  -lZycore -lZydis -lPolyHook_2\
		-o Build/JITTest2.o \
		-Wl,-Bdynamic -lgccjit

$(PCH_OUT): $(PCH_SRC)
	$(CXX) $(CXXFLAGS) $(patsubst %, -include %, $(PCH_SRC))  -x c++-header -c $< -o	$@ -flto -Wl,-flto
$(PCH_OUT_DBG): $(PCH_DBG_SRC)
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) $(patsubst %, -include %, $(PCH_DBG_SRC))  -x c++-header -c $< -o	$@ -flto -Wl,-flto

.PHONY: build_dbg
build_dbg: $(PCH_OUT_DBG)
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) $(patsubst %, -include %, $(PCH_DBG_SRC)) -c $(MAIN_SRC) -o ./Build/Plugin.o
	$(LD) $(LDFLAGS) -shared Build/Plugin.o -lfmt -Wl,-lfmt -o Build/Plugin.so -flto -Wl,-flto

.PHONY: compilation_time_profiling
compilation_time_profiling:
	$(CXX) $(CXXFLAGS) $(CXX_COMPILATION_TIME_PROFILING_FLAGS) $(patsubst %, -include %, $(PCH_SRC)) -c $(MAIN_SRC) -o ./Build/Plugin.o
	$(LD) $(LDFLAGS) -shared Build/Plugin.o -o Build/Plugin.so

.PHONY: run
run:
	@g++-12 ./Build/Test.cpp -o ./Build/Test -fplugin=./Build/Plugin.so -lfmt -I/home/nullptr07/fmt/include/ -std=c++2b -Wall -rdynamic #-fsanitize=address #-fsanitize=leak
.PHONY: run_dbg
run_dbg:
	@~/gcc/gcc-build/bin/g++ ./Build/Test.cpp -o ./Build/Test -fplugin=./Build/Plugin.so -std=c++2b -Wall -lfmt -I/home/nullptr07/fmt/include/
.PHONY: clean
clean:
	@rm -rf Build/Plugin.o Build/Plugin.so

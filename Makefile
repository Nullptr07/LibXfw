CXX := ccache g++-12
LD := g++-12
CXXFLAGS := \
		-std=c++2b -O3 -no-pie -fno-PIE -fPIC -ffast-math -flto=8 -Winvalid-pch \
	-I /usr/lib/gcc/x86_64-linux-gnu/13/plugin/include \
	-I /home/nullptr07/Xfw/Deps
DEBUG_FLAGS := \
	-g -O0 -DDEBUG
LDFLAGS := \
	-fuse-ld=mold -flto=8
CXX_COMPILATION_TIME_PROFILING_FLAGS := \
	-ftime-report -H -ftime-report-details
BUILD_REFACTORED := \
	false

ifeq ($(BUILD_REFACTORED), false)
	PCH_SRC := \
		Deps/pch.hpp
	PCH_OUT := \
		Deps/pch.hpp.gch
	MAIN_SRC := \
		Src/Plugin.cxx
else
	PCH_SRC := \
		Deps/pch_without_inja.hpp
	PCH_OUT := \
		Deps/pch_without_inja.hpp.gch
	MAIN_SRC := \
		Src/PluginRefactored.cxx
endif

all: $(PCH_OUT)
	$(CXX) $(CXXFLAGS) $(patsubst %, -include %, $(PCH_SRC)) -c $(MAIN_SRC) -o ./Build/Plugin.o
	$(LD) $(LDFLAGS) -shared Build/Plugin.o -o Build/Plugin.so
$(PCH_OUT): $(PCH_SRC)
	$(CXX) $(CXXFLAGS) $(patsubst %, -include %, $(PCH_SRC))  -x c++-header -c $< -o	$@

.PHONY: build_dbg
build_dbg: $(PCH_OUT)
	$(CXX) $(CXXFLAGS) $(DEBUG_FLAGS) $(patsubst %, -include %, $(PCH_SRC)) -c $(MAIN_SRC) -o ./Build/Plugin.o
	$(LD) $(LDFLAGS) -shared Build/Plugin.o -o Build/Plugin.so

.PHONY: compilation_time_profiling
compilation_time_profiling: $(PCH_OUT)
	$(CXX) $(CXXFLAGS) $(CXX_COMPILATION_TIME_PROFILING_FLAGS) $(patsubst %, -include %, $(PCH_SRC)) -c $(MAIN_SRC) -o ./Build/Plugin.o
	$(LD) $(LDFLAGS) -shared Build/Plugin.o -o Build/Plugin.so

.PHONY: run
run:
	@g++-12 ./Build/Test.cpp -o ./Build/Test -fplugin=./Build/Plugin.so -std=c++2b
.PHONY: run_dbg
run_dbg:
	@~/gcc/gcc-debug-build/bin/g++ ./Build/Test.cpp -o ./Build/Test -fplugin=./Build/Plugin.so -std=c++2b
.PHONY: clean
clean:
	@rm -rf Build/Plugin.o Build/Plugin.so

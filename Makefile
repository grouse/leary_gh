mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))

ROOT  = ${CURDIR}

BUILD = $(ROOT)/build

.DEFAULT_GOAL := all

SPV_SRC = $(ROOT)/src/shaders
SPV_DST = $(BUILD)/data/shaders
SPV_VERT = $(addprefix $(SPV_DST)/,$(addsuffix .vert.spv,generic font basic2d mesh terrain))
SPV_FRAG = $(addprefix $(SPV_DST)/,$(addsuffix .frag.spv,generic font basic2d mesh terrain))

$(SPV_DST)/%.vert.spv: $(SPV_SRC)/%.glsl
	glslangValidator -V -g -S vert -DVERTEX_SHADER $< -o $@

$(SPV_DST)/%.frag.spv: $(SPV_SRC)/%.glsl
	glslangValidator -V -g -S frag -DFRAGMENT_SHADER $< -o $@

shaders: | $(SPV_DST) $(SPV_VERT) $(SPV_FRAG)

FLAGS = -std=c++14 -g

NOWARNINGS = -Wno-int-to-void-pointer-cast -Wno-nested-anon-types -Wno-unused-function
WARNINGS   = -Wall -Wextra -Wpedantic $(NOWARNINGS)

INCLUDE_DIR = -I$(ROOT)/src -I$(ROOT)

OPTIMIZED   = -O3
UNOPTIMIZED = -O0

LEARY_LIBS  = -lvulkan -lX11 -lXi -ldl -lpthread
LEARY_FLAGS = $(FLAGS) $(WARNINGS) $(UNOPTIMIZED) $(INCLUDE_DIR)

$(BUILD)/game.so: FORCE
	$(CXX) $(LEARY_FLAGS) -fPIC -shared $(ROOT)/src/platform/linux_leary.cpp -o $@

$(BUILD)/leary: FORCE
	$(CXX) $(LEARY_FLAGS) $(LEARY_LIBS) -o $@ $(ROOT)/src/platform/linux_main.cpp

leary: $(BUILD)/game.so $(BUILD)/leary


TOOLS_FLAGS = $(FLAGS) $(WARNINGS) $(UNOPTIMIZED) $(INCLUDE_DIR)
$(BUILD)/preprocessor: FORCE
	$(CXX) $(TOOLS_FLAGS) $(ROOT)/tools/preprocessor.cpp -o $@

$(BUILD)/benchmark: FORCE
	$(CXX) $(TOOLS_FLAGS) -O2 $(ROOT)/tools/benchmark.cpp -o $@

tools: $(BUILD)/preprocessor

TESTS_FLAGS = $(FLAGS) $(WARNINGS) $(UNOPTIMIZED) $(INCLUDE_DIR)
$(BUILD)/tests: FORCE
	$(CXX) $(TOOLS_FLAGS) $(ROOT)/tests/main.cpp -o $@
tests: $(BUILD)/tests

BENCHMARKS_FLAGS = $(FLAGS) $(WARNINGS) $(UNOPTIMIZED) $(INCLUDE_DIR)
$(BUILD)/benchmarks: FORCE
	$(CXX) $(TOOLS_FLAGS) $(ROOT)/benchmarks/main.cpp -o $@
benchmarks: $(BUILD)/benchmarks

all: shaders tools leary tests benchmarks

$(SPV_DST):
	mkdir -p $(SPV_DST)

FORCE:

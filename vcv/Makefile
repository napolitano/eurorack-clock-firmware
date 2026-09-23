# South Signal Lab CLOCK VCV Rack plugin build.
# Author: Axel Napolitano
# License: PolyForm-Noncommercial-1.0.0

RACK_DIR ?= ../Rack-SDK

EXTRA_FLAGS += -DCLOCK_HOST_TEST=1 -DCLOCK_SIMULATOR=1
EXTRA_FLAGS += -I../src -I../lib/clock_core/src -I../sim -I../sim/framework -I.
EXTRA_FLAGS += -fno-unsafe-math-optimizations
EXTRA_CXXFLAGS += -std=c++17

SOURCES += $(wildcard src/*.cpp)
SOURCES += clock_vcv_runtime.cpp

CLOCK_FW_SOURCES := $(shell find ../src ../lib/clock_core/src -type f -name '*.cpp' ! -name 'main.cpp' ! -name 'native_smoke_main.cpp')
CLOCK_HOST_SOURCES := \
	../sim/simulator_runtime.cpp \
	../sim/simulator_runtime_inputs.cpp \
	../sim/simulator_persistence.cpp \
	../sim/virtual_input_signal.cpp
CLOCK_EXTERNAL_SOURCES := $(CLOCK_FW_SOURCES) $(CLOCK_HOST_SOURCES)
CLOCK_EXTERNAL_OBJECTS := $(patsubst ../%.cpp,build/clockfw/%.o,$(CLOCK_EXTERNAL_SOURCES))
OBJECTS += $(CLOCK_EXTERNAL_OBJECTS)

DISTRIBUTABLES += res
DISTRIBUTABLES += README.md
DISTRIBUTABLES += LICENSE.md

include $(RACK_DIR)/plugin.mk

build/clockfw/%.o: ../%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

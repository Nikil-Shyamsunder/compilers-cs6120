CXX      ?= g++
CXXFLAGS ?= -std=gnu++17 -O2 -Wall -Wextra -Wno-unused-parameter
INC      ?= -I$(shell brew --prefix)/include

SRC_COMMON = common.cpp
HDR_COMMON = common.hpp

all: lvn tdce reaching_definitions dominator_util

lvn: lvn.cpp $(SRC_COMMON) $(HDR_COMMON)
	$(CXX) $(CXXFLAGS) $(INC) lvn.cpp $(SRC_COMMON) -o build/lvn

tdce: tdce.cpp $(SRC_COMMON) $(HDR_COMMON)
	$(CXX) $(CXXFLAGS) $(INC) tdce.cpp $(SRC_COMMON) -o build/tdce

reaching_definitions: reaching_definitions.cpp $(SRC_COMMON) $(HDR_COMMON)
	$(CXX) $(CXXFLAGS) $(INC) reaching_definitions.cpp $(SRC_COMMON) -o build/reaching_definitions

dominator_util: dominator_util.cpp $(SRC_COMMON) $(HDR_COMMON)
	$(CXX) $(CXXFLAGS) $(INC) dominator_util.cpp $(SRC_COMMON) -o build/dominator_util

clean:
	rm -f build/*
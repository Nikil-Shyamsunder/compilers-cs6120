CXX      ?= g++
CXXFLAGS ?= -std=gnu++17 -O2 -Wall -Wextra -Wno-unused-parameter
INC      ?= -I$(shell brew --prefix)/include

SRC_COMMON = common.cpp
HDR_COMMON = common.hpp

all: lvn tdce

lvn: lvn.cpp $(SRC_COMMON) $(HDR_COMMON)
	$(CXX) $(CXXFLAGS) $(INC) lvn.cpp $(SRC_COMMON) -o build/lvn

tdce: tdce.cpp $(SRC_COMMON) $(HDR_COMMON)
	$(CXX) $(CXXFLAGS) $(INC) tdce.cpp $(SRC_COMMON) -o build/tdce

clean:
	rm -f build/lvn build/tdce

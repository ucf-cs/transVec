# How to print, for debug purposes
#$(info L_ATOMIC="$(L_ATOMIC)")

# Default compiler
CC = g++-7

MACHINE_TYPE=$(shell uname -m)

L_ATOMIC =
DATA_STRUCTURE =

ifeq ($(DATA_STRUCTURE),)
	DS =
	ifeq ($(shell head define.hpp | grep "^\#define COMPACTVEC" | wc -l), 1)
		L_ATOMIC=-latomic
	endif
else
	DS = |$(DATA_STRUCTURE)
	ifeq ($(DATA_STRUCTURE), COMPACTVEC)
		L_ATOMIC=-latomic
	endif
endif

ifeq ($(MACHINE_TYPE),"aarch64")
	L_ATOMIC=-latomic
endif

INCLUDESTO = -Isto/lib -Isto/sto-core -Isto/legacy -Isto/datatype -Isto/benchmark -Isto -Isto/masstree-beta -L/sto/obj
STDFLAGS = -std=c++14 -pthread -fgnu-tm -Wall -Wextra
DEBUG_FLAGS = -g 
OPTIMAL_FLAGS = -march=native -Ofast -flto

ifeq ($(MACHINE_TYPE),aarch64)
	L_ATOMIC=-latomic
else
	STDFLAGS+=-mcx16
endif

CC_FLAGS =  $(OPTIMAL_FLAGS) $(INCLUDESTO) $(STDFLAGS)

# List vectorization at compile time
#-fopt-info-vec-missed
#-fopt-info

# Use lock cmpxchg16b instruction directly on x86-64
#-mcx16

# Link libatomic for ARM and other platforms (Has library call overhead)
#-latomic

# File names
EXEC    = transVec.out
MAIN    = test_cases/testcase05.cpp

SOURCESCPP = $(wildcard *.cpp) test_cases/main.cpp $(MAIN)
SOURCESCC =  $(wildcard sto/sto-core/*.cc) sto/masstree-beta/compiler.cc #$(wildcard sto/masstree-beta/*.cc) #$(wildcard **/*.cc)
#$(foreach d,$(wildcard sto/*),$(call rwildcard,$d/,*.cc) $(filter $(subst *,%,*.cc),$d))
SOURCES = $(SOURCESCPP) $(SOURCESCC)

OBJECTSCPP = $(SOURCESCPP:.cpp=.o)
OBJECTSCC = $(SOURCESCC:.cc=.o)
OBJECTS = $(OBJECTSCPP) $(OBJECTSCC)

DEFINES = 

# Main target
$(EXEC): $(OBJECTS)
	@$(CC) $(CC_FLAGS) $(OBJECTS) -o $(EXEC) $(L_ATOMIC)

sto/sto-core/%.o : sto/sto-core/%.cc
	@$(CC) $< -c $(CC_FLAGS) -o $@

sto/masstree-beta/%.o : sto/masstree-beta/%.cc
	@$(CC) $< -c $(CC_FLAGS) -o $@

# To obtain object files
%.o: %.cpp
	@$(CC) -c $(CC_FLAGS) $(subst |, -D ,$(DS)) $(subst |, -D ,$(DEFINES)) $< -o $@ 

# To remove generated files
clean:
	@rm -f $(EXEC) $(OBJECTS) $(wildcard test_cases/*.o)

# Clean the reports directory
cr:
	@rm -f reports/*.txt
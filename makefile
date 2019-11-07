# Default compiler
SGMTVEC = g++-7
# Used to build CompactVector
CMPTVEC = g++-5
# Used to build GCC STM vector
STMVEC  = g++-7
DATA_STRUCTURE =

# Determine which version of GCC to use.
# If DATA_STRUCTURE is not defined, just go ahead and figure out which g++ to
# use using grep. Else pick the g++ depending on what the value of DATA_STRUCUTURE is
ifeq ($(DATA_STRUCTURE),)
	DS =
	ifeq ($(shell head define.hpp | grep "^\#define COMPACTVEC" | wc -l), 1)
		CC = $(CMPTVEC)
	else
		CC = $(STMVEC)
	endif
else
	DS = |$(DATA_STRUCTURE)
	ifeq ($(DATA_STRUCTURE), COMPACTVEC)
		CC = $(CMPTVEC)
	else
		CC = $(STMVEC)
	endif
endif

INCLUDESTO = -Isto/lib -Isto/sto-core -Isto/legacy -Isto/datatype -Isto/benchmark -Isto -Isto/masstree-beta -L/sto/obj
# remove mcx16 on ARM
STDFLAGS = -std=c++14 -pthread -mcx16 -fgnu-tm -Wall -Wextra
DEBUG_FLAGS = -g 
OPTIMAL_FLAGS = -march=native -Ofast -flto
CC_FLAGS =  $(OPTIMAL_FLAGS) $(INCLUDESTO) $(STDFLAGS)

# TODO: Make this actually work.
# # Add these flags for STO
# ifeq ($(shell head define.hpp | grep "^\#define STOVEC" | wc -l), 1)
# 	CC_FLAGS += ${INCLUDESTO)
# endif

# List vectorization at compile time
#-fopt-info-vec-missed
#-fopt-info

# Use lock cmpxchg16b instruction on x86-64
#-mcx16

# Link libatomic for ARM and other platforms (Has library call overhead)
#-latomic

# File names
EXEC    = transVec.out
MAIN    = test_cases/testcase01.cpp

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
	@$(CC) $(CC_FLAGS) $(OBJECTS) -o $(EXEC)
#add -latomic on ARM

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
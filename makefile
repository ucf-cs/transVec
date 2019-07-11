# Default compiler
SGMTVEC = g++
# Used to build CompactVector
CMPTVEC = g++-5
# Used to build GCC STM vector
STMVEC  = g++-7
DATA_STRUCTURE =

# If DATA_STRUCTURE is not defined, just go ahead and figure out which g++ to
# use using grep. Else pick the g++ depending on what the value of DATA_STRUCUTURE is
ifeq ($(DATA_STRUCTURE),)
	DS =
	ifeq ($(shell head define.hpp | grep "^\#define SEGMENTVEC" | wc -l), 1)
		CC = $(SGMTVEC)
	else ifeq ($(shell head define.hpp | grep "^\#define COMPACTVEC" | wc -l), 1)
		CC = $(CMPTVEC)
	else
		CC = $(STMVEC)
	endif
else
	DS = |$(DATA_STRUCTURE)
	ifeq ($(DATA_STRUCTURE), SEGMENTVEC)
		CC = $(SGMTVEC)
	else ifeq ($(DATA_STRUCTURE), COMPACTVEC)
		CC = $(CMPTVEC)
	else
		CC = $(STMVEC)
	endif
endif

DEBUG_FLAGS = -std=c++14 -g -pthread -fgnu-tm -mcx16
OPTIMAL_FLAGS = -std=c++14 -march=native -Ofast -pthread -fgnu-tm  -mcx16
CC_FLAGS = $(DEBUG_FLAGS)

# List vectorization at compile time
#-fopt-info-vec-missed
#-fopt-info

# Use lock cmpxchg16b instruction on x86-64
#-mcx16

# Link libatomic for ARM and other platforms (Has library call overhead)
#-latomic

# File names
EXEC    = transVec.out
MAIN    = test_cases/testcase19.cpp
SOURCES = $(wildcard *.cpp) test_cases/main.cpp $(MAIN)
OBJECTS = $(SOURCES:.cpp=.o)
DEFINES = 

# Main target
$(EXEC): $(OBJECTS)
	@$(CC) $(CC_FLAGS) $(OBJECTS) -o $(EXEC)

# To obtain object files
%.o: %.cpp
	@$(CC) -c $(CC_FLAGS) $(subst |, -D ,$(DS)) $(subst |, -D ,$(DEFINES)) -flto $< -o $@ 

# To remove generated files
clean:
	@rm -f $(EXEC) $(OBJECTS) $(wildcard test_cases/*.o)

# Clean the reports directory
cr:
	@rm -f reports/*.txt
# Default compiler
CC = g++
# Used to build CompactVector
# CC = g++-5
# Used to build GCC STM vector
# CC = g++-7

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
MAIN    = test_cases/testcase11.cpp
SOURCES = $(wildcard *.cpp) test_cases/main.cpp $(MAIN)
OBJECTS = $(SOURCES:.cpp=.o)
DEFINES = 

# Main target
$(EXEC): $(OBJECTS)
	@$(CC) $(CC_FLAGS) $(OBJECTS) -o $(EXEC)

# To obtain object files
%.o: %.cpp
	@$(CC) -c $(CC_FLAGS) $(subst |, -D ,$(DEFINES)) -flto $< -o $@ 

# To remove generated files
clean:
	@rm -f $(EXEC) $(OBJECTS) $(wildcard test_cases/*.o)

# Clean the reports directory
cr:
	@rm -f reports/*.txt
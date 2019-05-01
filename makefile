# Declaration of variables
CC = g++

DEBUG_FLAGS = -std=c++11 -g -pthread -fgnu-tm
OPTIMAL_FLAGS = -std=c++11 -march=native -Ofast -pthread -fgnu-tm
#-fopt-info-vec-missed
#-fopt-info
CC_FLAGS = $(DEBUG_FLAGS)

# File names
EXEC = transVec.out
SOURCES = $(wildcard *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)

# Main target
$(EXEC): $(OBJECTS)
	$(CC) $(CC_FLAGS) $(OBJECTS) -o $(EXEC)

# To obtain object files
%.o: %.cpp
	$(CC) -c $(CC_FLAGS) -flto $< -o $@

# To remove generated files
clean:
	rm -f $(EXEC) $(OBJECTS)
# default:
# main:
# deltaPage:
# helpScheme:
# rwSet:
# segmentedVector:
# transaction:
# transVector:


# Declaration of variables
CC = g++
CC_FLAGS = -std=c++11 -g -pthread
OPTIMAL_FLAGS = -std=c++11 -march=native -Ofast -pthread
# Used to prevent crashing with dynamic allocation, for some unknown reason.
TEST_FLAGS = -std=c++11 -march=native -O2 -pthread
 
# File names
EXEC = transVec.out
SOURCES = $(wildcard *.cpp)
OBJECTS = $(SOURCES:.cpp=.o)
 
# Main target
$(EXEC): $(OBJECTS)
	$(CC) $(OPTIMAL_FLAGS) $(OBJECTS) -o $(EXEC)
 
# To obtain object files
%.o: %.cpp
	$(CC) -c $(OPTIMAL_FLAGS) $< -o $@
 
# To remove generated files
clean:
	rm -f $(EXEC) $(OBJECTS)
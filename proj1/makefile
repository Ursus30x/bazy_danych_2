CXX = g++
CXXFLAGS = -Wall -Wextra -O2 -std=c++17
TARGET = tape_demo
SRC = main.cpp tapeSort.cpp tape.cpp logger.cpp recordType.cpp
OBJ = $(SRC:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

CXX = g++
CXXFLAGS = -std=c++17 -Wall -O3
LDFLAGS = -ltinyxml2 -lzip -lz

SRCS = $(wildcard *.cpp)
OBJS = $(SRCS:%.cpp=obj/%.o)
TARGET = rom-verify

$(shell mkdir -p obj)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

obj/%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

clean:
	rm -rf obj $(TARGET)

run: $(TARGET)
	./$(TARGET) $(rompath)

.PHONY: all clean run

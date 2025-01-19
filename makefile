CXX = g++
CXXFLAGS = -std=c++17 -Wall -O3
LDFLAGS = -ltinyxml2 -lyaml-cpp -lz -lzip

SRCS = $(wildcard src/*.cpp)
OBJS = $(SRCS:src/%.cpp=obj/%.o)
TARGET = rom-verify

$(shell mkdir -p obj)

-include $(OBJS:.o=.d)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

obj/%.d: src/%.cpp
	@$(CXX) $(CXXFLAGS) -MM -MT $(@:.d=.o) $< > $@

obj/%.o: src/%.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@
	@$(CXX) $(CXXFLAGS) -MM -MT $@ $< > $(patsubst %.o,%.d,$@)

clean:
	rm -rf obj $(TARGET) $(OBJS:.o=.d)

run: $(TARGET)
	./$(TARGET) $(rompath)

.PHONY: all clean run

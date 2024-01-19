
CXX = g++
MAX_VERTS=1024
CXXFLAGS_NOOPT = -DOVERRIDE_MAX_VERTS=$(MAX_VERTS) -I. -std=c++20 -flto
CXXFLAGS = -O3 $(CXXFLAGS_NOOPT) 
BUILD_DIR=./build_obj
SRC_DIR=./src

SOURCE_FILES = $(shell ls -1 $(SRC_DIR)/*.cpp)
O_FILES = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SOURCE_FILES))

all: unidom 

unidom: $(BUILD_DIR) $(O_FILES)
	$(CXX) $(CXXFLAGS) -o $@ $(O_FILES)
	
$(BUILD_DIR):
	mkdir $(BUILD_DIR)
	
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ -c $<
	
debug_compile:
	$(CXX) $(CXXFLAGS_NOOPT) -g -o unidom *.cpp

.phony: all clean slow_compile debug_compile

clean:
	rm -rf $(BUILD_DIR)
	rm -f unidom

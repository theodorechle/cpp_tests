CPP_C=g++
CPP_FLAGS=-std=c++17 -Wall -g -MMD -MP
BIN_DIR=bin
OBJ_DIR=obj
SRC_DIR=src
LIB=bin/cpp_tests_lib

# Source files
SRC_TESTS=$(wildcard $(SRC_DIR)/*.cpp)

# Object files
OBJ_TESTS=$(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRC_TESTS))

.PHONY: lib

# Build the final executable combining exe and style
lib: $(LIB).a

$(LIB).a: $(OBJ_TESTS)
	@mkdir -p $(BIN_DIR)
	ar -r $@ $^

# Rule for compiling all object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CPP_C) $(CPP_FLAGS) -c $< -o $@

# Clean all generated files
clean:
	@find obj -mindepth 1 ! -name .gitkeep -delete
	@find bin -mindepth 1 ! -name .gitkeep -delete

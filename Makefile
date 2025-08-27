# Compiler and flags
CC := gcc
CFLAGS := -Wall -Iinclude
LDFLAGS := -lm -lraylib -lGL -lpthread -ldl -lrt -lX11   # for Linux/X11

# Directories
SRC_DIR := src
BUILD_DIR := build
BIN := $(BUILD_DIR)/app

# Source and object files
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC))

# Default target
build: $(BIN)

$(BIN): $(OBJ)
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile each .c into .o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Run target
run: build
	./$(BIN)

# Clean target
clean:
	rm -rf $(BUILD_DIR)

.PHONY: build run clean

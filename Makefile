CC = gcc

CFLAGS_COMMON = -std=c11 -pedantic -W -Wall -Wextra
CFLAGS_DEBUG = -O0 -g
CFLAGS_RELEASE = -O2

SRC_DIR = src
BUILD_DIR = build
TARGET = dirwalk

SRC = $(SRC_DIR)/dirwalk.c

DEBUG_OBJ = $(BUILD_DIR)/debug/dirwalk.o
RELEASE_OBJ = $(BUILD_DIR)/release/dirwalk.o

.PHONY: all debug release clean distclean run valgrind

all: release

release: $(TARGET)

$(TARGET): $(RELEASE_OBJ)
	$(CC) $(CFLAGS_COMMON) $(CFLAGS_RELEASE) $^ -o $@

$(BUILD_DIR)/release/dirwalk.o: $(SRC)
	mkdir -p $(BUILD_DIR)/release
	$(CC) $(CFLAGS_COMMON) $(CFLAGS_RELEASE) -c $< -o $@

debug: dirwalk_debug

dirwalk_debug: $(DEBUG_OBJ)
	$(CC) $(CFLAGS_COMMON) $(CFLAGS_DEBUG) $^ -o $@

$(BUILD_DIR)/debug/dirwalk.o: $(SRC)
	mkdir -p $(BUILD_DIR)/debug
	$(CC) $(CFLAGS_COMMON) $(CFLAGS_DEBUG) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

valgrind: debug
	valgrind --leak-check=full --show-leak-kinds=all -s ./dirwalk_debug

clean:
	rm -rf $(BUILD_DIR)

distclean: clean
	rm -f $(TARGET) dirwalk_debug

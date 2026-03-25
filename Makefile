CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c99 -g
LDFLAGS =

SRC_DIR = src src/core src/ui src/input src/commands src/utils
INC_DIR = include
BUILD_DIR = build
BIN_DIR = bin

TARGET = $(BIN_DIR)/vedit

SRCS = $(foreach dir, $(SRC_DIR), $(wildcard $(dir)/*.c))
OBJS = $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRCS))

INC_FLAGS = -I$(INC_DIR)

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC_FLAGS) -c $< -o $@

debug: CFLAGS += -O0
debug: clean all

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)

.PHONY: all clean debug

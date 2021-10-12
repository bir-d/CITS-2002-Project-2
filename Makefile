CC = cc
INCLUDE_DIR = include
CCFLAGS = -std=c11 -Wall -Werror -pedantic -D_POSIX_C_SOURCE=200809L -I$(INCLUDE_DIR)
SHELL = /bin/sh
OBJ_DIR = build

SRCS := $(wildcard ./*.c)
OBJS := $(patsubst ./%.c,$(OBJ_DIR)/%.o,$(SRCS))

$(OBJ_DIR)/%.o: %.c
	mkdir -p $(OBJ_DIR)
	$(CC) -c -o $@ $< $(CCFLAGS)

duplicates: $(OBJS)
	$(CC) -o $@ $^ $(CCFLAGS)

debug: CCFLAGS += -g -DDEBUG
debug: duplicates

clean:
	rm -rf $(OBJ_DIR)/ duplicates

.PHONY: clean

# 简单 Makefile: 编译红黑树测试程序
# 用法:
#   make        - 编译生成 build/rbtree_test
#   make run    - 编译并运行测试
#   make clean  - 清理 build 目录

CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -O2

BUILD   := build
TARGET  := $(BUILD)/rbtree_test
SRCS    := main.c rbtree.c
OBJS    := $(SRCS:%.c=$(BUILD)/%.o)

all: $(TARGET)

$(BUILD):
	mkdir -p $@

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# 目标文件和头文件变化时都重新编译
$(BUILD)/%.o: %.c rbtree.h | $(BUILD)
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD)

.PHONY: all run clean

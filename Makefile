# 简单 Makefile: 编译红黑树测试程序
# 用法:
#   make        - 编译生成 rbtree_test
#   make run    - 编译并运行测试
#   make clean  - 清理生成文件

CC      ?= gcc
CFLAGS  ?= -Wall -Wextra -O2

TARGET  := rbtree_test
SRCS    := main.c rbtree.c
OBJS    := $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

# 头文件变化时也重新编译
%.o: %.c rbtree.h
	$(CC) $(CFLAGS) -c $< -o $@

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) $(OBJS)

.PHONY: all run clean

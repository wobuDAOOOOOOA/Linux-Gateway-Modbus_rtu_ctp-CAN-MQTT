# 编译器
CC = gcc

# 目录结构
SRC_DIR = src
INC_DIR = include

# 源文件（在 src/ 目录下找）
SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/can.c $(SRC_DIR)/modbus_tcp.c $(SRC_DIR)/modbus_rtu.c

# 目标文件名（debug 和 release 用不同名字）
TARGET_DEBUG = gateway_debug
TARGET_RELEASE = gateway

# 编译选项
DEBUG_CFLAGS = -Wall -g -O0 -DDEBUG
RELEASE_CFLAGS = -Wall -O2 -DNDEBUG -s

# 头文件路径
CFLAGS += -I$(INC_DIR)

# 链接库
LDLIBS = -lmodbus -lpthread

# debug 版本
debug:
	$(CC) $(DEBUG_CFLAGS) $(CFLAGS) -o $(TARGET_DEBUG) $(SRCS) $(LDLIBS)

# release 版本
release:
	$(CC) $(RELEASE_CFLAGS) $(CFLAGS) -o $(TARGET_RELEASE) $(SRCS) $(LDLIBS)

# 清理
clean:
	rm -f $(TARGET_DEBUG) $(TARGET_RELEASE)

# 防止文件重名冲突
.PHONY: debug release clean
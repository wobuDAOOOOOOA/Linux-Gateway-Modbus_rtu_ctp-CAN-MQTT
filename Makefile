TARGET = gateway
SRCS = can.c modbus_tcp.c modbus_rtu.c main.c
CC = gcc
CFLAGS = -Wall -g
LDLIBS = -lmodbus

DEBUG_CFLAGS = -Wall -g -O0 -DNDEBUG
RELEASE_CFLAGS = -Wall -O2 -DNDEBUG -s
# 编译 debug 版本
debug:
		$(CC) $(DEBUG_CFLAGS) -o $(TARGET)_debug $(SRCS) $(LDLIBS)

# 编译 release 版本
release:
		$(CC) $(RELEASE_CFLAGS) -o $(TARGET) $(SRCS) $(LDLIBS)


$(TARGET): $(SRCS)
		$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDLIBS)

clean:
		rm -f $(TARGET) $(TARGET)_debug
DEPFLAGS = -MMD -MP
CFLAGS += $(DEPFLAGS)

# 自动包含所有 .d 依赖文件
DEPS = $(SRCS:.c=.d)
-include $(DEPS)
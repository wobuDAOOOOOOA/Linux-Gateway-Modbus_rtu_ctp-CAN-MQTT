TARGET = test
SRCS = can.c modbus_tcp.c modbus_rtu.c main.c
CC = gcc
CFLAGS = -Wall -g
LDLIBS = -lmodbus

$(TARGET): $(SRCS)
		$(CC) $(CFLAGS) -o $(TARGET) $(SRCS) $(LDLIBS)

clean:
		rm -f $(TARGET)

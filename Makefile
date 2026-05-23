CC=gcc
CFLAGS=-Wall -Wextra -O2 -Iinclude

TARGET=ebpf-security-agent
SRC=src/main.c src/event_logger.c

all:
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET) events.log

CC=gcc
CFLAGS=-Wall -Wextra -O2 -g -Iinclude

APP=ebpf-security-agent
SRC=src/main.c src/event_logger.c src/config_loader.c

all: $(APP)

$(APP): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

clean:
	rm -f $(APP)
	rm -f events.log
	rm -f build.log

.PHONY: all clean

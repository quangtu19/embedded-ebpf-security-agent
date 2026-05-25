CC=gcc
CLANG=clang

CFLAGS=-Wall -Wextra -O2 -g -Iinclude
BPF_CFLAGS=-O2 -g -target bpf -Wall -Iinclude -I/usr/include/$(shell uname -m)-linux-gnu

APP=ebpf-security-agent

USER_SRC=src/main.c \
         src/event_logger.c \
         src/config_loader.c \
         src/ebpf_loader.c \
         src/ringbuf_reader.c

BPF_OBJ=bpf/exec_trace.bpf.o

LDFLAGS=-lbpf -lelf -lz

all: $(APP) $(BPF_OBJ)

$(APP): $(USER_SRC)
	$(CC) $(CFLAGS) -o $@ $(USER_SRC) $(LDFLAGS)

$(BPF_OBJ): bpf/exec_trace.bpf.c bpf/common.bpf.h
	$(CLANG) $(BPF_CFLAGS) -c bpf/exec_trace.bpf.c -o $(BPF_OBJ)

clean:
	rm -f $(APP)
	rm -f $(BPF_OBJ)
	rm -f events.log
	rm -f build.log

.PHONY: all clean

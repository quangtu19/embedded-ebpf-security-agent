CC=gcc
CLANG=clang

CFLAGS=-Wall -Wextra -O2 -g -Iinclude
BPF_CFLAGS=-O2 -g -target bpf -D__TARGET_ARCH_x86 -Wall -Iinclude -I/usr/include/$(shell uname -m)-linux-gnu

APP=ebpf-security-agent

USER_SRC=src/main.c \
         src/event_logger.c \
         src/config_loader.c \
         src/ebpf_loader.c \
         src/ringbuf_reader.c

BPF_OBJS=bpf/exec_trace.bpf.o bpf/tcp_connect.bpf.o

LDFLAGS=-lbpf -lelf -lz

all: $(APP) $(BPF_OBJS)

$(APP): $(USER_SRC)
	$(CC) $(CFLAGS) -o $@ $(USER_SRC) $(LDFLAGS)

bpf/exec_trace.bpf.o: bpf/exec_trace.bpf.c bpf/common.bpf.h
	$(CLANG) $(BPF_CFLAGS) -c bpf/exec_trace.bpf.c -o bpf/exec_trace.bpf.o

bpf/tcp_connect.bpf.o: bpf/tcp_connect.bpf.c bpf/common.bpf.h
	$(CLANG) $(BPF_CFLAGS) -c bpf/tcp_connect.bpf.c -o bpf/tcp_connect.bpf.o

clean:
	rm -f $(APP)
	rm -f $(BPF_OBJS)
	rm -f events.log
	rm -f build.log

.PHONY: all clean

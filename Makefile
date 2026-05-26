CC ?= gcc
CLANG ?= clang

APP := ebpf-security-agent

APP_SRC := \
	src/main.c \
	src/event_logger.c \
	src/config_loader.c \
	src/xdp_controller.c \
	src/trace_monitor.c

BPF_SRCS := \
	bpf/xdp_filter.bpf.c \
	bpf/exec_trace.bpf.c \
	bpf/tcp_connect.bpf.c

BPF_OBJS := $(BPF_SRCS:.c=.o)

CFLAGS := -Wall -Wextra -O2 -g -Iinclude $(shell pkg-config --cflags libbpf 2>/dev/null)
LDLIBS := $(shell pkg-config --libs libbpf 2>/dev/null || echo -lbpf -lelf -lz)

BPF_CFLAGS := -O2 -g -target bpf \
	-I./bpf \
	-I/usr/include/$(shell uname -m)-linux-gnu

all: $(BPF_OBJS) $(APP)

$(APP): $(APP_SRC) include/*.h
	$(CC) $(CFLAGS) -o $@ $(APP_SRC) $(LDLIBS)

bpf/%.bpf.o: bpf/%.bpf.c bpf/common.bpf.h
	$(CLANG) $(BPF_CFLAGS) -c $< -o $@

clean:
	rm -f $(APP)
	rm -f $(BPF_OBJS)
	rm -f events.log
	rm -f build.log

.PHONY: all clean

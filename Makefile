CC ?= gcc
CLANG ?= clang

APP := ebpf-security-agent
CTL := ebpf-agentctl

APP_SRC := \
	src/main.c \
	src/event_logger.c \
	src/config_loader.c \
	src/xdp_controller.c

CTL_SRC := \
	src/cli.c

BPF_SRC := bpf/xdp_filter.bpf.c
BPF_OBJ := bpf/xdp_filter.bpf.o

CFLAGS := -Wall -Wextra -O2 -g -Iinclude $(shell pkg-config --cflags libbpf 2>/dev/null)
LDLIBS := $(shell pkg-config --libs libbpf 2>/dev/null || echo -lbpf -lelf -lz)

BPF_CFLAGS := -O2 -g -target bpf \
	-I./bpf \
	-I/usr/include/$(shell uname -m)-linux-gnu

all: $(BPF_OBJ) $(APP) $(CTL)

$(APP): $(APP_SRC) include/*.h
	$(CC) $(CFLAGS) -o $@ $(APP_SRC) $(LDLIBS)

$(CTL): $(CTL_SRC) include/*.h
	$(CC) $(CFLAGS) -o $@ $(CTL_SRC) $(LDLIBS)

$(BPF_OBJ): $(BPF_SRC) bpf/common.bpf.h
	$(CLANG) $(BPF_CFLAGS) -c $(BPF_SRC) -o $(BPF_OBJ)

clean:
	rm -f $(APP)
	rm -f $(CTL)
	rm -f $(BPF_OBJ)
	rm -f build.log

.PHONY: all clean

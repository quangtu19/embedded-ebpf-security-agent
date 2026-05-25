#include <stdio.h>
#include <errno.h>
#include <bpf/libbpf.h>

#include "ebpf_loader.h"
#include "ringbuf_reader.h"

struct bpf_module {
    struct bpf_object *obj;
    struct bpf_link *link;
    struct ring_buffer *rb;
};

static struct bpf_module g_exec = {};
static struct bpf_module g_tcp = {};

static int load_module(struct bpf_module *module,
                       const char *object_path,
                       const char *program_name,
                       int is_tcp)
{
    struct bpf_program *prog;
    int events_map_fd;
    int err;

    module->obj = bpf_object__open_file(object_path, NULL);
    if (libbpf_get_error(module->obj)) {
        fprintf(stderr, "Failed to open BPF object: %s\n", object_path);
        module->obj = NULL;
        return -1;
    }

    err = bpf_object__load(module->obj);
    if (err) {
        fprintf(stderr, "Failed to load BPF object: %s\n", object_path);
        return -1;
    }

    prog = bpf_object__find_program_by_name(module->obj, program_name);
    if (prog == NULL) {
        fprintf(stderr, "Failed to find BPF program: %s\n", program_name);
        return -1;
    }

    module->link = bpf_program__attach(prog);
    if (libbpf_get_error(module->link)) {
        fprintf(stderr, "Failed to attach BPF program: %s\n", program_name);
        module->link = NULL;
        return -1;
    }

    events_map_fd = bpf_object__find_map_fd_by_name(module->obj, "events");
    if (events_map_fd < 0) {
        fprintf(stderr, "Failed to find ring buffer map: events in %s\n", object_path);
        return -1;
    }

    if (is_tcp) {
        module->rb = ringbuf_reader_create_tcp(events_map_fd);
    } else {
        module->rb = ringbuf_reader_create_proc(events_map_fd);
    }

    if (module->rb == NULL) {
        fprintf(stderr, "Failed to create ring buffer reader for %s\n", object_path);
        return -1;
    }

    return 0;
}

int ebpf_loader_start(void)
{
    if (load_module(&g_exec, "bpf/exec_trace.bpf.o", "handle_exec", 0) != 0) {
        return -1;
    }

    if (load_module(&g_tcp, "bpf/tcp_connect.bpf.o", "handle_tcp_v4_connect", 1) != 0) {
        return -1;
    }

    return 0;
}

int ebpf_loader_poll(int timeout_ms)
{
    int err;

    if (g_exec.rb != NULL) {
        err = ring_buffer__poll(g_exec.rb, timeout_ms);
        if (err == -EINTR) {
            return 0;
        }
        if (err < 0) {
            fprintf(stderr, "exec ring_buffer__poll failed: %d\n", err);
            return err;
        }
    }

    if (g_tcp.rb != NULL) {
        err = ring_buffer__poll(g_tcp.rb, 0);
        if (err == -EINTR) {
            return 0;
        }
        if (err < 0) {
            fprintf(stderr, "tcp ring_buffer__poll failed: %d\n", err);
            return err;
        }
    }

    return 0;
}

static void unload_module(struct bpf_module *module)
{
    if (module->rb != NULL) {
        ring_buffer__free(module->rb);
        module->rb = NULL;
    }

    if (module->link != NULL) {
        bpf_link__destroy(module->link);
        module->link = NULL;
    }

    if (module->obj != NULL) {
        bpf_object__close(module->obj);
        module->obj = NULL;
    }
}

void ebpf_loader_stop(void)
{
    unload_module(&g_tcp);
    unload_module(&g_exec);
}

#include <stdio.h>
#include <errno.h>
#include <bpf/libbpf.h>

#include "ebpf_loader.h"
#include "ringbuf_reader.h"

static struct bpf_object *g_obj = NULL;
static struct bpf_link *g_link = NULL;
static struct ring_buffer *g_rb = NULL;

int ebpf_loader_start(const char *object_path)
{
    struct bpf_program *prog;
    int events_map_fd;
    int err;

    g_obj = bpf_object__open_file(object_path, NULL);
    if (libbpf_get_error(g_obj)) {
        fprintf(stderr, "Failed to open BPF object: %s\n", object_path);
        g_obj = NULL;
        return -1;
    }

    err = bpf_object__load(g_obj);
    if (err) {
        fprintf(stderr, "Failed to load BPF object: %s\n", object_path);
        return -1;
    }

    prog = bpf_object__find_program_by_name(g_obj, "handle_exec");
    if (prog == NULL) {
        fprintf(stderr, "Failed to find BPF program: handle_exec\n");
        return -1;
    }

    g_link = bpf_program__attach(prog);
    if (libbpf_get_error(g_link)) {
        fprintf(stderr, "Failed to attach BPF program\n");
        g_link = NULL;
        return -1;
    }

    events_map_fd = bpf_object__find_map_fd_by_name(g_obj, "events");
    if (events_map_fd < 0) {
        fprintf(stderr, "Failed to find ring buffer map: events\n");
        return -1;
    }

    g_rb = ringbuf_reader_create(events_map_fd);
    if (g_rb == NULL) {
        fprintf(stderr, "Failed to create ring buffer reader\n");
        return -1;
    }

    return 0;
}

int ebpf_loader_poll(int timeout_ms)
{
    int err;

    if (g_rb == NULL) {
        return 0;
    }

    err = ring_buffer__poll(g_rb, timeout_ms);
if (err == -EINTR) {
    return 0;
}

if (err < 0) {
    fprintf(stderr, "ring_buffer__poll failed: %d\n", err);
    return err;
}
    return 0;
}

void ebpf_loader_stop(void)
{
    if (g_rb != NULL) {
        ring_buffer__free(g_rb);
        g_rb = NULL;
    }

    if (g_link != NULL) {
        bpf_link__destroy(g_link);
        g_link = NULL;
    }

    if (g_obj != NULL) {
        bpf_object__close(g_obj);
        g_obj = NULL;
    }
}

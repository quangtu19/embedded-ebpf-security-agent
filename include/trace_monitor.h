#ifndef TRACE_MONITOR_H
#define TRACE_MONITOR_H

#include <stddef.h>

struct bpf_object;
struct bpf_link;
struct ring_buffer;

#define TRACE_MONITOR_MAX_LINKS 16

struct trace_monitor {
    struct bpf_object *exec_obj;
    struct bpf_object *tcp_obj;

    struct bpf_link *exec_links[TRACE_MONITOR_MAX_LINKS];
    struct bpf_link *tcp_links[TRACE_MONITOR_MAX_LINKS];

    size_t exec_link_count;
    size_t tcp_link_count;

    struct ring_buffer *exec_rb;
    struct ring_buffer *tcp_rb;
};

int trace_monitor_init(struct trace_monitor *tm);
int trace_monitor_poll(struct trace_monitor *tm, int timeout_ms);
void trace_monitor_cleanup(struct trace_monitor *tm);

#endif

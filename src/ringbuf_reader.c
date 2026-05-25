#include <stdio.h>
#include <string.h>
#include <bpf/libbpf.h>

#include "event.h"
#include "event_logger.h"

static int handle_proc_exec_event(void *ctx, void *data, size_t data_sz)
{
    struct proc_exec_event *event;
    char json[512];

    (void)ctx;

    if (data_sz < sizeof(struct proc_exec_event)) {
        return 0;
    }

    event = (struct proc_exec_event *)data;

    snprintf(json, sizeof(json),
             "{\"event_type\":\"PROC_EXEC\",\"severity\":\"INFO\","
             "\"pid\":%u,\"uid\":%u,\"comm\":\"%s\",\"ts_ns\":%llu}",
             event->pid,
             event->uid,
             event->comm,
             (unsigned long long)event->ts_ns);

    logger_write(json);

    return 0;
}

struct ring_buffer *ringbuf_reader_create(int map_fd)
{
    return ring_buffer__new(map_fd, handle_proc_exec_event, NULL, NULL);
}

#include <stdio.h>
#include <stdint.h>
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
             "\"component\":\"process\",\"pid\":%u,\"uid\":%u,"
             "\"comm\":\"%s\",\"ts_ns\":%llu}",
             event->pid,
             event->uid,
             event->comm,
             (unsigned long long)event->ts_ns);

    logger_write(json);
    return 0;
}

static void format_ipv4(uint32_t ip, char *buf, size_t buf_sz)
{
    unsigned char *b = (unsigned char *)&ip;

    snprintf(buf, buf_sz, "%u.%u.%u.%u", b[0], b[1], b[2], b[3]);
}

static int handle_tcp_connect_event(void *ctx, void *data, size_t data_sz)
{
    struct tcp_connect_event *event;
    char json[768];
    char ip_str[32];
    const char *event_name;
    const char *severity;

    (void)ctx;

    if (data_sz < sizeof(struct tcp_connect_event)) {
        return 0;
    }

    event = (struct tcp_connect_event *)data;
    format_ipv4(event->dst_ip, ip_str, sizeof(ip_str));

    if (event->event_type == EVENT_SUSPICIOUS_CONNECT) {
        event_name = "SUSPICIOUS_CONNECT";
        severity = "WARNING";

        snprintf(json, sizeof(json),
                 "{\"event_type\":\"%s\",\"severity\":\"%s\","
                 "\"component\":\"network\",\"pid\":%u,\"uid\":%u,"
                 "\"comm\":\"%s\",\"dst_ip\":\"%s\",\"dst_port\":%u,"
                 "\"protocol\":\"tcp\",\"reason\":\"suspicious_port\","
                 "\"ts_ns\":%llu}",
                 event_name,
                 severity,
                 event->pid,
                 event->uid,
                 event->comm,
                 ip_str,
                 event->dst_port,
                 (unsigned long long)event->ts_ns);
    } else {
        event_name = "TCP_CONNECT";
        severity = "INFO";

        snprintf(json, sizeof(json),
                 "{\"event_type\":\"%s\",\"severity\":\"%s\","
                 "\"component\":\"network\",\"pid\":%u,\"uid\":%u,"
                 "\"comm\":\"%s\",\"dst_ip\":\"%s\",\"dst_port\":%u,"
                 "\"protocol\":\"tcp\",\"ts_ns\":%llu}",
                 event_name,
                 severity,
                 event->pid,
                 event->uid,
                 event->comm,
                 ip_str,
                 event->dst_port,
                 (unsigned long long)event->ts_ns);
    }

    logger_write(json);
    return 0;
}

struct ring_buffer *ringbuf_reader_create_proc(int map_fd)
{
    return ring_buffer__new(map_fd, handle_proc_exec_event, NULL, NULL);
}

struct ring_buffer *ringbuf_reader_create_tcp(int map_fd)
{
    return ring_buffer__new(map_fd, handle_tcp_connect_event, NULL, NULL);
}

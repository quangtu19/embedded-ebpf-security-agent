#include "trace_monitor.h"
#include "event_logger.h"

#include <arpa/inet.h>
#include <bpf/libbpf.h>
#include <errno.h>
#include <linux/types.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>

#define TASK_COMM_LEN 16

struct proc_exec_event {
    __u64 ts_ns;
    __u32 pid;
    __u32 uid;
    char comm[TASK_COMM_LEN];
};

struct tcp_connect_event {
    __u32 event_type;
    __u64 ts_ns;
    __u32 pid;
    __u32 uid;
    char comm[TASK_COMM_LEN];
    __u32 dst_ip;
    __u16 dst_port;
    __u8 protocol;
};

static int bump_memlock_rlimit(void)
{
    struct rlimit rlim = {
        .rlim_cur = RLIM_INFINITY,
        .rlim_max = RLIM_INFINITY,
    };

    if (setrlimit(RLIMIT_MEMLOCK, &rlim) != 0) {
        perror("setrlimit(RLIMIT_MEMLOCK)");
        return -1;
    }

    return 0;
}

static const char *event_type_name(__u32 event_type)
{
    if (event_type == 3)
        return "SUSPICIOUS_CONNECT";
    return "TCP_CONNECT";
}

static int handle_exec_event(void *ctx, void *data, size_t len)
{
    const struct proc_exec_event *e = data;
    char line[512];

    (void)ctx;

    if (len < sizeof(*e))
        return 0;

    snprintf(line, sizeof(line),
             "{\"event_type\":\"PROC_EXEC\","
             "\"severity\":\"INFO\","
             "\"component\":\"process\","
             "\"pid\":%u,"
             "\"uid\":%u,"
             "\"comm\":\"%.*s\","
             "\"ts_ns\":%llu}",
             e->pid,
             e->uid,
             TASK_COMM_LEN,
             e->comm,
             (unsigned long long)e->ts_ns);

    logger_write(line);
    return 0;
}

static int handle_tcp_event(void *ctx, void *data, size_t len)
{
    const struct tcp_connect_event *e = data;
    struct in_addr addr;
    char ip[INET_ADDRSTRLEN] = "0.0.0.0";
    char line[640];

    (void)ctx;

    if (len < sizeof(*e))
        return 0;

    addr.s_addr = e->dst_ip;
    inet_ntop(AF_INET, &addr, ip, sizeof(ip));

    snprintf(line, sizeof(line),
             "{\"event_type\":\"%s\","
             "\"severity\":\"INFO\","
             "\"component\":\"network\","
             "\"pid\":%u,"
             "\"uid\":%u,"
             "\"comm\":\"%.*s\","
             "\"dst_ip\":\"%s\","
             "\"dst_port\":%u,"
             "\"protocol\":\"tcp\","
             "\"ts_ns\":%llu}",
             event_type_name(e->event_type),
             e->pid,
             e->uid,
             TASK_COMM_LEN,
             e->comm,
             ip,
             e->dst_port,
             (unsigned long long)e->ts_ns);

    logger_write(line);
    return 0;
}

static struct bpf_object *open_and_load_bpf_object(const char *primary_path,
                                                   const char *fallback_path,
                                                   const char **used_path)
{
    struct bpf_object *obj = NULL;

    obj = bpf_object__open_file(primary_path, NULL);
    if (!obj || libbpf_get_error(obj)) {
        obj = bpf_object__open_file(fallback_path, NULL);
        if (!obj || libbpf_get_error(obj)) {
            fprintf(stderr, "trace_monitor: failed to open %s or %s\n",
                    primary_path, fallback_path);
            return NULL;
        }
        *used_path = fallback_path;
    } else {
        *used_path = primary_path;
    }

    if (bpf_object__load(obj) != 0) {
        fprintf(stderr, "trace_monitor: failed to load %s\n", *used_path);
        bpf_object__close(obj);
        return NULL;
    }

    return obj;
}

static int attach_all_programs(struct bpf_object *obj,
                               struct bpf_link **links,
                               size_t *link_count)
{
    struct bpf_program *prog;

    *link_count = 0;

    bpf_object__for_each_program(prog, obj) {
        struct bpf_link *link = bpf_program__attach(prog);

        if (!link || libbpf_get_error(link)) {
            fprintf(stderr, "trace_monitor: failed to attach BPF program\n");
            return -1;
        }

        if (*link_count >= TRACE_MONITOR_MAX_LINKS) {
            fprintf(stderr, "trace_monitor: too many BPF links\n");
            bpf_link__destroy(link);
            return -1;
        }

        links[*link_count] = link;
        (*link_count)++;
    }

    return (*link_count > 0) ? 0 : -1;
}

static void destroy_links(struct bpf_link **links, size_t count)
{
    size_t i;

    for (i = 0; i < count; i++) {
        if (links[i]) {
            bpf_link__destroy(links[i]);
            links[i] = NULL;
        }
    }
}

int trace_monitor_init(struct trace_monitor *tm)
{
    int map_fd;
    const char *used_path = NULL;

    if (!tm)
        return -1;

    memset(tm, 0, sizeof(*tm));

    if (bump_memlock_rlimit() != 0)
        return -1;

    tm->exec_obj = open_and_load_bpf_object(
        "/opt/ebpf-security-agent/bpf/exec_trace.bpf.o",
        "bpf/exec_trace.bpf.o",
        &used_path);
    if (!tm->exec_obj)
        return -1;

    if (attach_all_programs(tm->exec_obj, tm->exec_links, &tm->exec_link_count) != 0)
        return -1;

    map_fd = bpf_object__find_map_fd_by_name(tm->exec_obj, "events");
    if (map_fd < 0) {
        fprintf(stderr, "trace_monitor: exec events map not found\n");
        return -1;
    }

    tm->exec_rb = ring_buffer__new(map_fd, handle_exec_event, NULL, NULL);
    if (!tm->exec_rb || libbpf_get_error(tm->exec_rb)) {
        fprintf(stderr, "trace_monitor: failed to create exec ring buffer\n");
        return -1;
    }

    tm->tcp_obj = open_and_load_bpf_object(
        "/opt/ebpf-security-agent/bpf/tcp_connect.bpf.o",
        "bpf/tcp_connect.bpf.o",
        &used_path);
    if (!tm->tcp_obj)
        return -1;

    if (attach_all_programs(tm->tcp_obj, tm->tcp_links, &tm->tcp_link_count) != 0)
        return -1;

    map_fd = bpf_object__find_map_fd_by_name(tm->tcp_obj, "events");
    if (map_fd < 0) {
        fprintf(stderr, "trace_monitor: tcp events map not found\n");
        return -1;
    }

    tm->tcp_rb = ring_buffer__new(map_fd, handle_tcp_event, NULL, NULL);
    if (!tm->tcp_rb || libbpf_get_error(tm->tcp_rb)) {
        fprintf(stderr, "trace_monitor: failed to create tcp ring buffer\n");
        return -1;
    }

    logger_write("{\"event_type\":\"TRACE_MONITOR_ATTACHED\",\"severity\":\"INFO\",\"component\":\"trace\"}");
    return 0;
}

int trace_monitor_poll(struct trace_monitor *tm, int timeout_ms)
{
    int ret = 0;
    int r;

    if (!tm)
        return -1;

    if (tm->exec_rb) {
        r = ring_buffer__poll(tm->exec_rb, timeout_ms);
        if (r < 0 && r != -EINTR)
            ret = r;
    }

    if (tm->tcp_rb) {
        r = ring_buffer__poll(tm->tcp_rb, 0);
        if (r < 0 && r != -EINTR)
            ret = r;
    }

    return ret;
}

void trace_monitor_cleanup(struct trace_monitor *tm)
{
    if (!tm)
        return;

    if (tm->exec_rb) {
        ring_buffer__free(tm->exec_rb);
        tm->exec_rb = NULL;
    }

    if (tm->tcp_rb) {
        ring_buffer__free(tm->tcp_rb);
        tm->tcp_rb = NULL;
    }

    destroy_links(tm->exec_links, tm->exec_link_count);
    destroy_links(tm->tcp_links, tm->tcp_link_count);

    if (tm->exec_obj) {
        bpf_object__close(tm->exec_obj);
        tm->exec_obj = NULL;
    }

    if (tm->tcp_obj) {
        bpf_object__close(tm->tcp_obj);
        tm->tcp_obj = NULL;
    }
}

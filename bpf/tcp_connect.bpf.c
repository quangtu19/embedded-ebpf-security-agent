#include <linux/bpf.h>
#include <linux/in.h>
#include <linux/socket.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>

#include "common.bpf.h"

#ifndef AF_INET
#define AF_INET 2
#endif

struct trace_event_raw_sys_enter {
    unsigned short common_type;
    unsigned char common_flags;
    unsigned char common_preempt_count;
    int common_pid;
    long id;
    unsigned long args[6];
};

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);
} events SEC(".maps");

SEC("tracepoint/syscalls/sys_enter_connect")
int handle_tcp_connect(struct trace_event_raw_sys_enter *ctx)
{
    struct tcp_connect_event *e;
    struct sockaddr_in addr = {};
    void *uaddr;
    __u64 pid_tgid;
    __u64 uid_gid;
    __u16 dport;

    uaddr = (void *)ctx->args[1];
    if (!uaddr) {
        return 0;
    }

    if (bpf_probe_read_user(&addr, sizeof(addr), uaddr) < 0) {
        return 0;
    }

    if (addr.sin_family != AF_INET) {
        return 0;
    }

    e = bpf_ringbuf_reserve(&events, sizeof(*e), 0);
    if (!e) {
        return 0;
    }

    pid_tgid = bpf_get_current_pid_tgid();
    uid_gid = bpf_get_current_uid_gid();
    dport = bpf_ntohs(addr.sin_port);

    e->event_type = EVENT_TCP_CONNECT;
    e->ts_ns = bpf_ktime_get_ns();
    e->pid = pid_tgid >> 32;
    e->uid = (__u32)uid_gid;
    e->dst_ip = addr.sin_addr.s_addr;
    e->dst_port = dport;
    e->protocol = 6;

    bpf_get_current_comm(&e->comm, sizeof(e->comm));

    if (dport == 4444 || dport == 1337 || dport == 31337) {
        e->event_type = EVENT_SUSPICIOUS_CONNECT;
    }

    bpf_ringbuf_submit(e, 0);
    return 0;
}

char LICENSE[] SEC("license") = "GPL";

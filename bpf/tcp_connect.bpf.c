#include <linux/bpf.h>
#include <linux/in.h>
#include <linux/socket.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_endian.h>

#include "common.bpf.h"

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);
} events SEC(".maps");

SEC("kprobe/tcp_v4_connect")
int handle_tcp_v4_connect(struct pt_regs *ctx)
{
    struct tcp_connect_event *e;
    struct sockaddr_in addr = {};
    void *uaddr;
    __u64 pid_tgid;
    __u64 uid_gid;
    __u16 dport;

    uaddr = (void *)PT_REGS_PARM2(ctx);
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

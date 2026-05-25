#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

#include "common.bpf.h"

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);
} events SEC(".maps");

SEC("tracepoint/syscalls/sys_enter_execve")
int handle_exec(void *ctx)
{
    struct proc_exec_event *e;
    __u64 pid_tgid;
    __u64 uid_gid;

    (void)ctx;

    e = bpf_ringbuf_reserve(&events, sizeof(*e), 0);
    if (!e) {
        return 0;
    }

    pid_tgid = bpf_get_current_pid_tgid();
    uid_gid = bpf_get_current_uid_gid();

    e->ts_ns = bpf_ktime_get_ns();
    e->pid = pid_tgid >> 32;
    e->uid = (__u32)uid_gid;

    bpf_get_current_comm(&e->comm, sizeof(e->comm));

    bpf_ringbuf_submit(e, 0);
    return 0;
}

char LICENSE[] SEC("license") = "GPL";

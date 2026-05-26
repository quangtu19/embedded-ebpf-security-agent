#ifndef COMMON_BPF_H
#define COMMON_BPF_H

#include <linux/types.h>

#define TASK_COMM_LEN 16

#define EVENT_PROC_EXEC 1
#define EVENT_TCP_CONNECT 2
#define EVENT_SUSPICIOUS_CONNECT 3

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

#endif

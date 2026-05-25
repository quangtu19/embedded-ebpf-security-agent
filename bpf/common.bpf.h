#ifndef COMMON_BPF_H
#define COMMON_BPF_H

#include <linux/types.h>

#define TASK_COMM_LEN 16

struct proc_exec_event {
    __u64 ts_ns;
    __u32 pid;
    __u32 uid;
    char comm[TASK_COMM_LEN];
};

#endif

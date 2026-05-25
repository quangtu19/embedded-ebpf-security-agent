#ifndef EVENT_H
#define EVENT_H

#include <stdint.h>

#define TASK_COMM_LEN 16

struct proc_exec_event {
    uint64_t ts_ns;
    uint32_t pid;
    uint32_t uid;
    char comm[TASK_COMM_LEN];
};

#endif

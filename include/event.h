#ifndef EVENT_H
#define EVENT_H

#include <stdint.h>

#define TASK_COMM_LEN 16

#define EVENT_PROC_EXEC 1
#define EVENT_TCP_CONNECT 2
#define EVENT_SUSPICIOUS_CONNECT 3

struct proc_exec_event {
    uint64_t ts_ns;
    uint32_t pid;
    uint32_t uid;
    char comm[TASK_COMM_LEN];
};

struct tcp_connect_event {
    uint32_t event_type;
    uint64_t ts_ns;
    uint32_t pid;
    uint32_t uid;
    char comm[TASK_COMM_LEN];
    uint32_t dst_ip;
    uint16_t dst_port;
    uint8_t protocol;
};

#endif

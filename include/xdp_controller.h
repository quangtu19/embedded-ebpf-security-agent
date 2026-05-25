#ifndef XDP_CONTROLLER_H
#define XDP_CONTROLLER_H

#include <stdint.h>

struct bpf_object;

struct xdp_stats {
    uint64_t tcp;
    uint64_t udp;
    uint64_t icmp;
    uint64_t other;
    uint64_t total_packets;
    uint64_t total_bytes;
    uint64_t dropped;
};

struct xdp_controller {
    struct bpf_object *obj;
    int ifindex;
    unsigned int xdp_flags;
    int protocol_map_fd;
    int packet_map_fd;
    int blacklist_map_fd;
    int attached;
};

int xdp_controller_init(
    struct xdp_controller *ctl,
    const char *object_path,
    const char *iface,
    const char *mode
);

int xdp_controller_read_stats(
    struct xdp_controller *ctl,
    struct xdp_stats *stats
);

int xdp_controller_log_stats(struct xdp_controller *ctl);

void xdp_controller_cleanup(struct xdp_controller *ctl);

#endif

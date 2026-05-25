#include "xdp_controller.h"
#include "event_logger.h"
#include "common_bpf_keys.h"

#include <bpf/libbpf.h>
#include <bpf/bpf.h>

#include <errno.h>
#include <linux/if_link.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <sys/resource.h>
#include <unistd.h>

#ifndef XDP_FLAGS_SKB_MODE
#define XDP_FLAGS_SKB_MODE (1U << 1)
#endif

#ifndef XDP_FLAGS_DRV_MODE
#define XDP_FLAGS_DRV_MODE (1U << 2)
#endif

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

static unsigned int mode_to_xdp_flags(const char *mode)
{
    if (mode == NULL)
        return XDP_FLAGS_SKB_MODE;

    if (strcmp(mode, "native") == 0 || strcmp(mode, "drv") == 0)
        return XDP_FLAGS_DRV_MODE;

    return XDP_FLAGS_SKB_MODE;
}

static uint64_t read_u64_counter(int map_fd, uint32_t key)
{
    uint64_t value = 0;

    if (bpf_map_lookup_elem(map_fd, &key, &value) != 0)
        return 0;

    return value;
}

int xdp_controller_init(
    struct xdp_controller *ctl,
    const char *object_path,
    const char *iface,
    const char *mode
)
{
    struct bpf_program *prog = NULL;
    int prog_fd;

    if (ctl == NULL || object_path == NULL || iface == NULL) {
        fprintf(stderr, "xdp_controller_init: invalid argument\n");
        return -1;
    }

    memset(ctl, 0, sizeof(*ctl));
    ctl->protocol_map_fd = -1;
    ctl->packet_map_fd = -1;
    ctl->blacklist_map_fd = -1;

    if (bump_memlock_rlimit() != 0)
        return -1;

    ctl->ifindex = if_nametoindex(iface);
    if (ctl->ifindex == 0) {
        fprintf(stderr, "xdp_controller_init: interface not found: %s\n", iface);
        return -1;
    }

    ctl->xdp_flags = mode_to_xdp_flags(mode);

    ctl->obj = bpf_object__open_file(object_path, NULL);
    if (!ctl->obj) {
        fprintf(stderr, "xdp_controller_init: failed to open %s\n", object_path);
        return -1;
    }

    if (bpf_object__load(ctl->obj) != 0) {
        fprintf(stderr, "xdp_controller_init: failed to load %s\n", object_path);
        bpf_object__close(ctl->obj);
        ctl->obj = NULL;
        return -1;
    }

    prog = bpf_object__find_program_by_name(ctl->obj, "xdp_packet_counter");
    if (!prog)
        prog = bpf_object__next_program(ctl->obj, NULL);

    if (!prog) {
        fprintf(stderr, "xdp_controller_init: no BPF program found\n");
        bpf_object__close(ctl->obj);
        ctl->obj = NULL;
        return -1;
    }

    prog_fd = bpf_program__fd(prog);
    if (prog_fd < 0) {
        fprintf(stderr, "xdp_controller_init: invalid program fd\n");
        bpf_object__close(ctl->obj);
        ctl->obj = NULL;
        return -1;
    }

    ctl->protocol_map_fd =
        bpf_object__find_map_fd_by_name(ctl->obj, "protocol_count_map");

    ctl->packet_map_fd =
        bpf_object__find_map_fd_by_name(ctl->obj, "packet_count_map");

    ctl->blacklist_map_fd =
        bpf_object__find_map_fd_by_name(ctl->obj, "blacklist_map");

    if (ctl->protocol_map_fd < 0 ||
        ctl->packet_map_fd < 0 ||
        ctl->blacklist_map_fd < 0) {
        fprintf(stderr, "xdp_controller_init: failed to find XDP maps\n");
        bpf_object__close(ctl->obj);
        ctl->obj = NULL;
        return -1;
    }

    if (bpf_xdp_attach(ctl->ifindex, prog_fd, ctl->xdp_flags, NULL) != 0) {
        fprintf(stderr,
                "xdp_controller_init: failed to attach XDP to %s: %s\n",
                iface,
                strerror(errno));
        bpf_object__close(ctl->obj);
        ctl->obj = NULL;
        return -1;
    }

    ctl->attached = 1;

    return 0;
}

int xdp_controller_read_stats(
    struct xdp_controller *ctl,
    struct xdp_stats *stats
)
{
    if (ctl == NULL || stats == NULL)
        return -1;

    memset(stats, 0, sizeof(*stats));

    stats->tcp = read_u64_counter(ctl->protocol_map_fd, PROTO_TCP);
    stats->udp = read_u64_counter(ctl->protocol_map_fd, PROTO_UDP);
    stats->icmp = read_u64_counter(ctl->protocol_map_fd, PROTO_ICMP);
    stats->other = read_u64_counter(ctl->protocol_map_fd, PROTO_OTHER);

    stats->total_packets =
        read_u64_counter(ctl->packet_map_fd, STAT_TOTAL_PACKETS);

    stats->total_bytes =
        read_u64_counter(ctl->packet_map_fd, STAT_TOTAL_BYTES);

    stats->dropped =
        read_u64_counter(ctl->packet_map_fd, STAT_DROPPED);

    return 0;
}

int xdp_controller_log_stats(struct xdp_controller *ctl)
{
    struct xdp_stats stats;
    char line[640];

    if (xdp_controller_read_stats(ctl, &stats) != 0)
        return -1;

    snprintf(
        line,
        sizeof(line),
        "{\"event_type\":\"PACKET_STATS\","
        "\"severity\":\"INFO\","
        "\"component\":\"xdp\","
        "\"tcp\":%llu,"
        "\"udp\":%llu,"
        "\"icmp\":%llu,"
        "\"other\":%llu,"
        "\"total_packets\":%llu,"
        "\"total_bytes\":%llu,"
        "\"dropped\":%llu}",
        (unsigned long long)stats.tcp,
        (unsigned long long)stats.udp,
        (unsigned long long)stats.icmp,
        (unsigned long long)stats.other,
        (unsigned long long)stats.total_packets,
        (unsigned long long)stats.total_bytes,
        (unsigned long long)stats.dropped
    );

    return logger_write(line);
}

void xdp_controller_cleanup(struct xdp_controller *ctl)
{
    if (ctl == NULL)
        return;

    if (ctl->attached) {
        bpf_xdp_detach(ctl->ifindex, ctl->xdp_flags, NULL);
        ctl->attached = 0;
    }

    if (ctl->obj) {
        bpf_object__close(ctl->obj);
        ctl->obj = NULL;
    }
}

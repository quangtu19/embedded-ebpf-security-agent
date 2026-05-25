#include "common.bpf.h"

#include <linux/bpf.h>
#include <linux/if_ether.h>
#include <linux/ip.h>
#include <linux/in.h>

#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 4);
    __type(key, __u32);
    __type(value, __u64);
} protocol_count_map SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_ARRAY);
    __uint(max_entries, 3);
    __type(key, __u32);
    __type(value, __u64);
} packet_count_map SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 1024);
    __type(key, __u32);
    __type(value, __u8);
} blacklist_map SEC(".maps");

static __always_inline void inc_counter(void *map, __u32 key, __u64 delta)
{
    __u64 *value;

    value = bpf_map_lookup_elem(map, &key);
    if (value)
        __sync_fetch_and_add(value, delta);
}

SEC("xdp")
int xdp_packet_counter(struct xdp_md *ctx)
{
    void *data = (void *)(long)ctx->data;
    void *data_end = (void *)(long)ctx->data_end;

    struct ethhdr *eth = data;

    if ((void *)(eth + 1) > data_end)
        return XDP_PASS;

    __u64 pkt_len = (__u64)((long)data_end - (long)data);

    inc_counter(&packet_count_map, STAT_TOTAL_PACKETS, 1);
    inc_counter(&packet_count_map, STAT_TOTAL_BYTES, pkt_len);

    if (eth->h_proto != bpf_htons(ETH_P_IP)) {
        inc_counter(&protocol_count_map, PROTO_OTHER, 1);
        return XDP_PASS;
    }

    struct iphdr *iph = (void *)(eth + 1);

    if ((void *)(iph + 1) > data_end) {
        inc_counter(&protocol_count_map, PROTO_OTHER, 1);
        return XDP_PASS;
    }

    if (iph->protocol == IPPROTO_TCP) {
        inc_counter(&protocol_count_map, PROTO_TCP, 1);
    } else if (iph->protocol == IPPROTO_UDP) {
        inc_counter(&protocol_count_map, PROTO_UDP, 1);
    } else if (iph->protocol == IPPROTO_ICMP) {
        inc_counter(&protocol_count_map, PROTO_ICMP, 1);
    } else {
        inc_counter(&protocol_count_map, PROTO_OTHER, 1);
    }

    __u32 src_ip = iph->saddr;
    __u8 *blocked = bpf_map_lookup_elem(&blacklist_map, &src_ip);

    if (blocked) {
        inc_counter(&packet_count_map, STAT_DROPPED, 1);
        return XDP_DROP;
    }

    return XDP_PASS;
}

char LICENSE[] SEC("license") = "GPL";

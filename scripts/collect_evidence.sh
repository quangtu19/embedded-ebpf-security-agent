#!/bin/bash
set -e

IFACE=${1:-enp0s8}
OUT_DIR="docs/evidence/khung6"

mkdir -p "$OUT_DIR"

uname -a > "$OUT_DIR/uname.txt"
ip addr > "$OUT_DIR/ip_addr.txt"
ip -details link show dev "$IFACE" > "$OUT_DIR/ip_link_xdp.txt" 2>&1 || true
sudo bpftool prog show > "$OUT_DIR/bpftool_prog_show.txt" 2>&1 || true
sudo bpftool map show > "$OUT_DIR/bpftool_map_show.txt" 2>&1 || true
sudo bpftool map dump name protocol_count_map > "$OUT_DIR/protocol_count_map_dump.txt" 2>&1 || true
sudo bpftool map dump name packet_count_map > "$OUT_DIR/packet_count_map_dump.txt" 2>&1 || true
grep PACKET_STATS events.log > "$OUT_DIR/packet_stats_logs.txt" 2>/dev/null || true

echo "[+] Evidence saved to $OUT_DIR"

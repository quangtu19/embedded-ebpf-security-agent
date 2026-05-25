#!/usr/bin/env bash
set -euo pipefail

IP=${1:-}

if [ -z "$IP" ]; then
    echo "Usage: sudo ./scripts/blacklist_remove.sh <IPv4>"
    exit 1
fi

MAP_ID=$(sudo bpftool map show | awk '/blacklist_map/ {gsub(":", "", $1); print $1; exit}')

if [ -z "$MAP_ID" ]; then
    echo "ERROR: blacklist_map not found. Is agent running?"
    exit 1
fi

HEX=$(python3 -c 'import socket,sys; print(" ".join(f"{b:02x}" for b in socket.inet_aton(sys.argv[1])))' "$IP")

sudo bpftool map delete id "$MAP_ID" key hex $HEX 2>/dev/null || true

echo "BLACKLIST_REMOVE ip=$IP map_id=$MAP_ID"
echo "{\"event_type\":\"BLACKLIST_REMOVE\",\"severity\":\"INFO\",\"component\":\"xdp\",\"ip\":\"$IP\"}" >> events.log

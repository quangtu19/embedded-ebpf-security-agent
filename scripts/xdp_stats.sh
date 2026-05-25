#!/usr/bin/env bash
set -euo pipefail

PACKET_ID=$(sudo bpftool map show | awk '/packet_count/ {gsub(":", "", $1); print $1; exit}')
PROTO_ID=$(sudo bpftool map show | awk '/protocol_count/ {gsub(":", "", $1); print $1; exit}')
BLACKLIST_ID=$(sudo bpftool map show | awk '/blacklist_map/ {gsub(":", "", $1); print $1; exit}')

echo "packet_count_map id=${PACKET_ID:-not_found}"
if [ -n "${PACKET_ID:-}" ]; then
    sudo bpftool map dump id "$PACKET_ID"
fi

echo
echo "protocol_count_map id=${PROTO_ID:-not_found}"
if [ -n "${PROTO_ID:-}" ]; then
    sudo bpftool map dump id "$PROTO_ID"
fi

echo
echo "blacklist_map id=${BLACKLIST_ID:-not_found}"
if [ -n "${BLACKLIST_ID:-}" ]; then
    sudo bpftool map dump id "$BLACKLIST_ID"
fi

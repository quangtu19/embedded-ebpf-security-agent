#!/usr/bin/env bash
set -euo pipefail

MAP_ID=$(sudo bpftool map show | awk '/blacklist_map/ {gsub(":", "", $1); print $1; exit}')

if [ -z "$MAP_ID" ]; then
    echo "ERROR: blacklist_map not found. Is agent running?"
    exit 1
fi

echo "blacklist_map id=$MAP_ID"
sudo bpftool map dump id "$MAP_ID"

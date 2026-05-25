#!/bin/bash
set -e

IFACE=${1:-enp0s8}

echo "[+] Detaching XDP from $IFACE"

sudo ip link set dev "$IFACE" xdpgeneric off 2>/dev/null || true
sudo ip link set dev "$IFACE" xdp off 2>/dev/null || true

echo "[+] Done"

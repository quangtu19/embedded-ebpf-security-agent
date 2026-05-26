#!/usr/bin/env bash
set -euo pipefail

IFACE="${1:-enp0s8}"

echo "Resetting XDP on interface: $IFACE"

sudo ip link set dev "$IFACE" xdpgeneric off 2>/dev/null || true
sudo ip link set dev "$IFACE" xdp off 2>/dev/null || true
sudo ip link show dev "$IFACE"

echo "Done."

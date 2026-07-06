#!/bin/bash
set -e

TARGET=${1:-192.168.56.10}

echo "[+] ICMP test"
ping -c 5 "$TARGET"

echo "[+] TCP test to SSH port"
nc -vz -w 2 "$TARGET" 22 || true

echo "[+] Done"

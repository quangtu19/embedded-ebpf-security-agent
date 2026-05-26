#!/usr/bin/env bash
set -euo pipefail

BASE="docs/evidence/khung10"
OUT="$BASE/benchmarks/xdp_counter_result.txt"
PING_TARGET="${PING_TARGET:-8.8.8.8}"
PING_COUNT="${PING_COUNT:-20}"

mkdir -p "$(dirname "$OUT")"

run_ctl() {
  if [ -x "./ebpf-agentctl" ]; then
    sudo ./ebpf-agentctl "$@"
  elif [ -x "/opt/ebpf-security-agent/bin/ebpf-agentctl" ]; then
    sudo /opt/ebpf-security-agent/bin/ebpf-agentctl "$@"
  else
    echo "ERROR: ebpf-agentctl not found" >&2
    return 1
  fi
}

{
  echo "=== KHUNG 10 - XDP counter consistency benchmark ==="
  date -Is
  echo "ping_target=$PING_TARGET"
  echo "ping_count=$PING_COUNT"
  echo

  echo "--- service status ---"
  systemctl is-active ebpf-security-agent || true
  echo

  echo "--- stats before traffic ---"
  run_ctl stats || true
  echo

  echo "--- generate ICMP traffic ---"
  ping -c "$PING_COUNT" "$PING_TARGET" || true
  echo

  sleep 2

  echo "--- stats after traffic ---"
  run_ctl stats || true
  echo

  echo "--- bpftool map show ---"
  sudo bpftool map show 2>&1 || true
  echo

  echo "Expected:"
  echo "- total/icmp counter should increase after ping if XDP is attached to the correct interface."
  echo "- If counters do not change, check xdp_interface in config/agent.yaml and systemd service."
} | tee "$OUT"

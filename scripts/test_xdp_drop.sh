#!/usr/bin/env bash
set -euo pipefail

BASE="docs/evidence/khung10"
OUT="$BASE/network/xdp_drop_result.txt"
IFACE="${IFACE:-enp0s8}"
CLIENT_IP="${CLIENT_IP:-}"

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

SERVER_IP="$(ip -4 addr show dev "$IFACE" 2>/dev/null | awk '/inet /{print $2}' | cut -d/ -f1 | head -n 1 || true)"

{
  echo "=== KHUNG 10 - XDP blacklist/drop test ==="
  date -Is
  echo "iface=$IFACE"
  echo "server_ip=$SERVER_IP"
  echo "client_ip=$CLIENT_IP"
  echo

  if [ -z "$CLIENT_IP" ]; then
    echo "SKIP: CLIENT_IP is not set."
    echo "Usage example:"
    echo "  sudo CLIENT_IP=192.168.1.50 IFACE=$IFACE ./scripts/test_xdp_drop.sh"
    echo
    echo "During the 15-second window, run from client machine:"
    echo "  ping $SERVER_IP"
    exit 0
  fi

  echo "--- stats before ---"
  run_ctl stats || true
  echo

  echo "--- blacklist add ---"
  run_ctl blacklist add "$CLIENT_IP" || true
  echo

  echo "--- blacklist list ---"
  run_ctl blacklist list || true
  echo

  echo "Now ping this server from client $CLIENT_IP for 15 seconds:"
  echo "  ping $SERVER_IP"
  echo

  sleep 15

  echo "--- stats during/after blacklist ---"
  run_ctl stats || true
  echo

  echo "--- blacklist remove ---"
  run_ctl blacklist remove "$CLIENT_IP" || true
  echo

  echo "--- stats after remove ---"
  run_ctl stats || true
  echo

  echo "Expected:"
  echo "- During blacklist: client ping should fail or packet drop counter should increase."
  echo "- After remove: client ping should work again."
  echo
  echo "Note:"
  echo "- If command says blacklist_map missing, current XDP program has not implemented blacklist map yet."
} | tee "$OUT"

#!/usr/bin/env bash
set -euo pipefail

# Automated one-VM XDP blacklist/drop validation using a temporary network namespace.
# It avoids needing a second VM by generating traffic from a veth peer with a known source IP.
# Run from repository root after installing and starting the systemd service.

if [ "${EUID}" -ne 0 ]; then
  echo "ERROR: run with sudo/root" >&2
  exit 1
fi

ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
cd "$ROOT"

SERVICE="${SERVICE:-ebpf-security-agent}"
CONFIG="${CONFIG:-/etc/ebpf-security-agent/agent.yaml}"
IFACE="${IFACE:-veth-agent}"
PEER="${PEER:-veth-client}"
NS="${NS:-ebpf-xdp-client}"
HOST_IP="${HOST_IP:-10.200.1.1}"
CLIENT_IP="${CLIENT_IP:-10.200.1.2}"
OUT="${OUT:-docs/evidence/khung10/network/xdp_drop_netns_result.txt}"
MODE="${MODE:-skb}"

mkdir -p "$(dirname "$OUT")"

run_ctl() {
  if [ -x "./ebpf-agentctl" ]; then
    ./ebpf-agentctl "$@"
  elif [ -x "/opt/ebpf-security-agent/bin/ebpf-agentctl" ]; then
    /opt/ebpf-security-agent/bin/ebpf-agentctl "$@"
  elif command -v ebpf-agentctl >/dev/null 2>&1; then
    ebpf-agentctl "$@"
  else
    echo "ERROR: ebpf-agentctl not found" >&2
    return 127
  fi
}

restore_config=0
backup_config=""
cleanup() {
  set +e
  run_ctl blacklist remove "$CLIENT_IP" >/dev/null 2>&1 || true
  ip netns del "$NS" >/dev/null 2>&1 || true
  ip link del "$IFACE" >/dev/null 2>&1 || true
  if [ "$restore_config" = "1" ] && [ -n "$backup_config" ] && [ -f "$backup_config" ]; then
    cp "$backup_config" "$CONFIG"
    systemctl restart "$SERVICE" >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT

{
  echo "=== XDP blacklist/drop netns validation ==="
  date -Is
  echo "repo=$ROOT"
  echo "service=$SERVICE"
  echo "config=$CONFIG"
  echo "iface=$IFACE"
  echo "namespace=$NS"
  echo "host_ip=$HOST_IP"
  echo "client_ip=$CLIENT_IP"
  echo "mode=$MODE"
  echo

  echo "--- environment ---"
  uname -a || true
  ip -br link || true
  systemctl is-active "$SERVICE" || true

  echo
  echo "--- create veth/netns lab ---"
  ip netns del "$NS" >/dev/null 2>&1 || true
  ip link del "$IFACE" >/dev/null 2>&1 || true
  ip netns add "$NS"
  ip link add "$IFACE" type veth peer name "$PEER"
  ip addr add "$HOST_IP/24" dev "$IFACE"
  ip link set "$IFACE" up
  ip link set "$PEER" netns "$NS"
  ip netns exec "$NS" ip addr add "$CLIENT_IP/24" dev "$PEER"
  ip netns exec "$NS" ip link set lo up
  ip netns exec "$NS" ip link set "$PEER" up
  ip -br addr show "$IFACE" || true
  ip netns exec "$NS" ip -br addr || true

  echo
  echo "--- point agent config to lab interface ---"
  if [ ! -f "$CONFIG" ]; then
    echo "ERROR: config not found: $CONFIG"
    echo "final_result=FAIL"
    exit 1
  fi
  backup_config="$(mktemp)"
  cp "$CONFIG" "$backup_config"
  restore_config=1
  sed -i -E "s/^([[:space:]]*xdp_interface:[[:space:]]*).*/\1$IFACE/" "$CONFIG"
  sed -i -E "s/^([[:space:]]*xdp_mode:[[:space:]]*).*/\1$MODE/" "$CONFIG"
  grep -E 'xdp_interface|xdp_mode|xdp_enable' "$CONFIG" || true

  echo
  echo "--- restart service ---"
  systemctl daemon-reload || true
  systemctl restart "$SERVICE"
  sleep 3
  systemctl --no-pager --full status "$SERVICE" || true
  if ! systemctl is-active --quiet "$SERVICE"; then
    echo "ERROR: service is not active after restart"
    echo "final_result=FAIL"
    exit 1
  fi

  echo
  echo "--- baseline ping before blacklist ---"
  set +e
  ip netns exec "$NS" ping -c 3 -W 1 "$HOST_IP"
  baseline_status=$?
  set -e
  echo "baseline_ping_status=$baseline_status"
  if [ "$baseline_status" -ne 0 ]; then
    echo "ERROR: baseline ping failed before blacklist; cannot validate drop."
    echo "final_result=FAIL"
    exit 1
  fi

  echo
  echo "--- stats before blacklist ---"
  run_ctl stats || true

  echo
  echo "--- add blacklist entry ---"
  run_ctl blacklist add "$CLIENT_IP" || true
  run_ctl blacklist list || true
  sleep 1

  echo
  echo "--- ping during blacklist; expected failure ---"
  set +e
  ip netns exec "$NS" ping -c 4 -W 1 "$HOST_IP"
  drop_ping_status=$?
  set -e
  echo "drop_ping_status=$drop_ping_status"

  echo
  echo "--- stats after blacklist ping ---"
  run_ctl stats || true

  echo
  echo "--- remove blacklist entry ---"
  run_ctl blacklist remove "$CLIENT_IP" || true
  run_ctl blacklist list || true
  sleep 1

  echo
  echo "--- ping after remove; expected success ---"
  set +e
  ip netns exec "$NS" ping -c 3 -W 1 "$HOST_IP"
  recovery_status=$?
  set -e
  echo "recovery_ping_status=$recovery_status"

  echo
  echo "--- final stats ---"
  run_ctl stats || true

  if [ "$baseline_status" -eq 0 ] && [ "$drop_ping_status" -ne 0 ] && [ "$recovery_status" -eq 0 ]; then
    echo "final_result=PASS"
  else
    echo "final_result=FAIL"
    exit 1
  fi
} | tee "$OUT"

if grep -q 'final_result=PASS' "$OUT" && [ -f docs/results.md ]; then
  python3 - <<'PYUPDATE'
from pathlib import Path
import re
p = Path('docs/results.md')
text = p.read_text(encoding='utf-8')
replacement = '| EXP-06 | XDP drop accuracy | netns lab: baseline OK, blacklist drop OK, remove recovery OK | 100% lab | PASS | `docs/evidence/khung10/network/xdp_drop_netns_result.txt` |'
text = re.sub(r'\| EXP-06 \| XDP drop accuracy \|.*?\| 100% lab \| (?:SKIP|PENDING|FAIL) \|.*?\|', replacement, text)
p.write_text(text, encoding='utf-8')
PYUPDATE
  echo "Updated docs/results.md EXP-06 to PASS."
fi

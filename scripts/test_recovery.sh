#!/usr/bin/env bash
set -euo pipefail

BASE="docs/evidence/khung10"
OUT="$BASE/recovery/recovery_result.txt"
SERVICE="ebpf-security-agent"
AGENT_NAME="ebpf-security-agent"

mkdir -p "$(dirname "$OUT")"

{
  echo "=== KHUNG 10 - systemd recovery test ==="
  date -Is
  echo

  echo "--- status before kill ---"
  systemctl status "$SERVICE" --no-pager || true
  echo

  PID="$(pidof "$AGENT_NAME" | awk '{print $1}' || true)"
  echo "pid_before=$PID"

  if [ -z "$PID" ]; then
    echo "ERROR: agent is not running"
    exit 1
  fi

  echo "--- kill -9 daemon ---"
  sudo kill -9 "$PID" || true
  sleep 6
  echo

  echo "--- status after kill ---"
  systemctl status "$SERVICE" --no-pager || true
  echo

  echo "--- pid after restart ---"
  pidof "$AGENT_NAME" || true
  echo

  echo "--- recent journal ---"
  journalctl -u "$SERVICE" -n 120 --no-pager || true
  echo

  echo "Expected:"
  echo "- service should become active again because Restart=on-failure is configured."
} | tee "$OUT"

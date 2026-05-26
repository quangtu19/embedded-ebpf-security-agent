#!/usr/bin/env bash
set -euo pipefail

BASE="docs/evidence/khung10"
OUT="$BASE/benchmarks/idle_result.txt"
AGENT_NAME="ebpf-security-agent"

mkdir -p "$(dirname "$OUT")"

PID="$(pidof "$AGENT_NAME" | awk '{print $1}' || true)"

{
  echo "=== KHUNG 10 - CPU/RAM idle benchmark ==="
  date -Is
  echo

  if [ -z "${PID}" ]; then
    echo "ERROR: $AGENT_NAME is not running"
    echo "Run first: sudo systemctl start ebpf-security-agent"
    exit 1
  fi

  echo "PID=$PID"
  echo

  echo "--- ps RSS/VSZ/CPU/MEM ---"
  ps -o pid,ppid,%cpu,%mem,rss,vsz,comm -p "$PID" || true
  echo

  echo "--- top snapshot ---"
  top -b -n 1 -p "$PID" | head -n 20 || true
  echo

  echo "--- pidstat 1s x 30 ---"
  if command -v pidstat >/dev/null 2>&1; then
    pidstat -p "$PID" 1 30 || true
  else
    echo "SKIP: pidstat not found. Install: sudo apt install -y sysstat"
  fi

  echo
  echo "Target:"
  echo "- CPU idle overhead: < 5%"
  echo "- Memory RSS: < 100 MB"
} | tee "$OUT"

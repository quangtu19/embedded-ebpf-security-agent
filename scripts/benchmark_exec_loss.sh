#!/usr/bin/env bash
set -euo pipefail

BASE="docs/evidence/khung10"
OUT="$BASE/benchmarks/exec_loss_result.txt"
EXPECTED="${EXPECTED:-1000}"

mkdir -p "$(dirname "$OUT")"

find_log() {
  for p in \
    "/var/log/ebpf-security-agent/events.log" \
    "/opt/ebpf-security-agent/events.log" \
    "events.log"
  do
    if [ -f "$p" ]; then
      echo "$p"
      return 0
    fi
  done
  echo "/var/log/ebpf-security-agent/events.log"
}

LOG="$(find_log)"

count_exec() {
  grep -c 'PROC_EXEC' "$LOG" 2>/dev/null || true
}

{
  echo "=== KHUNG 10 - PROC_EXEC event loss benchmark ==="
  date -Is
  echo "log=$LOG"
  echo "expected=$EXPECTED"
  echo

  if [ ! -f "$LOG" ]; then
    echo "ERROR: log file not found"
    echo "Check service/config log_path first."
    exit 1
  fi

  BEFORE="$(count_exec)"
  echo "before=$BEFORE"

  for i in $(seq 1 "$EXPECTED"); do
    /bin/true
  done

  sleep 3

  AFTER="$(count_exec)"
  OBSERVED=$((AFTER - BEFORE))
  LOSS=$((EXPECTED - OBSERVED))
if [ "$LOSS" -lt 0 ]; then
  LOSS=0
fi

  echo "after=$AFTER"
  echo "observed=$OBSERVED"
  echo "loss=$LOSS"

  awk -v e="$EXPECTED" -v o="$OBSERVED" 'BEGIN {
    if (e <= 0) exit;
    rate=((e-o)/e)*100;
    if (rate < 0) rate=0;
    printf "loss_rate=%.2f%%\n", rate;
  }'

  echo
  echo "Target: loss_rate < 1-3%"
} | tee "$OUT"

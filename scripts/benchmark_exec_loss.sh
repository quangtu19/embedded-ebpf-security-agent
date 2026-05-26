#!/usr/bin/env bash
set -euo pipefail

LOG="/var/log/ebpf-security-agent/events.log"
OUT="docs/evidence/khung10/benchmarks/exec_loss_result.txt"
EXPECTED=1000

mkdir -p "$(dirname "$OUT")"

BEFORE=$(grep -c 'PROC_EXEC' "$LOG" 2>/dev/null || true)

for i in $(seq 1 "$EXPECTED"); do
  /bin/true
done

sleep 2

AFTER=$(grep -c 'PROC_EXEC' "$LOG" 2>/dev/null || true)
OBSERVED=$((AFTER - BEFORE))
LOSS=$((EXPECTED - OBSERVED))

{
  echo "=== PROC_EXEC event loss benchmark ==="
  date
  echo "expected=$EXPECTED"
  echo "observed=$OBSERVED"
  echo "loss=$LOSS"
  awk -v e="$EXPECTED" -v o="$OBSERVED" 'BEGIN { printf "loss_rate=%.2f%%\n", ((e-o)/e)*100 }'
} | tee "$OUT"
EOF

cat > scripts/benchmark_tcp_loss.sh <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

LOG="/var/log/ebpf-security-agent/events.log"
OUT="docs/evidence/khung10/benchmarks/tcp_loss_result.txt"
EXPECTED=100
TARGET="http://example.com"

mkdir -p "$(dirname "$OUT")"

BEFORE=$(grep -c 'TCP_CONNECT' "$LOG" 2>/dev/null || true)

for i in $(seq 1 "$EXPECTED"); do
  curl -s -o /dev/null --max-time 5 "$TARGET" || true
done

sleep 2

AFTER=$(grep -c 'TCP_CONNECT' "$LOG" 2>/dev/null || true)
OBSERVED=$((AFTER - BEFORE))
LOSS=$((EXPECTED - OBSERVED))

{
  echo "=== TCP_CONNECT event loss benchmark ==="
  date
  echo "target=$TARGET"
  echo "expected=$EXPECTED"
  echo "observed=$OBSERVED"
  echo "loss=$LOSS"
  awk -v e="$EXPECTED" -v o="$OBSERVED" 'BEGIN { printf "loss_rate=%.2f%%\n", ((e-o)/e)*100 }'
} | tee "$OUT"
EOF

cat > scripts/benchmark_xdp_counter.sh <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

OUT="docs/evidence/khung10/benchmarks/xdp_counter_result.txt"
mkdir -p "$(dirname "$OUT")"

{
  echo "=== XDP counter benchmark ==="
  date

  echo "--- stats before ---"
  sudo ./ebpf-agentctl stats || true

  echo "--- generate ping traffic ---"
  ping -c 20 8.8.8.8 || true

  sleep 2

  echo "--- stats after ---"
  sudo ./ebpf-agentctl stats || true

  echo "--- bpftool map show ---"
  sudo bpftool map show || true
} | tee "$OUT"
EOF

cat > scripts/test_recovery.sh <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

OUT="docs/evidence/khung10/recovery/recovery_result.txt"
mkdir -p "$(dirname "$OUT")"

{
  echo "=== Recovery test ==="
  date

  echo "--- status before kill ---"
  systemctl status ebpf-security-agent --no-pager || true

  PID=$(pidof ebpf-security-agent || true)
  echo "PID=$PID"

  if [ -n "$PID" ]; then
    sudo kill -9 "$PID"
  fi

  sleep 5

  echo "--- status after kill ---"
  systemctl status ebpf-security-agent --no-pager || true

  echo "--- recent journal ---"
  journalctl -u ebpf-security-agent -n 80 --no-pager || true
} | tee "$OUT"

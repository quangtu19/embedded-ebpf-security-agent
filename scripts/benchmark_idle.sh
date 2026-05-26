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
EOF

cat > scripts/benchmark_exec_loss.sh <<'EOF'
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
  echo "Note: If observed=0, current agent may not have PROC_EXEC monitor attached yet."
} | tee "$OUT"
EOF

cat > scripts/benchmark_tcp_loss.sh <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

BASE="docs/evidence/khung10"
OUT="$BASE/benchmarks/tcp_loss_result.txt"
EXPECTED="${EXPECTED:-100}"
TARGET="${TARGET:-http://example.com}"

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

count_tcp() {
  grep -c 'TCP_CONNECT' "$LOG" 2>/dev/null || true
}

{
  echo "=== KHUNG 10 - TCP_CONNECT event loss benchmark ==="
  date -Is
  echo "log=$LOG"
  echo "target=$TARGET"
  echo "expected=$EXPECTED"
  echo

  if [ ! -f "$LOG" ]; then
    echo "ERROR: log file not found"
    echo "Check service/config log_path first."
    exit 1
  fi

  BEFORE="$(count_tcp)"
  echo "before=$BEFORE"

  for i in $(seq 1 "$EXPECTED"); do
    curl -s -o /dev/null --max-time 5 "$TARGET" || true
  done

  sleep 3

  AFTER="$(count_tcp)"
  OBSERVED=$((AFTER - BEFORE))
  LOSS=$((EXPECTED - OBSERVED))

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
  echo "Note: If observed=0, current agent may not have TCP_CONNECT monitor attached yet."
} | tee "$OUT"
EOF

cat > scripts/benchmark_xdp_counter.sh <<'EOF'
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
EOF

cat > scripts/test_xdp_drop.sh <<'EOF'
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
EOF

cat > scripts/test_recovery.sh <<'EOF'
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
EOF

cat > scripts/collect_evidence.sh <<'EOF'
#!/usr/bin/env bash
set -euo pipefail

BASE="docs/evidence/khung10"
IFACE="${IFACE:-enp0s8}"

mkdir -p "$BASE"/{system,bpf,service,logs,benchmarks,network,recovery,reports}

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

copy_log_if_exists() {
  local src="$1"
  local dst="$2"
  if [ -f "$src" ]; then
    sudo cp "$src" "$dst" 2>/dev/null || true
  fi
}

echo "=== Collecting Khung 10 evidence into $BASE ==="

date -Is > "$BASE/system/date.txt"
uname -a > "$BASE/system/uname.txt" 2>&1 || true
lsb_release -a > "$BASE/system/lsb_release.txt" 2>&1 || true
hostnamectl > "$BASE/system/hostnamectl.txt" 2>&1 || true
clang --version > "$BASE/system/clang_version.txt" 2>&1 || true
bpftool version > "$BASE/system/bpftool_version.txt" 2>&1 || true
ip addr > "$BASE/system/ip_addr.txt" 2>&1 || true
ip route > "$BASE/system/ip_route.txt" 2>&1 || true

git rev-parse HEAD > "$BASE/system/git_commit.txt" 2>&1 || true
git status --short > "$BASE/system/git_status_short.txt" 2>&1 || true

sudo bpftool prog show > "$BASE/bpf/bpftool_prog_show.txt" 2>&1 || true
sudo bpftool map show > "$BASE/bpf/bpftool_map_show.txt" 2>&1 || true
sudo bpftool net > "$BASE/bpf/bpftool_net.txt" 2>&1 || true
ip link show > "$BASE/bpf/ip_link_show.txt" 2>&1 || true
ip -s link show dev "$IFACE" > "$BASE/bpf/ip_link_stats_${IFACE}.txt" 2>&1 || true

systemctl status ebpf-security-agent --no-pager > "$BASE/service/systemctl_status.txt" 2>&1 || true
systemctl cat ebpf-security-agent > "$BASE/service/systemctl_cat.txt" 2>&1 || true
journalctl -u ebpf-security-agent -n 300 --no-pager > "$BASE/service/journalctl_recent.txt" 2>&1 || true

copy_log_if_exists "/var/log/ebpf-security-agent/events.log" "$BASE/logs/events_var_log.log"
copy_log_if_exists "/opt/ebpf-security-agent/events.log" "$BASE/logs/events_opt.log"
copy_log_if_exists "events.log" "$BASE/logs/events_local.log"

cat "$BASE"/logs/events_*.log 2>/dev/null > "$BASE/logs/events_combined.log" || true

grep PROC_EXEC "$BASE/logs/events_combined.log" > "$BASE/logs/proc_exec_logs.txt" 2>/dev/null || true
grep TCP_CONNECT "$BASE/logs/events_combined.log" > "$BASE/logs/tcp_connect_logs.txt" 2>/dev/null || true
grep PACKET_STATS "$BASE/logs/events_combined.log" > "$BASE/logs/packet_stats_logs.txt" 2>/dev/null || true
grep -E 'XDP|BLACKLIST|DROPPED|DROP' "$BASE/logs/events_combined.log" > "$BASE/logs/xdp_drop_logs.txt" 2>/dev/null || true

run_ctl status > "$BASE/reports/agentctl_status.txt" 2>&1 || true
run_ctl stats > "$BASE/reports/agentctl_stats.txt" 2>&1 || true
run_ctl report > "$BASE/reports/agentctl_report.json" 2>&1 || true

echo "Evidence collected at $BASE" | tee "$BASE/README.txt"

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

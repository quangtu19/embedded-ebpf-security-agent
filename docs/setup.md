# Setup and Reproducibility Guide

This guide is written for the VM-based version of the eBPF/XDP Security Telemetry Agent.

## 1. Recommended environment

| Item | Recommended value |
|---|---|
| OS | Ubuntu 22.04/24.04 VM or Raspberry Pi OS 64-bit |
| Kernel | 5.15+ recommended, 6.x preferred |
| Compiler | `clang`, `llvm`, `gcc`, `make` |
| eBPF tooling | `libbpf-dev`, `bpftool`, `linux-headers-$(uname -r)` |
| XDP mode | `skb` / generic mode for VM labs |
| Privilege | root or equivalent capabilities required |

Check environment:

```bash
uname -a
lsb_release -a || cat /etc/os-release
bpftool version || true
clang --version
ip -br link
```

## 2. Install dependencies

```bash
sudo apt update
sudo apt install -y build-essential clang llvm gcc make pkg-config \
  libbpf-dev libelf-dev zlib1g-dev linux-headers-$(uname -r) \
  linux-tools-common linux-tools-generic bpftool iproute2 iputils-ping \
  curl netcat-openbsd tcpdump sysstat
```

Notes:

- If `linux-tools-generic` does not install a matching `bpftool`, install `bpftool` directly.
- On Raspberry Pi OS, package names can vary; keep `clang`, `llvm`, `libbpf-dev`, `libelf-dev`, `zlib1g-dev`, and kernel headers as the required set.

## 3. Build

```bash
make clean
make
```

Expected artifacts:

```text
ebpf-security-agent
bpf/exec_trace.bpf.o
bpf/tcp_connect.bpf.o
bpf/xdp_filter.bpf.o
```

## 4. Install

```bash
sudo ./scripts/install.sh
sudo systemctl daemon-reload
sudo systemctl enable --now ebpf-security-agent
sudo systemctl status ebpf-security-agent --no-pager
```

Deployment layout:

```text
/opt/ebpf-security-agent/bin/ebpf-security-agent
/opt/ebpf-security-agent/bin/ebpf-agentctl
/opt/ebpf-security-agent/bpf/*.bpf.o
/etc/ebpf-security-agent/agent.yaml
/var/log/ebpf-security-agent/events.log
/run/ebpf-security-agent/
/sys/fs/bpf/ebpf-security-agent/
```

## 5. Configure interface

Find the interface:

```bash
ip -br link
ip -4 addr
```

Edit config:

```bash
sudo nano /etc/ebpf-security-agent/agent.yaml
```

Example:

```yaml
agent:
  log_path: /var/log/ebpf-security-agent/events.log
  heartbeat_interval_sec: 5
xdp:
  xdp_enable: true
  xdp_interface: enp0s3
  xdp_mode: skb
  xdp_object_path: /opt/ebpf-security-agent/bpf/xdp_filter.bpf.o
  xdp_stats_interval_sec: 5
```

Restart:

```bash
sudo systemctl restart ebpf-security-agent
sudo journalctl -u ebpf-security-agent -n 80 --no-pager
```

## 6. Functional tests

### 6.1 Process execution

```bash
ls >/dev/null
whoami >/dev/null
python3 -c 'print("hello")'
sudo grep -E 'PROC_EXEC|exec' /var/log/ebpf-security-agent/events.log | tail -n 20
```

### 6.2 TCP connect

```bash
curl -I https://example.com || true
nc -vz example.com 80 || true
sudo grep -E 'TCP_CONNECT|tcp' /var/log/ebpf-security-agent/events.log | tail -n 20
```

### 6.3 XDP stats

```bash
ping -c 5 8.8.8.8 || true
sudo /opt/ebpf-security-agent/bin/ebpf-agentctl stats || sudo ./ebpf-agentctl stats
sudo bpftool prog show | grep -i xdp || true
sudo bpftool map show | grep -E 'stats|blacklist|drop|xdp' || true
```

### 6.4 XDP blacklist/drop, one-VM netns lab

```bash
sudo ./scripts/test_xdp_drop_netns.sh
cat docs/evidence/khung10/network/xdp_drop_netns_result.txt
```

Pass criteria:

- baseline ping from namespace succeeds;
- ping fails after adding namespace IP to blacklist;
- ping succeeds again after removing IP;
- result file contains `final_result=PASS`.

### 6.5 Recovery

```bash
sudo ./scripts/test_recovery.sh || true
sudo systemctl status ebpf-security-agent --no-pager
sudo journalctl -u ebpf-security-agent -n 80 --no-pager
```

## 7. Benchmark and evidence collection

Run available scripts:

```bash
sudo ./scripts/benchmark_idle.sh || true
sudo ./scripts/benchmark_exec_loss.sh || true
sudo ./scripts/benchmark_tcp_loss.sh || true
sudo ./scripts/benchmark_xdp_counter.sh || true
sudo ./scripts/test_xdp_drop_netns.sh || true
sudo ./scripts/test_recovery.sh || true
sudo ./scripts/collect_evidence.sh || true
```

Final helper:

```bash
sudo ./scripts/finalize_vm_completion.sh
```

Evidence paths:

```text
docs/evidence/khung10/benchmarks/
docs/evidence/khung10/network/
docs/evidence/khung10/recovery/
docs/results.md
```

## 8. Cleanup

Remove editor artifacts and backups:

```bash
./scripts/clean_repo_artifacts.sh
```

Detach XDP manually if needed:

```bash
sudo ./scripts/reset_xdp.sh enp0s3 || true
```

or:

```bash
sudo ip link set dev enp0s3 xdpgeneric off 2>/dev/null || true
sudo ip link set dev enp0s3 xdp off 2>/dev/null || true
```

## 9. Troubleshooting

### Permission denied / EPERM

Run with root or install the systemd service. eBPF/XDP requires privileged operations.

### Interface not found

Check `ip -br link` and update `xdp_interface` in `/etc/ebpf-security-agent/agent.yaml`.

### XDP attach failed

Use `xdp_mode: skb` in VM labs. Native XDP depends on driver/NIC support.

### No blacklist map

The current binary may not have loaded or pinned the XDP blacklist map. Restart service and check:

```bash
sudo systemctl restart ebpf-security-agent
sudo bpftool map show | grep -i blacklist || true
```

### Ping still passes after blacklist

Confirm the source IP being blacklisted is the actual packet source. For reproducible one-VM testing, use `scripts/test_xdp_drop_netns.sh`.

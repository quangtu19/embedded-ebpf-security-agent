# eBPF/XDP Security Telemetry Agent for Embedded Linux

A lightweight VM/embedded-Linux security telemetry agent implemented in C with eBPF/XDP.

The project demonstrates an end-to-end Linux system programming workflow:

- load eBPF programs from a user-space daemon;
- trace process execution events;
- trace IPv4 TCP connect events;
- attach an XDP program to a configured interface;
- count ingress packets by protocol;
- maintain an IPv4 source blacklist in a BPF map;
- write structured telemetry logs;
- run as a systemd service;
- collect reproducible benchmark and test evidence.

> Scope statement: this is a VM/embedded-Linux security telemetry prototype, not a commercial IDS/IPS/EDR. It focuses on kernel/userspace interaction, observability, controlled XDP blacklist/drop behavior, service operation, and evaluation evidence.

## Project status

| Area | Status | Evidence / command |
|---|---:|---|
| Build with `make` | implemented | `make clean && make` |
| Process execution telemetry | implemented | `PROC_EXEC` logs after running commands |
| TCP connect telemetry | implemented | `TCP_CONNECT` logs after `curl`/`nc` |
| XDP packet counters | implemented | `ebpf-agentctl stats`, `bpftool map dump` |
| XDP IPv4 source blacklist | implemented, must be validated per VM | `scripts/test_xdp_drop_netns.sh` |
| systemd service | implemented | `systemctl status ebpf-security-agent` |
| benchmark evidence | implemented | `docs/results.md`, `docs/evidence/khung10/` |
| repo documentation | completed by this overlay | `README.md`, `docs/setup.md`, `docs/architecture.md` |

The only result that must be generated on the actual VM is XDP drop accuracy, because it depends on the tested interface, XDP mode, kernel, and lab topology. Use the automated network-namespace test below to generate this evidence on one Ubuntu VM.

## Repository layout

```text
embedded-ebpf-security-agent/
├── bpf/                         # eBPF/XDP kernel programs
│   ├── common.bpf.h
│   ├── exec_trace.bpf.c
│   ├── tcp_connect.bpf.c
│   └── xdp_filter.bpf.c
├── include/                     # user-space headers
├── src/                         # C user-space daemon and controllers
├── config/agent.yaml            # runtime configuration
├── scripts/                     # install, tests, benchmarks, evidence collection
├── systemd/                     # systemd unit file
├── docs/                        # architecture, setup, results, evidence
├── Makefile
└── README.md
```

## Quick start on Ubuntu VM

Install dependencies:

```bash
sudo apt update
sudo apt install -y build-essential clang llvm libbpf-dev libelf-dev zlib1g-dev \
  linux-headers-$(uname -r) linux-tools-common linux-tools-generic bpftool \
  pkg-config iproute2 iputils-ping curl netcat-openbsd tcpdump sysstat
```

Build:

```bash
make clean
make
```

Install and start service:

```bash
sudo ./scripts/install.sh
sudo systemctl daemon-reload
sudo systemctl enable --now ebpf-security-agent
sudo systemctl status ebpf-security-agent --no-pager
```

Check logs:

```bash
sudo journalctl -u ebpf-security-agent -n 80 --no-pager
sudo tail -n 80 /var/log/ebpf-security-agent/events.log
```

Generate telemetry:

```bash
ls >/dev/null
whoami >/dev/null
curl -I https://example.com || true
sudo /opt/ebpf-security-agent/bin/ebpf-agentctl stats || sudo ./ebpf-agentctl stats
```

## Configure the XDP interface

The default config may use a VM-specific interface such as `enp0s3`. Always check your interface first:

```bash
ip -br link
ip -4 addr
```

Edit `/etc/ebpf-security-agent/agent.yaml` after installation:

```yaml
xdp:
  xdp_enable: true
  xdp_interface: enp0s3   # change this to your real interface
  xdp_mode: skb           # generic/SKB mode is the safest for VM labs
```

Restart:

```bash
sudo systemctl restart ebpf-security-agent
sudo journalctl -u ebpf-security-agent -n 80 --no-pager
```

## Final one-VM XDP drop validation

Run the network-namespace lab test from the repository root:

```bash
sudo ./scripts/test_xdp_drop_netns.sh
```

What it does:

1. creates a temporary veth pair and network namespace;
2. points the agent config to the lab interface;
3. restarts the systemd service;
4. confirms ping works before blacklist;
5. adds the namespace IP to the blacklist map;
6. confirms ping fails while blacklisted;
7. removes the IP from the blacklist;
8. confirms ping works again;
9. writes evidence to `docs/evidence/khung10/network/xdp_drop_netns_result.txt`;
10. updates `docs/results.md` from `SKIP/PENDING` to `PASS` only when the test passes.

## Final completion run

After cloning or after applying this overlay:

```bash
chmod +x scripts/*.sh
sudo ./scripts/finalize_vm_completion.sh
```

This script cleans repo artifacts, builds the project, checks service state, runs available benchmark scripts, runs the XDP netns drop test, and collects evidence when the matching script exists.

## Expected benchmark targets

| Metric | Target |
|---|---:|
| CPU idle overhead | < 5% |
| RSS memory | < 100 MB |
| PROC_EXEC loss | < 1-3% |
| TCP_CONNECT loss | < 1-3% |
| XDP counter consistency | counters increase consistently |
| XDP drop accuracy | 100% in controlled lab |
| Recovery | systemd restarts service after crash |

## Limitations

- IPv4-only blacklist/drop in the MVP.
- No payload inspection, DPI, TLS/domain visibility, or malware classification.
- Not resistant to a fully privileged root/kernel attacker.
- XDP behavior depends on kernel, driver, NIC, interface, VM mode, and XDP mode.
- High burst rates can still cause ring-buffer loss if the daemon cannot consume events fast enough.
- Results on a VM or Raspberry Pi do not automatically generalize to all embedded Linux boards.

## Suggested report sentence

> This project implements a VM/embedded-Linux eBPF/XDP security telemetry agent with process execution tracing, TCP connect tracing, XDP packet counters, IPv4 source blacklist/drop support, structured logs, systemd deployment, and reproducible benchmark evidence. It is a security telemetry prototype rather than a full IDS/IPS/EDR.

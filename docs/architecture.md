# Architecture

## 1. System purpose

The agent is a small security telemetry daemon for Linux VM/embedded nodes. It combines kernel-space eBPF/XDP programs with a C user-space daemon to observe process execution, TCP outbound connections, ingress packet counters, and controlled IPv4 source blacklist/drop behavior.

The system is intentionally scoped as a prototype security telemetry agent, not as a full IDS/IPS/EDR.

## 2. Context view

```text
+------------------+        traffic / activity        +-------------------------+
| Client / netns   | -------------------------------> | Linux VM / embedded node |
| Test workload    |                                  | Ubuntu / Raspberry Pi    |
+------------------+                                  +------------+------------+
                                                                  |
                                                                  | eBPF/XDP telemetry
                                                                  v
                                                        +----------------------+
                                                        | ebpf-security-agent  |
                                                        | C user-space daemon  |
                                                        +----------+-----------+
                                                                   |
                                                                   | logs / status / evidence
                                                                   v
                                                        +----------------------+
                                                        | events.log / journal |
                                                        | docs/evidence/...    |
                                                        +----------------------+
```

## 3. Component view

```text
User space
---------

+---------------------------------------------------------------------+
| ebpf-security-agent daemon                                           |
|                                                                     |
|  +----------------+     +----------------+     +----------------+   |
|  | config_loader  | --> | ebpf_loader    | --> | trace_monitor  |   |
|  | agent.yaml     |     | libbpf attach  |     | ringbuf events |   |
|  +----------------+     +----------------+     +----------------+   |
|          |                       |                      |            |
|          v                       v                      v            |
|  +----------------+     +----------------+     +----------------+   |
|  | xdp_controller | <-> | BPF maps       | --> | event_logger   |   |
|  | stats/blacklist|     | stats/blacklist|     | JSON/log file  |   |
|  +----------------+     +----------------+     +----------------+   |
|                                                                     |
+---------------------------------------------------------------------+

Kernel space
------------

+---------------------------------------------------------------------+
| eBPF trace program: process execution                               |
| eBPF trace/kprobe program: IPv4 TCP connect                          |
| XDP program: protocol counters + IPv4 source blacklist/drop          |
| BPF maps: ring buffer, stats maps, blacklist map, drop counters      |
+---------------------------------------------------------------------+
```

## 4. Data-flow view

| Flow | Kernel source | Kernel artifact | User-space path | Output |
|---|---|---|---|---|
| Process execution | `execve` / sched exec hook | `exec_trace.bpf.c` | ring buffer -> trace monitor -> logger | `PROC_EXEC` log |
| TCP connect | `tcp_v4_connect` / TCP hook | `tcp_connect.bpf.c` | ring buffer -> trace monitor -> logger | `TCP_CONNECT` log |
| Packet count | ingress packets | `xdp_filter.bpf.c` | stats map -> xdp controller | `PACKET_STATS` / CLI stats |
| Source blacklist/drop | ingress IPv4 packets | `blacklist_map` lookup -> `XDP_DROP` | map update through CLI/controller | drop counter, drop evidence |
| Service operation | systemd | service restart/health | journal + log file | recovery evidence |

## 5. Startup sequence

```text
systemd starts service
        |
        v
mount /sys/fs/bpf if needed
        |
        v
create /sys/fs/bpf/ebpf-security-agent, /var/log/..., /run/...
        |
        v
daemon parses /etc/ebpf-security-agent/agent.yaml
        |
        v
load eBPF objects through libbpf
        |
        +--> attach process trace program
        +--> attach TCP connect trace program
        +--> attach XDP program to configured interface
        |
        v
open ring buffer and map readers
        |
        v
write AGENT_STARTED / heartbeat / packet stats
```

## 6. XDP blacklist/drop sequence

```text
Admin or test script:
  ebpf-agentctl blacklist add 10.200.1.2
        |
        v
User-space controller updates blacklist_map
        |
        v
Packet enters XDP hook on configured interface
        |
        v
XDP parses Ethernet + IPv4 headers with bounds checks
        |
        v
XDP looks up IPv4 source address in blacklist_map
        |
        +--> match: increment drop counter, return XDP_DROP
        |
        +--> no match: increment pass/protocol counters, return XDP_PASS
```

## 7. State and failure handling

| Failure | Expected state | Expected behavior |
|---|---|---|
| invalid config | `CRITICAL` | log error and exit non-zero |
| missing interface | `CRITICAL` or `DEGRADED` | log attach failure, do not silently claim XDP visibility |
| BPF verifier/load failure | `CRITICAL` | log libbpf/verifier error |
| process/TCP attach failure | `DEGRADED` or `CRITICAL` | continue only if remaining modules are useful |
| XDP attach failure | `DEGRADED` | process/TCP telemetry may continue |
| map update failure | `WARNING` | CLI returns error; daemon does not crash |
| daemon killed | recovery through systemd | service restarts and logs startup again |

## 8. Security boundary

Allowed claim:

> The agent provides telemetry and lab-controlled XDP source-IP drop evidence for Linux VM/embedded nodes.

Not allowed claim:

> The agent is a complete IDS/IPS/EDR or can stop all attacks.

## 9. Evaluation design

| Test ID | Goal | Evidence |
|---|---|---|
| TC01 | service boot | `systemctl status`, `journalctl` |
| TC02 | process exec telemetry | `PROC_EXEC` logs |
| TC03 | TCP connect telemetry | `TCP_CONNECT` logs |
| TC04 | XDP packet counters | CLI stats, BPF map dump |
| TC05 | blacklist add/drop | `test_xdp_drop_netns.sh` evidence |
| TC06 | blacklist remove/recovery | ping passes after remove |
| TC07 | wrong interface failure | journal + state log |
| TC08 | daemon crash recovery | systemd restart evidence |
| TC09 | CPU/RAM overhead | benchmark output |
| TC10 | event loss | benchmark output |

## 10. Final implementation boundary

This repository version is considered complete for VM-based coursework when:

1. it builds from a clean clone;
2. service starts under systemd;
3. process and TCP logs are generated;
4. XDP counters increase under traffic;
5. blacklist/drop test passes in a controlled netns or two-VM lab;
6. benchmark evidence is stored under `docs/evidence/khung10/`;
7. `docs/results.md` records all PASS/PENDING results honestly;
8. repo artifacts such as `.swp` and `.bak` files are removed.

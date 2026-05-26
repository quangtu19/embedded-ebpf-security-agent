# Khung 10 - Benchmark, Evaluation & Evidence Results

## Environment

| Field | Value |
|---|---|
| Repository commit | TBD |
| OS | TBD |
| Kernel | TBD |
| Interface | enp0s8 / TBD |
| XDP mode | skb / generic |
| Service | ebpf-security-agent |
| Date | TBD |

## Result table

| Experiment | Environment | Metric | Result | Target | Pass/Fail | Evidence | Note |
|---|---|---:|---:|---:|---|---|---|
| EXP-01 | Ubuntu VM target | CPU idle overhead | TBD | < 5% | TBD | docs/evidence/khung10/benchmarks/idle_result.txt | pidstat 30s |
| EXP-02 | Ubuntu VM target | RSS memory | TBD | < 100 MB | TBD | docs/evidence/khung10/benchmarks/idle_result.txt | ps/top |
| EXP-03 | Ubuntu VM target | PROC_EXEC loss | TBD | < 1-3% | TBD | docs/evidence/khung10/benchmarks/exec_loss_result.txt | 1000 /bin/true |
| EXP-04 | Ubuntu VM target | TCP_CONNECT loss | TBD | < 1-3% | TBD | docs/evidence/khung10/benchmarks/tcp_loss_result.txt | 100 curl |
| EXP-05 | Ubuntu VM target | XDP counter consistency | TBD | counter increases | TBD | docs/evidence/khung10/benchmarks/xdp_counter_result.txt | ping workload |
| EXP-06 | VM target + client | XDP drop accuracy | TBD | 100% in lab | TBD | docs/evidence/khung10/network/xdp_drop_result.txt | requires client IP |
| EXP-07 | Ubuntu VM target | Recovery | TBD | service restarts | TBD | docs/evidence/khung10/recovery/recovery_result.txt | kill -9 daemon |

## Pass/fail interpretation

- PASS: metric meets target and evidence file is present.
- FAIL: metric does not meet target.
- TBD: script has not been run yet.
- SKIP: module is not implemented in current build, or lab condition is missing.

## Known limitations

- Current public build may only include XDP counter; PROC_EXEC/TCP_CONNECT benchmarks require process/TCP eBPF modules to be attached.
- XDP drop test requires blacklist_map implementation and a second client machine/source IP.
- Results on VM are not automatically generalizable to Raspberry Pi or other embedded devices.
- XDP mode, NIC driver, interface name and kernel version can affect behavior.

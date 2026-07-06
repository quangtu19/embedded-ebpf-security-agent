# Benchmark Results

This file records the VM-level benchmark/evidence status for the eBPF/XDP Security Telemetry Agent.

The existing VM evidence shows that the core telemetry path is working. XDP drop accuracy must be validated on the actual VM because it depends on interface name, XDP mode, kernel, BPF map state, and lab topology.

| Experiment | Metric | Result | Target | Pass/Fail | Evidence |
|---|---|---:|---:|---|---|
| EXP-01 | CPU idle overhead | 0.00% | < 5% | PASS | `docs/evidence/khung10/benchmarks/idle_result.txt` |
| EXP-02 | RSS memory | 2.43 MB | < 100 MB | PASS | `docs/evidence/khung10/benchmarks/idle_result.txt` |
| EXP-03 | PROC_EXEC loss | observed=1003/1000, loss_rate=0.00% | < 1-3% | PASS | `docs/evidence/khung10/benchmarks/exec_loss_result.txt` |
| EXP-04 | TCP_CONNECT loss | observed=100/100, loss_rate=0.00% | < 1-3% | PASS | `docs/evidence/khung10/benchmarks/tcp_loss_result.txt` |
| EXP-05 | XDP counter consistency | total=20, icmp=19, other=1, bytes=1926 | counter increases | PASS | `docs/evidence/khung10/benchmarks/xdp_counter_result.txt` |
| EXP-06 | XDP drop accuracy | pending rerun with `scripts/test_xdp_drop_netns.sh` | 100% lab | PENDING | `docs/evidence/khung10/network/xdp_drop_netns_result.txt` |
| EXP-07 | Recovery | service restarted after kill -9 | restart ok | PASS | `docs/evidence/khung10/recovery/recovery_result.txt` |

## Analysis

CPU usage and memory footprint are below the VM/embedded prototype targets. Process execution and TCP connect telemetry pass the current benchmark logs. The process benchmark can observe slightly more events than the synthetic target because the shell and benchmark script can spawn helper processes; this does not indicate event loss when the calculated loss rate is 0.00%.

The remaining completion point is EXP-06. Run:

```bash
sudo ./scripts/test_xdp_drop_netns.sh
```

When the result file contains `final_result=PASS`, the script automatically updates the EXP-06 row to PASS.

## Required final evidence package

```text
docs/evidence/khung10/benchmarks/idle_result.txt
docs/evidence/khung10/benchmarks/exec_loss_result.txt
docs/evidence/khung10/benchmarks/tcp_loss_result.txt
docs/evidence/khung10/benchmarks/xdp_counter_result.txt
docs/evidence/khung10/network/xdp_drop_netns_result.txt
docs/evidence/khung10/recovery/recovery_result.txt
```

## Honest completion statement

The project is complete for VM-based demonstration when EXP-06 is PASS on the target VM. Until that command is run successfully on the VM, documentation and scripts are complete but final drop evidence remains pending.

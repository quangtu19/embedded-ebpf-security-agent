# Khung 10 - Benchmark Results

| Experiment | Metric | Result | Target | Pass/Fail | Evidence |
|---|---|---:|---:|---|---|
| EXP-01 | CPU idle overhead | 0.00% | < 5% | PASS | docs/evidence/khung10/benchmarks/idle_result.txt |
| EXP-02 | RSS memory | 2.43 MB | < 100 MB | PASS | docs/evidence/khung10/benchmarks/idle_result.txt |
| EXP-03 | PROC_EXEC loss | observed=1003/1000, loss_rate=0.00% | < 1-3% | PASS | docs/evidence/khung10/benchmarks/exec_loss_result.txt |
| EXP-04 | TCP_CONNECT loss | observed=100/100, loss_rate=0.00% | < 1-3% | PASS | docs/evidence/khung10/benchmarks/tcp_loss_result.txt |
| EXP-05 | XDP counter consistency | total=20, icmp=19, other=1, bytes=1926 | counter increases | PASS | docs/evidence/khung10/benchmarks/xdp_counter_result.txt |
| EXP-06 | XDP drop accuracy | not tested - no separate client host | 100% lab | SKIP | docs/evidence/khung10/network/xdp_drop_result.txt |
| EXP-07 | Recovery | service restarted after kill -9 | restart ok | PASS | docs/evidence/khung10/recovery/recovery_result.txt |

## Analysis

The benchmark results show that the agent has very low idle overhead. CPU usage remained approximately 0.00%, and RSS memory was approximately 2.43 MB, far below the 100 MB target.

PROC_EXEC telemetry passed the burst benchmark. The benchmark generated 1000 process executions and observed 1003 PROC_EXEC events. The observed count can be slightly higher because the shell and benchmark script may create additional process events. The measured loss rate is 0.00%.

TCP_CONNECT telemetry passed the connection benchmark. The benchmark generated 100 outbound TCP connection attempts to http://1.1.1.1 and observed 100 TCP_CONNECT events, giving a 0.00% loss rate.

XDP counter consistency passed after correcting the monitored interface to enp0s3. The XDP stats recorded total=20 packets, icmp=19, other=1, and bytes=1926 during the ping workload.

Recovery passed. After the daemon was killed with SIGKILL, systemd restarted the service automatically and returned it to active running state.

## Known limitations

XDP drop accuracy was not fully tested because it requires a separate client host with a known source IP to verify blacklist add/drop/remove behavior in a controlled lab setup.

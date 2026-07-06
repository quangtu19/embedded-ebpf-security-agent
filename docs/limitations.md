# Limitations and Validity Threats

## Limitations

- The project is a security telemetry prototype, not a complete IDS/IPS/EDR.
- The MVP supports IPv4 source blacklist/drop only.
- The agent does not inspect payloads, TLS contents, or domain names.
- The agent does not prevent a fully privileged root or kernel-level attacker from disabling the service or modifying logs.
- XDP attach behavior depends on kernel version, driver, NIC, VM platform, and selected XDP mode.
- Generic/SKB XDP mode is recommended for VM reproducibility, while native XDP is driver-dependent.
- Command-line capture in eBPF may be truncated or intentionally limited for verifier safety.
- Ring-buffer event loss can still occur under extreme bursts if the user-space daemon cannot consume events quickly enough.
- VM benchmark values do not generalize to every embedded Linux board.

## Validity threats

| Threat | Description | Mitigation |
|---|---|---|
| Internal validity | Test scripts can spawn helper processes and affect event counts. | Report observed/expected counts and explain extra events. |
| External validity | VM results may not represent all Raspberry Pi or ARM boards. | Record kernel, OS, architecture, and XDP mode in evidence. |
| Construct validity | CPU/RAM/loss metrics do not fully represent security value. | Combine benchmarks with functional telemetry and drop evidence. |
| Reliability | Results depend on config, interface, root privileges, and map state. | Use reproducible scripts and store raw command output. |

## Future work

- IPv6 telemetry and blacklist support.
- Stronger CO-RE portability across kernel versions.
- eBPF file-write tracing to attribute config changes to processes.
- Prometheus exporter or lightweight dashboard.
- Rule engine for richer anomaly detection.
- SIEM/export format integration.
- Native XDP benchmarking on supported hardware.

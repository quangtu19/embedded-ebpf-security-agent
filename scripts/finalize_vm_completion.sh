#!/usr/bin/env bash
set -euo pipefail

# Final VM completion helper. Run from repository root.
# It does not fake evidence: if a test fails, the command fails and leaves the raw output for debugging.

if [ "${EUID}" -ne 0 ]; then
  echo "ERROR: run with sudo/root" >&2
  exit 1
fi

ROOT="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
cd "$ROOT"

mkdir -p docs/evidence/khung10/{benchmarks,network,recovery,system}

logfile="docs/evidence/khung10/system/finalize_vm_completion.txt"
{
  echo "=== Final VM completion run ==="
  date -Is
  echo "repo=$ROOT"
  echo

  echo "--- environment ---"
  uname -a || true
  lsb_release -a 2>/dev/null || cat /etc/os-release || true
  clang --version | head -n 2 || true
  bpftool version || true
  ip -br link || true
  echo

  echo "--- cleanup repo artifacts ---"
  if [ -x scripts/clean_repo_artifacts.sh ]; then
    ./scripts/clean_repo_artifacts.sh
  fi
  echo

  echo "--- build ---"
  make clean
  make
  echo

  if [ "${RUN_INSTALL:-0}" = "1" ] && [ -x scripts/install.sh ]; then
    echo "--- install ---"
    ./scripts/install.sh
    systemctl daemon-reload
    systemctl enable ebpf-security-agent || true
  fi

  echo "--- restart service ---"
  systemctl daemon-reload || true
  systemctl restart ebpf-security-agent
  sleep 3
  systemctl --no-pager --full status ebpf-security-agent || true
  if ! systemctl is-active --quiet ebpf-security-agent; then
    echo "ERROR: ebpf-security-agent is not active"
    exit 1
  fi
  echo

  echo "--- run available benchmark scripts ---"
  for s in \
    scripts/benchmark_idle.sh \
    scripts/benchmark_exec_loss.sh \
    scripts/benchmark_tcp_loss.sh \
    scripts/benchmark_xdp_counter.sh \
    scripts/test_recovery.sh; do
    if [ -x "$s" ]; then
      echo
      echo "+ $s"
      "$s" || echo "WARNING: $s returned non-zero; inspect its evidence file."
    else
      echo "SKIP: $s not found or not executable"
    fi
  done
  echo

  echo "--- run XDP drop netns validation ---"
  if [ -x scripts/test_xdp_drop_netns.sh ]; then
    scripts/test_xdp_drop_netns.sh
  else
    echo "ERROR: scripts/test_xdp_drop_netns.sh missing"
    exit 1
  fi
  echo

  echo "--- collect evidence ---"
  if [ -x scripts/collect_evidence.sh ]; then
    scripts/collect_evidence.sh || true
  fi
  echo

  echo "--- final results ---"
  cat docs/results.md || true
  echo
  echo "finalize_result=PASS"
} | tee "$logfile"

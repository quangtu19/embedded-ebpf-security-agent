#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

sudo mkdir -p /opt/ebpf-security-agent/bin
sudo mkdir -p /opt/ebpf-security-agent/bpf
sudo mkdir -p /etc/ebpf-security-agent
sudo mkdir -p /var/log/ebpf-security-agent
sudo mkdir -p /run/ebpf-security-agent
sudo mkdir -p /sys/fs/bpf/ebpf-security-agent || true

sudo cp "$PROJECT_ROOT/ebpf-security-agent" /opt/ebpf-security-agent/bin/

if [ -f "$PROJECT_ROOT/ebpf-agentctl" ]; then
  sudo cp "$PROJECT_ROOT/ebpf-agentctl" /opt/ebpf-security-agent/bin/
fi

sudo cp "$PROJECT_ROOT"/bpf/*.o /opt/ebpf-security-agent/bpf/ 2>/dev/null || true
sudo cp "$PROJECT_ROOT/config/agent.yaml" /etc/ebpf-security-agent/agent.yaml
sudo cp "$PROJECT_ROOT/systemd/ebpf-security-agent.service" /etc/systemd/system/ebpf-security-agent.service

sudo chmod +x /opt/ebpf-security-agent/bin/ebpf-security-agent

if [ -f /opt/ebpf-security-agent/bin/ebpf-agentctl ]; then
  sudo chmod +x /opt/ebpf-security-agent/bin/ebpf-agentctl
fi

sudo chmod 755 /var/log/ebpf-security-agent
sudo systemctl daemon-reload

echo "[OK] Install completed"
echo "Run: sudo systemctl enable --now ebpf-security-agent"

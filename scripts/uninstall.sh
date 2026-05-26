#!/usr/bin/env bash
set -euo pipefail

sudo systemctl stop ebpf-security-agent 2>/dev/null || true
sudo systemctl disable ebpf-security-agent 2>/dev/null || true

sudo rm -f /etc/systemd/system/ebpf-security-agent.service
sudo systemctl daemon-reload

sudo rm -rf /opt/ebpf-security-agent
sudo rm -rf /etc/ebpf-security-agent
sudo rm -rf /run/ebpf-security-agent

echo "[OK] Uninstall completed"
echo "Logs are kept at /var/log/ebpf-security-agent"

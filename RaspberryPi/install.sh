
#!/usr/bin/env bash
set -euo pipefail

if (( EUID == 0 )); then
  echo "Run this installer as your normal Pi user, not with sudo." >&2
  exit 1
fi

PROJECT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
PI_DIR="$PROJECT_ROOT/RaspberryPi"
SERVICE_FILE="/etc/systemd/system/powermesh.service"

echo "Installing PowerMesh from $PROJECT_ROOT"

sudo apt-get update
sudo apt-get install -y python3-venv iproute2 kmod

python3 -m venv "$PI_DIR/.venv"
"$PI_DIR/.venv/bin/python" -m pip install -r "$PI_DIR/requirements.txt"

# The service runs as root for TUN and UART access.
sudo tee "$SERVICE_FILE" >/dev/null <<EOF_SERVICE
[Unit]
Description=PowerMesh TUN to STM32 UART bridge
After=network.target

[Service]
Type=simple
WorkingDirectory=$PI_DIR
Environment=PYTHONUNBUFFERED=1
EnvironmentFile=-/etc/default/powermesh
ExecStartPre=/bin/bash $PI_DIR/setup_tun.sh
ExecStart=/bin/bash $PI_DIR/start_bridge.sh
Restart=always
RestartSec=3

[Install]
WantedBy=multi-user.target
EOF_SERVICE

sudo systemctl daemon-reload
sudo systemctl enable powermesh.service
sudo systemctl restart powermesh.service

echo
echo "PowerMesh boot service installed."
echo "Status: sudo systemctl status powermesh"
echo "Logs:   sudo journalctl -u powermesh -f"

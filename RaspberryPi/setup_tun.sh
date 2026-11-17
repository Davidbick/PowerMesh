#!/usr/bin/env bash
set -euo pipefail

TUN_NAME="powermesh0"
TUN_ADDRESS="fd00:706d::781d:ff:fe00:0/64"
REMOTE_TEST_ADDRESS="fd00:706d::781d:ff:fe00:1"
MTU=1280

if (( EUID != 0 )); then
  echo "Run this script as root (the PowerMesh service does this automatically)." >&2
  exit 1
fi

modprobe tun

if ! ip link show dev "$TUN_NAME" >/dev/null 2>&1; then
  ip tuntap add dev "$TUN_NAME" mode tun
fi

ip link set dev "$TUN_NAME" mtu "$MTU"
# Remove the old prototype address when upgrading.
ip -6 addr del "fd00:706d::1/64" dev "$TUN_NAME" 2>/dev/null || true
ip -6 addr replace "$TUN_ADDRESS" dev "$TUN_NAME"
ip link set dev "$TUN_NAME" up

echo "PowerMesh TUN configured: $TUN_NAME ($TUN_ADDRESS, MTU $MTU)"
ip -6 route get "$REMOTE_TEST_ADDRESS"

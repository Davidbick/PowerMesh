#!/usr/bin/env bash

set -euo pipefail

TUN_NAME="powermesh0"
TUN_ADDRESS="fd00:706d::1/64"
MTU="1280"

USER_NAME="$(id -un)"

echo "Loading TUN driver..."
sudo modprobe tun

if ! ip link show "$TUN_NAME" >/dev/null 2>&1; then
    echo "Creating $TUN_NAME..."
    sudo ip tuntap add \
        dev "$TUN_NAME" \
        mode tun \
        user "$USER_NAME"
else
    echo "$TUN_NAME already exists"
fi

echo "Setting MTU to $MTU..."
sudo ip link set \
    dev "$TUN_NAME" \
    mtu "$MTU"

echo "Assigning IPv6 address $TUN_ADDRESS..."
sudo ip -6 addr replace \
    "$TUN_ADDRESS" \
    dev "$TUN_NAME"

echo "Bringing $TUN_NAME up..."
sudo ip link set \
    dev "$TUN_NAME" \
    up

echo
echo "PowerMesh TUN interface:"
ip -6 addr show dev "$TUN_NAME"

echo
echo "Route test for remote PowerMesh node:"
ip -6 route get fd00:706d::2
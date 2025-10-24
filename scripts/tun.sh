#!/bin/bash

# CS144 TUN device setup script
# This script sets up a TUN device for the Minnow TCP implementation

TUN_NAME="${1:-tun144}"
TUN_IP_PREFIX="${2:-169.254.144}"

# Check if running as root
if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root (use sudo)"
   exit 1
fi

# Create TUN device if it doesn't exist
if ! ip link show "$TUN_NAME" &> /dev/null; then
    echo "Creating TUN device: $TUN_NAME"
    ip tuntap add mode tun user "${SUDO_USER:-$USER}" name "$TUN_NAME"
else
    echo "TUN device $TUN_NAME already exists"
fi

# Configure the TUN device
echo "Configuring $TUN_NAME with IP ${TUN_IP_PREFIX}.1"
ip addr flush dev "$TUN_NAME"
ip addr add "${TUN_IP_PREFIX}.1/24" dev "$TUN_NAME"
ip link set "$TUN_NAME" up

# Add routing (optional, for specific destinations)
# ip route add 34.92.30.82 dev "$TUN_NAME" 2>/dev/null || true

echo "TUN device $TUN_NAME is ready"
echo "  IP address: ${TUN_IP_PREFIX}.1/24"
echo "  Status: $(ip link show "$TUN_NAME" | grep -o 'state [A-Z]*')"


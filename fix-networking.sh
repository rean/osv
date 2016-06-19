#!/bin/bash

mkdir -p /etc/qemu
touch /etc/qemu/bridge.conf
echo "allow virbr0" >> /etc/qemu/bridge.conf
# Add so you can start osv with networking: "scripts/run.py -n" without sudo
chmod u+s /usr/lib/qemu/qemu-bridge-helper

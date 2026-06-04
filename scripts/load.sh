#!/bin/bash

cd "$(dirname "$0")/../kernel" || exit 1

make clean
make

sudo rm -f /dev/mychardev
sudo insmod mychardev.ko

echo "Module loaded successfully."
ls -l /dev/mychardev
sudo dmesg | tail -n 10

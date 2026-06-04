#!/bin/bash

sudo rmmod mychardev

echo "Module unloaded successfully."
sudo dmesg | tail -n 10

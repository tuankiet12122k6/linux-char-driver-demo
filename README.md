# Linux Character Device Driver Demo

## 1. Project Overview

This project implements a simple Linux Kernel Module that creates a character device named `/dev/mychardev`.

The driver supports:

- `open()`
- `release()`
- `read()`
- `write()`
- `ioctl()`
- sysfs attribute
- mutex synchronization
- basic error handling

The purpose of this project is to demonstrate communication between user-space and kernel-space in Linux through a character device driver.

## 2. Directory Structure

```text
linux-char-driver-demo/
├── kernel/
│   ├── mychardev.c
│   └── Makefile
├── user/
│   ├── test_read_write.c
│   ├── test_ioctl.c
│   └── mychardev_ioctl.h
├── scripts/
│   ├── load.sh
│   └── unload.sh
├── docs/
│   └── report.md
└── README.md

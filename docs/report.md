# Report: Linux Character Device Driver

## 1. Introduction

This project implements a Linux Kernel Module that creates a simple character device driver named `/dev/mychardev`.

The driver allows user-space programs to communicate with kernel-space through standard Linux file operations such as `open()`, `read()`, `write()`, and `ioctl()`.

The project also includes sysfs support, mutex synchronization, and basic error handling.

## 2. Project Objectives

The main objectives of this project are:

* Build a simple Linux Kernel Module.
* Create a character device named `/dev/mychardev`.
* Allow user-space to read and write data to the kernel driver.
* Use `ioctl()` to control the driver.
* Use sysfs to expose driver information.
* Apply mutex synchronization to protect shared data.
* Evaluate basic safety issues in kernel programming.

## 3. Background

### 3.1 User Mode and Kernel Mode

User mode is where normal applications run with limited privileges. Kernel mode is where the operating system kernel and device drivers run with high privileges.

Because kernel code has direct access to system resources, programming errors in kernel mode may affect the whole operating system. Therefore, this project was developed and tested inside a Linux virtual machine.

### 3.2 Character Device

A character device transfers data as a stream of bytes. In this project, `/dev/mychardev` is a virtual character device.

The driver stores data inside a kernel buffer. User-space programs can write data into this buffer and read it back later.

### 3.3 Linux Kernel Module

A Linux Kernel Module is code that can be loaded into or removed from the Linux kernel at runtime.

This project uses:

```bash
sudo insmod mychardev.ko
```

to load the module, and:

```bash
sudo rmmod mychardev
```

to unload the module.

## 4. System Design

```text
+----------------------+
| User-space Program   |
| test_read_write.c    |
| test_ioctl.c         |
+----------+-----------+
           |
           | open/read/write/ioctl
           v
+----------------------+
| /dev/mychardev       |
+----------+-----------+
           |
           v
+----------------------+
| Linux VFS Layer      |
+----------+-----------+
           |
           v
+----------------------+
| mychardev Kernel     |
| Module               |
+----------+-----------+
           |
           v
+----------------------+
| kernel_buffer        |
| mutex                |
+----------------------+
```

## 5. Implementation

The driver is implemented in:

```text
kernel/mychardev.c
```

Main functions:

| Function             | Description                                                 |
| -------------------- | ----------------------------------------------------------- |
| `my_open()`          | Called when the device is opened                            |
| `my_release()`       | Called when the device is closed                            |
| `my_read()`          | Sends data from kernel buffer to user-space                 |
| `my_write()`         | Receives data from user-space into kernel buffer            |
| `my_ioctl()`         | Handles control commands                                    |
| `buffer_size_show()` | Shows buffer size through sysfs                             |
| `mychardev_init()`   | Registers the character device and creates `/dev/mychardev` |
| `mychardev_exit()`   | Removes the device and unregisters the driver               |

## 6. User-Kernel Communication

### 6.1 read() and write()

The driver uses `read()` and `write()` to transfer data between user-space and kernel-space.

Example test:

```bash
echo "Hello OSG202" | sudo tee /dev/mychardev
sudo cat /dev/mychardev
```

### 6.2 copy_to_user() and copy_from_user()

The kernel must not directly access user-space pointers.

This project uses:

* `copy_from_user()` to copy data from user-space to kernel-space.
* `copy_to_user()` to copy data from kernel-space to user-space.

These functions help prevent unsafe memory access.

### 6.3 ioctl()

`ioctl()` is used to send control commands from user-space to the driver.

This project supports:

| Command              | Function                      |
| -------------------- | ----------------------------- |
| `MYCHAR_IOC_CLEAR`   | Clear the kernel buffer       |
| `MYCHAR_IOC_GET_LEN` | Get the current buffer length |

### 6.4 sysfs

The driver creates a sysfs attribute:

```text
/sys/class/mycharclass/mychardev/buffer_size
```

Command:

```bash
cat /sys/class/mycharclass/mychardev/buffer_size
```

Expected output:

```text
1024
```

### 6.5 Mutex Synchronization

The driver uses a mutex to protect shared data:

```c
static DEFINE_MUTEX(buffer_mutex);
```

The protected shared resources are:

```text
kernel_buffer
data_len
```

Mutex synchronization helps avoid race conditions when multiple processes access the driver at the same time.

## 7. Testing

### 7.1 Build Module

```bash
cd kernel
make clean
make
```

### 7.2 Load Module

```bash
sudo insmod mychardev.ko
```

or:

```bash
cd scripts
./load.sh
```

### 7.3 Check Device

```bash
ls -l /dev/mychardev
```

Expected result:

```text
crw------- ... /dev/mychardev
```

The first character `c` means it is a character device.

### 7.4 Test Read/Write

```bash
echo "Hello OSG202" | sudo tee /dev/mychardev
sudo cat /dev/mychardev
```

Expected output:

```text
Hello OSG202
```

### 7.5 Test User-Space Program

```bash
cd user
gcc test_read_write.c -o test_read_write
sudo ./test_read_write
```

Expected output:

```text
Read from kernel: Hello from user-space program!
```

### 7.6 Test ioctl

```bash
cd user
gcc test_ioctl.c -o test_ioctl
sudo ./test_ioctl
```

Expected output:

```text
Current buffer length: ...
Buffer cleared
Buffer length after clear: 0
```

### 7.7 Test sysfs

```bash
cat /sys/class/mycharclass/mychardev/buffer_size
```

Expected output:

```text
1024
```

### 7.8 Unload Module

```bash
sudo rmmod mychardev
```

or:

```bash
cd scripts
./unload.sh
```

## 8. Safety Evaluation

| Risk                   | Solution                                    |
| ---------------------- | ------------------------------------------- |
| Buffer overflow        | Limit write size to `BUFFER_SIZE - 1`       |
| Invalid user pointer   | Use `copy_to_user()` and `copy_from_user()` |
| Race condition         | Use mutex                                   |
| Invalid ioctl command  | Return `-EINVAL`                            |
| Copy failure           | Return `-EFAULT`                            |
| Interrupted mutex lock | Return `-ERESTARTSYS`                       |
| Kernel crash risk      | Test inside virtual machine                 |
| Init failure           | Clean up resources step by step             |

## 9. Results

The project successfully creates a Linux character device:

```text
/dev/mychardev
```

The driver can:

* receive data from user-space,
* send data back to user-space,
* clear buffer using ioctl,
* return current buffer length using ioctl,
* expose buffer size through sysfs,
* synchronize buffer access using mutex,
* load and unload using shell scripts.

## 10. Conclusion

This project successfully demonstrates a simple Linux character device driver.

The implementation shows how user-space communicates with kernel-space through `read()`, `write()`, `ioctl()`, and sysfs interfaces. It also applies basic safety mechanisms such as buffer checking, user pointer handling, error return codes, and mutex synchronization.

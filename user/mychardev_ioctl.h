#ifndef MYCHARDEV_IOCTL_H
#define MYCHARDEV_IOCTL_H

#include <sys/ioctl.h>

#define MYCHAR_IOC_MAGIC 'k'
#define MYCHAR_IOC_CLEAR _IO(MYCHAR_IOC_MAGIC, 1)
#define MYCHAR_IOC_GET_LEN _IOR(MYCHAR_IOC_MAGIC, 2, int)

#endif

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include "mychardev_ioctl.h"

#define DEVICE_PATH "/dev/mychardev"

int main(void)
{
    int fd;
    int len;

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (ioctl(fd, MYCHAR_IOC_GET_LEN, &len) < 0) {
        perror("ioctl GET_LEN");
        close(fd);
        return 1;
    }

    printf("Current buffer length: %d\n", len);

    if (ioctl(fd, MYCHAR_IOC_CLEAR) < 0) {
        perror("ioctl CLEAR");
        close(fd);
        return 1;
    }

    printf("Buffer cleared\n");

    if (ioctl(fd, MYCHAR_IOC_GET_LEN, &len) < 0) {
        perror("ioctl GET_LEN after clear");
        close(fd);
        return 1;
    }

    printf("Buffer length after clear: %d\n", len);

    close(fd);
    return 0;
}

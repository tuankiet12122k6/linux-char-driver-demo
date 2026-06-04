#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define DEVICE_PATH "/dev/mychardev"

int main(void)
{
    int fd;
    char write_buf[] = "Hello from user-space program!";
    char read_buf[1024];
    int n;

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("open");
        return 1;
    }

    if (write(fd, write_buf, strlen(write_buf)) < 0) {
        perror("write");
        close(fd);
        return 1;
    }

    lseek(fd, 0, SEEK_SET);

    n = read(fd, read_buf, sizeof(read_buf) - 1);
    if (n < 0) {
        perror("read");
        close(fd);
        return 1;
    }

    read_buf[n] = '\0';
    printf("Read from kernel: %s\n", read_buf);

    close(fd);
    return 0;
}

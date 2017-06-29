#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "i2c_flash.h"
#define DEVICE_NAME "/dev/i2c_flash"

int main() {
    int fd = open(DEVICE_NAME, O_RDWR);
    char buf[256], rbuf[256] = {0};
    int i = 0;
    for (; i < 256; i++) {
        buf[i] = 'L';
    }
    write(fd, buf, 4);
    sleep(1);
    int num = 0;
    ioctl(fd, FLASHGETP, &num);
    printf("Current page is %d\n", num);
    ioctl(fd, FLASHSETP, 0);
    ioctl(fd, FLASHGETP, &num);
    printf("Current page is %d\n", num);
    int j = 0;
    while (read(fd, rbuf, 4) && j++ < 5) {
        perror("READ");
        sleep(1);
    }
    for (i = 0; i < 256; i++) {
        printf("%c:", rbuf[i]);
    }
    printf("\n");
    ioctl(fd, FLASHERASE);
    j = 0;
    while (read(fd, rbuf, 4) && j++ < 10) {
        perror("READ");
        sleep(1);
    }
    for (i = 0; i < 256; i++) {
        printf("%c:", rbuf[i]);
    }
    return 0;
}

// led_notify.c
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

void notify_led(unsigned char pattern) {
    int fd = open("/dev/fpga_led", O_WRONLY);
    if (fd < 0) {
        perror("LED device open error");
        return;
    }

    write(fd, &pattern, 1);
    close(fd);
}

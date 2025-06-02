// fnd_timer.c
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

void show_time_on_fnd(int seconds) {
    int fd = open("/dev/fpga_fnd", O_WRONLY);
    if (fd < 0) {
        perror("FND device open error");
        return;
    }

    unsigned char data[4] = {
        seconds / 60 / 10,
        seconds / 60 % 10,
        (seconds % 60) / 10,
        (seconds % 60) % 10
    };
    write(fd, data, 4);
    close(fd);
}

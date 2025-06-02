// buzzer_alert.c
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

void alert_buzzer(int on) {
    int fd = open("/dev/fpga_buzzer", O_WRONLY);
    if (fd < 0) {
        perror("Buzzer device open error");
        return;
    }

    unsigned char value = on ? 1 : 0;
    write(fd, &value, 1);
    close(fd);
}

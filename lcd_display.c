// lcd_display.c
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

void show_message_on_lcd(const char* message) {
    int fd = open("/dev/fpga_text_lcd", O_WRONLY);
    if (fd < 0) {
        perror("LCD device open error");
        return;
    }

    char buf[32] = {0};
    strncpy(buf, message, 32);
    write(fd, buf, 32);
    close(fd);
}

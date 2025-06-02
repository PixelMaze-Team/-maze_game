// dot_control.c
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

void display_maze_on_dot(const int maze[8][8]) {
    int fd = open("/dev/fpga_dot", O_WRONLY);
    if (fd < 0) {
        perror("DOT device open error");
        return;
    }

    unsigned char data[8] = {0};
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            if (maze[i][j]) data[i] |= (1 << (7 - j));
        }
    }

    write(fd, data, 8);
    close(fd);
}

// dot_control.c
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
void display_maze_on_dot(const int maze[10][7]) {
    int fd = open("/dev/fpga_dot", O_WRONLY);
    if (fd < 0) {
        perror("DOT device open error");
        return;
    }
    unsigned char data[10] = {0};  // 10행
    for (int i = 0; i < 10; ++i) {
        for (int j = 0; j < 7; ++j) {
            if (maze[i][j]) data[i] |= (1 << (6 - j));  // 7열
        }
    }
    write(fd, data, 10);
    close(fd);
}
// switch_input.c
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int get_switch_input() {
    int fd = open("/dev/fpga_push_switch", O_RDONLY);
    if (fd < 0) {
        perror("Switch device open error");
        return -1;
    }

    unsigned char input[9] = {0};
    read(fd, &input, sizeof(input));
    close(fd);

    for (int i = 0; i < 9; i++) {
        if (input[i] == 1) return i + 1;  // 1~9번 스위치
    }

    return 0;  // 아무 입력 없음
}

// maze_game.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#define MAZE_W 7
#define MAZE_H 10
int maze[MAZE_H][MAZE_W];
void generate_maze() {
    srand(time(NULL));
    for (int y = 0; y < MAZE_H; y++) {
        for (int x = 0; x < MAZE_W; x++) {
            maze[y][x] = (rand() % 5 == 0) ? 1 : 0;  // 약 20% 확률로 벽 생성
        }
    }
    maze[1][1] = 0;       // 시작점
    maze[8][5] = 0;       // 도착점
}
void display_dot() {
    int fd = open("/dev/fpga_dot", O_WRONLY);
    if (fd < 0) return;
    unsigned char data[10] = {0};
    for (int i = 0; i < MAZE_H; ++i) {
        for (int j = 0; j < MAZE_W; ++j) {
            if (maze[i][j]) data[i] |= (1 << (6 - j));
        }
    }
    write(fd, data, 10);
    close(fd);
}
void display_maze_with_player(int x, int y) {
    int fd = open("/dev/fpga_dot", O_WRONLY);
    if (fd < 0) return;
    unsigned char data[10] = {0};
    for (int i = 0; i < MAZE_H; ++i) {
        for (int j = 0; j < MAZE_W; ++j) {
            if (maze[i][j]) data[i] |= (1 << (6 - j));
        }
    }
    data[y] |= (1 << (6 - x));  // 플레이어 위치
    write(fd, data, 10);
    close(fd);
}
int get_switch_input() {
    int fd = open("/dev/fpga_push_switch", O_RDONLY);
    if (fd < 0) return 0;
    unsigned char input[9] = {0};
    read(fd, input, sizeof(input));
    close(fd);
    for (int i = 0; i < 9; i++) {
        if (input[i] == 1) return i + 1;
    }
    return 0;
}
void lcd_message(const char* msg) {
    int fd = open("/dev/fpga_text_lcd", O_WRONLY);
    if (fd < 0) return;
    char buffer[32] = {0};
    strncpy(buffer, msg, 32);
    write(fd, buffer, 32);
    close(fd);
}
void show_time_on_fnd(int seconds) {
    int fd = open("/dev/fpga_fnd", O_WRONLY);
    if (fd < 0) return;
    unsigned char data[4] = {
        (seconds / 1000) % 10,
        (seconds / 100) % 10,
        (seconds / 10) % 10,
        seconds % 10
    };
    write(fd, data, 4);
    close(fd);
}
void buzzer(int on) {
    int fd = open("/dev/fpga_buzzer", O_WRONLY);
    if (fd < 0) return;
    unsigned char data = on ? 1 : 0;
    write(fd, &data, 1);
    close(fd);
}
void led(unsigned char pattern) {
    int fd = open("/dev/fpga_led", O_WRONLY);
    if (fd < 0) return;
    write(fd, &pattern, 1);
    close(fd);
}
int main() {
    int x = 1, y = 1;
    int goal_x = 5, goal_y = 8;
    int time = 60;
    generate_maze();
    lcd_message("Maze Game Start!");
    buzzer(1); sleep(1); buzzer(0);
    led(0xFF);
    while (time > 0) {
        display_maze_with_player(x, y);
        show_time_on_fnd(time);
        int input = get_switch_input();
        if (input) {
            int new_x = x, new_y = y;
            switch (input) {
                case 2: new_y--; break;
                case 4: new_x--; break;
                case 6: new_x++; break;
                case 8: new_y++; break;
            }
            if (new_x >= 0 && new_x < MAZE_W && new_y >= 0 && new_y < MAZE_H) {
                if (maze[new_y][new_x] == 0) {
                    x = new_x; y = new_y;
                } else {
                    buzzer(1); usleep(300000); buzzer(0); // 벽에 부딪힘
                }
            }
        }
        if (x == goal_x && y == goal_y) {
            lcd_message("Clear!");
            buzzer(1); sleep(2); buzzer(0);
            led(0x00);
            break;
        }
        sleep(1);
        time--;
    }
    if (time == 0) {
        lcd_message("Time Over!");
        buzzer(1); sleep(2); buzzer(0);
        led(0xAA);
    }
    return 0;
}
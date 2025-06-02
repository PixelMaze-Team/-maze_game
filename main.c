// main.c
#include <stdio.h>
#include <unistd.h>
#include "maze_map.h"
// 각 제어 함수들의 선언
void display_maze_on_dot(const int maze[8][8]);
int get_switch_input();
void show_time_on_fnd(int seconds);
void alert_buzzer(int on);
void show_message_on_lcd(const char* message);
void notify_led(unsigned char pattern);
// 플레이어 좌표
int player_x = 1;
int player_y = 1;
// 현재 위치를 미로에 표시
void update_maze_with_player(int maze[8][8], int x, int y) {
    int temp[8][8];
    for (int i = 0; i < 8; ++i)
        for (int j = 0; j < 8; ++j)
            temp[i][j] = maze[i][j];
    temp[y][x] = 0; // 플레이어는 0으로 표시 (혹은 2 등 표시 방식 조절 가능)
    display_maze_on_dot(temp);
}
int main() {
    int time = 60;  // 제한 시간 (예: 60초)
    int x = player_x, y = player_y;
    show_message_on_lcd("Maze Game Start!");
    notify_led(0xFF); // LED 전부 ON
    alert_buzzer(1); sleep(1); alert_buzzer(0); // 부저 삑!
    while (time > 0) {
        show_time_on_fnd(time);
        update_maze_with_player(maze, x, y);
        int input = get_switch_input();  // 1~9 푸시스위치
        if (input != 0) {
            switch (input) {
                case 2: if (y > 0 && maze[y - 1][x] == 0) y--; break; // 위
                case 4: if (x > 0 && maze[y][x - 1] == 0) x--; break; // 왼쪽
                case 6: if (x < 7 && maze[y][x + 1] == 0) x++; break; // 오른쪽
                case 8: if (y < 7 && maze[y + 1][x] == 0) y++; break; // 아래
            }
        }
        if (x == 6 && y == 7) {
            show_message_on_lcd("Clear!");
            notify_led(0x00);  // LED OFF
            alert_buzzer(1); sleep(2); alert_buzzer(0);
            break;
        }
        sleep(1);
        time--;
    }
    if (time <= 0) {
        show_message_on_lcd("Time Over!");
        notify_led(0xAA);  // 경고 패턴
        alert_buzzer(1); sleep(1); alert_buzzer(0);
    }
    return 0;
}

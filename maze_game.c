/*-------------------------------------------------------
 *  7×10 Maze Game  (∞ Stage / AI 자동 클리어 연속 실행 모드)
 *  ▸ Start : (0,0)   Goal : (ROW-1,COL-1)
 *  ▸ Level별 벽 비율 : 점진적 상승 (최대 85%)
 *------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#define ROW 10
#define COL 7
#define LIMIT_SECONDS 60
double ratio[] = {0.25,0.40,0.55,0.70,0.75,0.78,0.80,0.82,0.84,0.85,0.85};
unsigned char maze[ROW][COL];
bool on_path[ROW][COL];
int prev[ROW][COL][2];
int px = 0, py = 0, stage = 0, time_left = LIMIT_SECONDS;
bool ai_mode = false;
int lcd_fd = -1, buz_fd = -1, dot_fd = -1, fnd_fd = -1, sw_fd = -1;
void fatal(const char *s) { perror(s); exit(1); }
void buz(int on) { char v = on ? 1 : 0; if (buz_fd >= 0) write(buz_fd, &v, 1); }
void show_lcd(const char *l1, const char *l2) {
  if (lcd_fd < 0) return;
  char buf[32] = {0};
  snprintf(buf, 16, "%-15s", l1);
  snprintf(buf + 16, 16, "%-15s", l2);
  write(lcd_fd, buf, 32);
}
void show_fnd(int sec) {
  if (fnd_fd < 0) return;
  unsigned char d[4] = {sec / 600, (sec / 60) % 10, (sec % 60) / 10, sec % 10};
  write(fnd_fd, d, 4);
}
void draw_dot(void) {
  if (dot_fd < 0) return;
  unsigned char line[ROW] = {0};
  for (int r = 0; r < ROW; r++) for (int c = 0; c < COL; c++) {
    int cell = (r == py && c == px) ? 3 : maze[r][c];
    if (cell == 1 || cell == 3) line[r] |= (1 << (COL - 1 - c));
  }
  write(dot_fd, line, ROW);
}
int can_move(int nx, int ny) {
  return nx >= 0 && nx < COL && ny >= 0 && ny < ROW && maze[ny][nx] != 1;
}
int dir4[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
void shuffle_dir(void) {
  for (int i = 3; i > 0; i--) {
    int r = rand() % (i + 1);
    int t0 = dir4[i][0], t1 = dir4[i][1];
    dir4[i][0] = dir4[r][0]; dir4[i][1] = dir4[r][1];
    dir4[r][0] = t0; dir4[r][1] = t1;
  }
}
void carve(int y, int x) {
  shuffle_dir();
  for (int k = 0; k < 4; k++) {
    int ny = y + dir4[k][1] * 2, nx = x + dir4[k][0] * 2;
    if (nx < 0 || nx >= COL || ny < 0 || ny >= ROW) continue;
    if (maze[ny][nx] == 1) {
      maze[y + dir4[k][1]][x + dir4[k][0]] = 0;
      maze[ny][nx] = 0;
      carve(ny, nx);
    }
  }
}
typedef struct { int y, x; } QN;
QN Q[ROW * COL]; int qh, qt;
void bfs_path(void) {
  for (int r = 0; r < ROW; r++) for (int c = 0; c < COL; c++) {
    prev[r][c][0] = prev[r][c][1] = -1;
    on_path[r][c] = false;
  }
  qh = qt = 0; Q[qt++] = (QN){0, 0}; prev[0][0][0] = 0;
  while (qh < qt) {
    QN v = Q[qh++];
    if (v.y == ROW - 1 && v.x == COL - 1) break;
    for (int k = 0; k < 4; k++) {
      int ny = v.y + dir4[k][1], nx = v.x + dir4[k][0];
      if (can_move(nx, ny) && prev[ny][nx][0] == -1) {
        prev[ny][nx][0] = v.y; prev[ny][nx][1] = v.x;
        Q[qt++] = (QN){ny, nx};
      }
    }
  }
  for (int y = ROW - 1, x = COL - 1; !(y == 0 && x == 0); ) {
    on_path[y][x] = true;
    int py_ = prev[y][x][0], px_ = prev[y][x][1];
    y = py_; x = px_;
  }
  on_path[0][0] = true;
}
void add_extra_walls(int level) {
  if (level > 10) level = 10;
  int target = (int)(ROW * COL * ratio[level]);
  int added = 0, guard = 0;
  while (added < target && guard < 5000) {
    int r = rand() % ROW, c = rand() % COL; guard++;
    if (on_path[r][c]) continue;
    if ((r == 0 && c == 0) || (r == ROW - 1 && c == COL - 1)) continue;
    if (maze[r][c] == 0) { maze[r][c] = 1; added++; }
  }
}
void make_maze(int level) {
  for (int r = 0; r < ROW; r++) for (int c = 0; c < COL; c++) maze[r][c] = 1;
  maze[0][0] = 0; carve(0, 0);
  maze[ROW - 1][COL - 1] = 2;
  bfs_path(); add_extra_walls(level); bfs_path();
}
void ai_clear_and_loop(void) {
  while (1) {
    if (prev[ROW - 1][COL - 1][0] == -1) return;
    int path[ROW * COL][2], len = 0;
    for (int y = ROW - 1, x = COL - 1; !(y == 0 && x == 0); ) {
      path[len][0] = y; path[len++][1] = x;
      int py_ = prev[y][x][0], px_ = prev[y][x][1];
      y = py_; x = px_;
    }
    path[len][0] = 0; path[len++][1] = 0;
    for (int i = len - 1; i >= 0; i--) {
      py = path[i][0]; px = path[i][1]; draw_dot(); usleep(70000);
    }
    show_lcd("CLEAR!!", "Next Stage");
    for (int i = 0; i < 2; i++) { buz(1); usleep(100000); buz(0); usleep(100000); }
    stage++; px = py = 0; time_left = LIMIT_SECONDS;
    make_maze(stage);
    char lcd2[16]; snprintf(lcd2, 16, "Stage %d", stage + 1);
    show_lcd("Maze Game", lcd2); show_fnd(time_left); draw_dot();
    usleep(200000); // 다음 스테이지 진입 딜레이
  }
}
int main(void) {
  srand(time(NULL));
  lcd_fd = open("/dev/fpga_text_lcd", O_WRONLY); if (lcd_fd < 0) fatal("lcd");
  buz_fd = open("/dev/fpga_buzzer", O_WRONLY);   if (buz_fd < 0) fatal("buzzer");
  dot_fd = open("/dev/fpga_dot", O_WRONLY);      if (dot_fd < 0) fatal("dot");
  fnd_fd = open("/dev/fpga_fnd", O_WRONLY);      if (fnd_fd < 0) fatal("fnd");
  sw_fd  = open("/dev/fpga_push_switch", O_RDONLY); if (sw_fd < 0) fatal("switch");
  make_maze(0);
  char lcd2[16]; snprintf(lcd2, 16, "Stage %d", stage + 1);
  show_lcd("Maze Game", lcd2); show_fnd(time_left); draw_dot();
  unsigned char sw[9] = {0}; time_t last = time(NULL);
  while (1) {
    time_t now = time(NULL);
    if (now != last) {
      time_left--; show_fnd(time_left); last = now;
      if (time_left <= 0) {
        show_lcd("TIME OVER", "Retry");
        buz(1); usleep(300000); buz(0); sleep(1);
        px = py = 0; time_left = LIMIT_SECONDS;
        show_lcd("Maze Game", lcd2); draw_dot();
      }
    }
    read(sw_fd, sw, 9);
    if (sw[0] && !ai_mode) {
      ai_mode = true;
      show_lcd("AI MODE", "Auto Clear");
      usleep(500000);
      ai_clear_and_loop();
    }
    if (!ai_mode) {
      int moved = 0;
      if (sw[1] && can_move(px, py - 1)) { py--; moved = 1; }
      else if (sw[7] && can_move(px, py + 1)) { py++; moved = 1; }
      else if (sw[3] && can_move(px - 1, py)) { px--; moved = 1; }
      else if (sw[5] && can_move(px + 1, py)) { px++; moved = 1; }
      else if (sw[1] | sw[3] | sw[5] | sw[7]) { buz(1); usleep(120000); buz(0); }
      if (moved) draw_dot();
      if (px == COL - 1 && py == ROW - 1) {
        show_lcd("CLEAR!!", "Next Stage");
        for (int i = 0; i < 2; i++) { buz(1); usleep(180000); buz(0); usleep(120000); }
        stage++; px = py = 0; time_left = LIMIT_SECONDS;
        make_maze(stage);
        snprintf(lcd2, 16, "Stage %d", stage + 1);
        show_lcd("Maze Game", lcd2); show_fnd(time_left); draw_dot();
      }
    }
    usleep(65000);
  }
  return 0;
}
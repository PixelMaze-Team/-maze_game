/*-------------------------------------------------------
 *  7×10 Maze Game  (∞ Stage / 난이도 점증)
 *  ▸ Start : (0,0)   Goal : (ROW-1,COL-1)
 *  ▸ Level별 벽 비율 : 25 / 40 / 55 / 70 / 75 / 78 / 80 / 82 / 84 / 85 %
 *------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
/* ───── 기본 상수 ───── */
#define ROW 10
#define COL 7
#define LIMIT_SECONDS 60
/* 단계별(0~10) 벽 비율 테이블 – 11단계 이후엔 85 % 고정 */
double ratio[] = {0.25,0.40,0.55,0.70,0.75,0.78,0.80,0.82,0.84,0.85,0.85};
/* ───── 전역 ───── */
unsigned char maze[ROW][COL];   /* 0 길 | 1 벽 | 2 Goal */
bool   on_path[ROW][COL];       /* BFS 최단경로 표시   */
int    px=0, py=0;              /* 플레이어 좌표       */
int    stage=0, time_left=LIMIT_SECONDS;
/* 장치 FD */
int lcd_fd=-1,buz_fd=-1,dot_fd=-1,fnd_fd=-1,sw_fd=-1;
/* ───── 공통 I/O 함수 ───── */
void fatal(const char*s){perror(s);exit(1);}
void buz(int on){char v=on?1:0; if(buz_fd>=0) write(buz_fd,&v,1);}
void show_lcd(const char*l1,const char*l2){
  if(lcd_fd<0) return;
  char buf[32]={0};
  snprintf(buf,16,"%-15s",l1);
  snprintf(buf+16,16,"%-15s",l2);
  write(lcd_fd,buf,32);
}
void show_fnd(int sec){
  if(fnd_fd<0) return;
  unsigned char d[4]={sec/600,(sec/60)%10,(sec%60)/10,sec%10};
  write(fnd_fd,d,4);
}
void draw_dot(void){
  if(dot_fd<0) return;
  unsigned char line[ROW]={0};
  for(int r=0;r<ROW;r++)for(int c=0;c<COL;c++){
      int cell=(r==py && c==px)?3:maze[r][c]; /* 3 = Player */
      if(cell==1||cell==3) line[r]|=(1<<(COL-1-c));
  }
  write(dot_fd,line,ROW);
}
int can_move(int nx,int ny){
  return nx>=0&&nx<COL&&ny>=0&&ny<ROW&&maze[ny][nx]!=1;
}
/* ───── DFS + Fisher-Yates 미로 뚫기 ───── */
int dir4[4][2]={{0,-1},{0,1},{-1,0},{1,0}};
void shuffle_dir(void){
  for(int i=3;i>0;i--){
      int r=rand()%(i+1);
      int t0=dir4[i][0], t1=dir4[i][1];
      dir4[i][0]=dir4[r][0]; dir4[i][1]=dir4[r][1];
      dir4[r][0]=t0; dir4[r][1]=t1;
  }
}
void carve(int y,int x){
  shuffle_dir();
  for(int k=0;k<4;k++){
      int ny=y+dir4[k][1]*2, nx=x+dir4[k][0]*2;
      if(nx<0||nx>=COL||ny<0||ny>=ROW) continue;
      if(maze[ny][nx]==1){
          maze[y+dir4[k][1]][x+dir4[k][0]]=0; /* 한 칸 뚫기 */
          maze[ny][nx]=0; carve(ny,nx);
      }
  }
}
/* ───── BFS 최단-경로 추출 ───── */
typedef struct{int y,x;} QN;
QN Q[ROW*COL]; int qh,qt;
void bfs_path(void){
  int prev[ROW][COL][2]; /* 이전 좌표 저장 */
  for(int r=0;r<ROW;r++)for(int c=0;c<COL;c++)
      prev[r][c][0]=prev[r][c][1]=-1, on_path[r][c]=false;
  qh=qt=0; Q[qt++] = (QN){0,0}; prev[0][0][0]=0;
  while(qh<qt){
      QN v=Q[qh++];
      if(v.y==ROW-1 && v.x==COL-1) break;
      for(int k=0;k<4;k++){
          int ny=v.y+dir4[k][1], nx=v.x+dir4[k][0];
          if(can_move(nx,ny)&&prev[ny][nx][0]==-1){
              prev[ny][nx][0]=v.y; prev[ny][nx][1]=v.x;
              Q[qt++] = (QN){ny,nx};
          }
      }
  }
  /* 역추적 → on_path[] 표시 */
  for(int y=ROW-1,x=COL-1; !(y==0&&x==0); ){
      on_path[y][x]=true;
      int py_=prev[y][x][0], px_=prev[y][x][1];
      y=py_; x=px_;
  }
  on_path[0][0]=true;
}
/* ───── 추가 벽 삽입 (난이도↑) ───── */
void add_extra_walls(int level){
  if(level>10) level=10;
  int target=(int)(ROW*COL*ratio[level]);
  int added=0, guard=0;
  while(added<target && guard<5000){
      int r=rand()%ROW, c=rand()%COL; guard++;
      if(on_path[r][c]) continue;                       /* 길 보호 */
      if((r==0&&c==0)||(r==ROW-1&&c==COL-1)) continue;  /* 시작·Goal 보호 */
      if(maze[r][c]==0){ maze[r][c]=1; added++; }
  }
}
/* ───── 전체 미로 생성 ───── */
void make_maze(int level){
  /* ① 전체 벽 */
  for(int r=0;r<ROW;r++)for(int c=0;c<COL;c++)maze[r][c]=1;
  /* ② DFS 로 뚫기 */
  maze[0][0]=0; carve(0,0);
  /* ③ Goal */
  maze[ROW-1][COL-1]=2;
  /* ④ BFS 경로 확보 후 난이도↑ */
  bfs_path(); add_extra_walls(level);
}
/* ───── MAIN ───── */
int main(void){
  srand(time(NULL));
  /* -------- 장치 열기 -------- */
  lcd_fd=open("/dev/fpga_text_lcd",O_WRONLY); if(lcd_fd<0)fatal("lcd");
  buz_fd=open("/dev/fpga_buzzer",O_WRONLY);   if(buz_fd<0)fatal("buzzer");
  dot_fd=open("/dev/fpga_dot",O_WRONLY);      if(dot_fd<0)fatal("dot");
  fnd_fd=open("/dev/fpga_fnd",O_WRONLY);      if(fnd_fd<0)fatal("fnd");
  sw_fd =open("/dev/fpga_push_switch",O_RDONLY); if(sw_fd<0)fatal("switch");
  /* -------- 초기 미로 -------- */
  make_maze(0);
  char lcd2[16]; snprintf(lcd2,16,"Stage %d",stage+1);
  show_lcd("Maze Game",lcd2); show_fnd(time_left); draw_dot();
  unsigned char sw[9]={0}; time_t last=time(NULL);
  while(1){
      /* ----- 타이머 관리 ----- */
      time_t now=time(NULL);
      if(now!=last){
          time_left--; show_fnd(time_left); last=now;
          if(time_left<=0){
              show_lcd("TIME OVER","Retry");
              buz(1); usleep(300000); buz(0); sleep(1);
              px=py=0; time_left=LIMIT_SECONDS;
              show_lcd("Maze Game",lcd2); draw_dot();
          }
      }
      /* ----- 스위치 입력 ----- */
      read(sw_fd,sw,9);
      int moved=0;
      if(sw[1]&&can_move(px,py-1)){py--;moved=1;}         /* 2: 위 */
      else if(sw[7]&&can_move(px,py+1)){py++;moved=1;}    /* 8: 아래 */
      else if(sw[3]&&can_move(px-1,py)){px--;moved=1;}    /* 4: 왼쪽 */
      else if(sw[5]&&can_move(px+1,py)){px++;moved=1;}    /* 6: 오른쪽 */
      else if(sw[1]|sw[3]|sw[5]|sw[7]){buz(1);usleep(120000);buz(0);} /* 벽 부딪힘 */
      if(moved) draw_dot();
      /* ----- Goal 판정 ----- */
      if(px==COL-1 && py==ROW-1){
          show_lcd("CLEAR!!","Next Stage");
          for(int i=0;i<2;i++){buz(1);usleep(180000);buz(0);usleep(120000);}
          stage++;                           /* 스테이지++  (무한) */
          px=py=0; time_left=LIMIT_SECONDS;
          make_maze(stage);                  /* 난이도 = stage */
          snprintf(lcd2,16,"Stage %d",stage+1);
          show_lcd("Maze Game",lcd2); show_fnd(time_left); draw_dot();
      }
      usleep(65000);
  }
  return 0;
}
/*-------------------------------------------------------
 *  7×10 Maze Game - AI-Auto-Clear (DIP-SW #1) 2025-06
 *  ▸ Start  (0,0)  ▸ Goal  (ROW-1,COL-1)
 *  ▸ Stage   0~3   : 벽 비율 25 / 40 / 55 / 70 %
 *  ▸ DIP-SW 1 ↑    : A★ 경로를 따라 자동 클리어
 *  ▸ Push-SW 2 4 6 8: ↑ ← → ↓  (수동 모드일 때)
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
#define MAX_STAGE 4
#define LIMIT_SECONDS 60

/* ───── 전역 ───── */
unsigned char maze[ROW][COL];          /* 0 길 | 1 벽 | 2 Goal             */
bool          on_path[ROW][COL];       /* 최단 경로 마커 (벽 보호용)        */
int           px=0, py=0;              /* 플레이어 위치                    */
int           stage=0, time_left=LIMIT_SECONDS;

int lcd_fd=-1, buz_fd=-1, dot_fd=-1, fnd_fd=-1, sw_fd=-1, dip_fd=-1;

/* === AI AUTO === */
#define MAX_PATH  (ROW*COL)
int path_y[MAX_PATH], path_x[MAX_PATH], path_len=0, path_idx=0;
bool ai_mode=false;
/* =============== */

/* ───── 공통 I/O ───── */
void fatal(const char*s){ perror(s); exit(1); }
void buz(int on){ char v=on?1:0; if(buz_fd>=0) write(buz_fd,&v,1); }

void show_lcd(const char*l1,const char*l2){
    if(lcd_fd<0) return;
    char buf[32]={0};
    snprintf(buf,16,"%-15s",l1); snprintf(buf+16,16,"%-15s",l2);
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
        int cell = (r==py && c==px)?3 : maze[r][c];      /* 3 = 플레이어 */
        if(cell==1||cell==3) line[r] |= 1<<(COL-1-c);
    }
    write(dot_fd,line,ROW);
}
int can_move(int nx,int ny){
    return nx>=0&&nx<COL&&ny>=0&&ny<ROW&&maze[ny][nx]!=1;
}

/* ───── DFS 미로 뼈대 ───── */
int dir4[4][2]={{0,-1},{0,1},{-1,0},{1,0}};
void shuffle_dir(void){ for(int i=3;i>0;i--){int r=rand()%(i+1);
   int t0=dir4[i][0],t1=dir4[i][1]; dir4[i][0]=dir4[r][0]; dir4[i][1]=dir4[r][1];
   dir4[r][0]=t0; dir4[r][1]=t1;} }
void carve(int y,int x){
    shuffle_dir();
    for(int k=0;k<4;k++){
        int ny=y+dir4[k][1]*2, nx=x+dir4[k][0]*2;
        if(nx<0||nx>=COL||ny<0||ny>=ROW) continue;
        if(maze[ny][nx]==1){
            maze[y+dir4[k][1]][x+dir4[k][0]]=0;
            maze[ny][nx]=0; carve(ny,nx);
        }
    }
}

/* ───── BFS - 최단경로 추출 → AI path_y/x[] 저장 ───── */
typedef struct{int y,x;} QN;
void build_path(void){
    int prev[ROW][COL][2];  /* 이전 좌표 저장 */
    for(int r=0;r<ROW;r++)for(int c=0;c<COL;c++) prev[r][c][0]=prev[r][c][1]=-1;
    path_len=path_idx=0;
    memset(on_path,0,sizeof(on_path));

    QN queue[ROW*COL]; int qh=0, qt=0;
    queue[qt++] = (QN){0,0}; prev[0][0][0]=0;

    while(qh<qt){
        QN v=queue[qh++];
        if(v.y==ROW-1 && v.x==COL-1) break;
        for(int k=0;k<4;k++){
            int ny=v.y+dir4[k][1], nx=v.x+dir4[k][0];
            if(can_move(nx,ny) && prev[ny][nx][0]==-1){
                prev[ny][nx][0]=v.y; prev[ny][nx][1]=v.x;
                queue[qt++] = (QN){ny,nx};
            }
        }
    }
    /* 역추적하여 path 배열 작성 */
    int ty=ROW-1, tx=COL-1;
    while(!(ty==0 && tx==0)){
        path_y[path_len]=ty; path_x[path_len]=tx; path_len++;
        int py_=prev[ty][tx][0], px_=prev[ty][tx][1];
        ty=py_; tx=px_;
    }
    path_y[path_len]=0; path_x[path_len]=0; path_len++;
    /* on_path 마킹 (벽 보호용) */
    for(int i=0;i<path_len;i++) on_path[path_y[i]][path_x[i]]=true;
}

/* ───── 난이도↑ 벽 추가 (on_path는 유지) ───── */
void add_extra_walls(int diff){
    double ratio[4]={0.25,0.40,0.55,0.70};
    int tgt=(int)(ROW*COL*ratio[diff]), add=0, guard=0;
    while(add<tgt && guard<2000){
        int r=rand()%ROW, c=rand()%COL; guard++;
        if(on_path[r][c]) continue;
        if((r==0&&c==0)||(r==ROW-1&&c==COL-1)) continue;
        if(maze[r][c]==0){ maze[r][c]=1; add++; }
    }
}

/* ───── 통합 미로 생성기 ───── */
void make_maze(int diff){
    for(int r=0;r<ROW;r++)for(int c=0;c<COL;c++) maze[r][c]=1;
    maze[0][0]=0; carve(0,0); maze[ROW-1][COL-1]=2;
    build_path();              /* on_path + path_y/x 확보       */
    add_extra_walls(diff);
}

/* === AI AUTO : 한 스텝 이동 === */
void auto_step(void){
    if(path_idx==0) path_idx=path_len-1;  /* 역추적으로 저장, 마지막부터 */
    path_idx--;                           /* 한 칸 앞으로 */
    px = path_x[path_idx];
    py = path_y[path_idx];
}
/* =============== */

int main(void){
    srand(time(NULL));

    /* 장치 오픈 */
    lcd_fd=open("/dev/fpga_text_lcd",O_WRONLY); if(lcd_fd<0)fatal("lcd");
    buz_fd=open("/dev/fpga_buzzer",O_WRONLY);   if(buz_fd<0)fatal("buzzer");
    dot_fd=open("/dev/fpga_dot",O_WRONLY);      if(dot_fd<0)fatal("dot");
    fnd_fd=open("/dev/fpga_fnd",O_WRONLY);      if(fnd_fd<0)fatal("fnd");
    sw_fd =open("/dev/fpga_push_switch",O_RDONLY); if(sw_fd<0)fatal("switch");
    dip_fd=open("/dev/fpga_dip_switch",O_RDONLY);   /* DIP 없으면 -1 */

    make_maze(0);
    char lcd2[16]; snprintf(lcd2,16,"Stage %d",stage+1);
    show_lcd("Maze Game",lcd2); show_fnd(time_left); draw_dot();

    unsigned char sw[9]={0}, dip_val=0;
    time_t last=time(NULL);

    while(1){
        /* --- DIP 읽어 AI 모드 토글 --- */
        if(dip_fd>=0){ lseek(dip_fd,0,SEEK_SET); read(dip_fd,&dip_val,1);
            ai_mode = dip_val & 0x01; }

        /* --- 타이머 --- */
        time_t now=time(NULL);
        if(now!=last){
            time_left--; last=now; show_fnd(time_left);
            if(time_left<=0){
                show_lcd("TIME OVER","Retry");
                buz(1); usleep(300000); buz(0); sleep(1);
                px=py=0; time_left=LIMIT_SECONDS;
                show_lcd("Maze Game",lcd2); draw_dot();
            }
        }

        /* --- 움직임 --- */
        int moved=0;
        if(ai_mode){
            auto_step(); moved=1;
            usleep(50000);             /* AI 이동 속도 50 ms */
        }else{
            read(sw_fd,sw,9);
            if(sw[1]&&can_move(px,py-1)){py--;moved=1;}         /* 2 ↑ */
            else if(sw[7]&&can_move(px,py+1)){py++;moved=1;}    /* 8 ↓ */
            else if(sw[3]&&can_move(px-1,py)){px--;moved=1;}    /* 4 ← */
            else if(sw[5]&&can_move(px+1,py)){px++;moved=1;}    /* 6 → */
            else if(sw[1]|sw[3]|sw[5]|sw[7]){buz(1);usleep(120000);buz(0);}
            usleep(65000);             /* 수동 루프 65 ms */
        }
        if(moved) draw_dot();

        /* --- Goal 도달 --- */
        if(px==COL-1 && py==ROW-1){
            show_lcd("CLEAR!!","Next Stage");
            for(int i=0;i<2;i++){buz(1);usleep(180000);buz(0);usleep(120000);}
            stage = (stage+1) % MAX_STAGE;
            px=py=0; time_left=LIMIT_SECONDS;
            make_maze(stage);
            snprintf(lcd2,16,"Stage %d",stage+1);
            show_lcd("Maze Game",lcd2); show_fnd(time_left); draw_dot();
        }
    }
    return 0;
}
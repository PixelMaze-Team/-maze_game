// maze_map.c
#include "maze_map.h"
const int maze[MAZE_HEIGHT][MAZE_WIDTH] = {
    {0, 1, 1, 1, 1, 1, 1, 1},
    {0, 0, 0, 0, 1, 0, 0, 1},
    {1, 1, 1, 0, 1, 0, 1, 1},
    {1, 0, 0, 0, 0, 0, 1, 1},
    {1, 0, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 0, 1},
    {1, 1, 1, 1, 1, 1, 0, 0}
};

#ifndef __CONSTANTS_H
#define __CONSTANTS_H

#define FRAMES_PER_SEC 30
#define MS_PER_FRAME 1000.f/(real32)(FRAMES_PER_SEC)
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define GRAPHICS_MEMORY_SIZE 1024*1024*50
#define PHYSICS_MEMORY_SIZE 1024*1024*100
#define GAME_MEMORY_SIZE 1024*1024*100

#define GAME_ASPECT_RATIO ((real32)SCREEN_WIDTH/(real32)SCREEN_HEIGHT)
#define GAME_GRID_HEIGHT 100
#define GAME_GRID_WIDTH ((real32)GAME_GRID_HEIGHT*GAME_ASPECT_RATIO)
#define GAME_GRID_UNIT_HEIGHT (1.f/(real32)GAME_GRID_HEIGHT)
#define GAME_GRID_UNIT_WIDTH (1.f/GAME_GRID_WIDTH*GAME_ASPECT_RATIO)

#endif
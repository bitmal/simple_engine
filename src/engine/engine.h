#ifndef __ENGINE_H
#define __ENGINE_H

#include "types.h"
#include "constants.h"

struct physics;
struct renderer;
struct memory;
struct config;
struct input;
struct statistics;

struct context
{
    struct memory *memoryContext;
    struct config *config;
    struct physics *physics;
    struct renderer *renderContext;
    struct input *inputContext;
		struct statistics *statsContext;
    b32 isRunning;
    u32 msSinceStart;
};

#endif

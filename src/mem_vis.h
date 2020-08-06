#ifndef __MEM_VIS_H
#define __MEM_VIS_H

#include "types.h"

struct basic_dict;
struct renderer;
struct memory;

struct mem_vis
{
    struct renderer *renderContext;
    struct memory *targetMemory;
    i32 rendererModel;
    b32 isActive;
};

struct mem_vis *
mem_vis_create(struct renderer *renderContext, struct memory *targetMemory);

void
mem_vis_toggle(struct mem_vis *context, b32 b);

void
mem_vis_destroy(struct mem_vis *context);

#endif
#include "mem_vis.h"
#include "memory.h"
#include "renderer.h"

struct mem_vis *
mem_vis_create(struct renderer *renderContext, struct memory *targetMemory)
{
    struct mem_vis *context = memory_alloc(renderContext->memoryContext, sizeof(struct mem_vis));
    context->renderContext = renderContext;
    context->targetMemory = targetMemory;
    context->isActive = B32_FALSE;
    context->rendererModel = renderer_instantiate_model(renderContext, "quad", "texture");
}

void
mem_vis_toggle(struct mem_vis *context, b32 b)
{
    context->isActive = b;
}

void
mem_vis_destroy(struct mem_vis *context)
{
    // TODO:
}
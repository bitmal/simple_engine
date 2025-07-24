#ifndef __ENGINE_H
#define __ENGINE_H

#include "types.h"
#include "constants.h"
#include "memory.h"

struct context
{
    const struct memory_allocation_key configKey;
    const struct memory_allocation_key physicsKey;
    const struct memory_allocation_key renderKey;
    const struct memory_allocation_key inputKey;
    const struct memory_allocation_key statsKey;
    b32 isRunning;
    u32 msSinceStart;
};

#endif

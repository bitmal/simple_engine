#include "memory.h"

#include "utils.h"

#include <SDL3/SDL_timer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <assert.h>
#if defined(_WIN32) || defined(__WIN32) || defined(WIN32)
    #include <malloc.h>
#elif defined(linux)
    #include <malloc.h>
#endif

static i64 g_MEMORY_ALLOCATION_ID_COUNTER = 0;
static i64 g_MEMORY_EVENT_ID_COUNTER = 0;
static i32 g_MEMORY_ID_COUNTER = 0;

#define MEMORY_USER_REGION_MEMORY_IDENTIFIER (0xDEAD)

struct memory_user_region_section_header
{
    u16 identifier;
    enum memory_user_region_section_status_t status;
    u64 byteSize;
    u64 allocCount;
};

struct memory_safe_ptr
{
    memory_wide_id ptrId;
    struct memory_user_region_section_header *sectionPtr;
    p64 allocatorByteOffset;
    u64 refCount;
    b32 isActive;
};

static void
_memory_event_push(struct memory *mem, enum memory_event_type type, void *eventData)
{
#ifdef MEMORY_DEBUG
    i32 eventIndex;
    u32 freeEvents = mem->_eventCapacity - mem->_eventCount;
    if (freeEvents == 0)
    {
        if (mem->_eventCapacity > 0)
        {
            i32 nextEventIndex = mem->_nextEvent - mem->_events;
            mem->_events = realloc(mem->_events, sizeof(struct memory_event)*(mem->_eventCapacity *= 5));

            if (nextEventIndex > 0)
            {
                memmove(mem->_events, &mem->_events[nextEventIndex], mem->_eventCount);
            }

            eventIndex = mem->_eventCount;
        }
        else
        {
            eventIndex = 0;
            mem->_events = malloc(sizeof(struct memory_event)*(mem->_eventCapacity = 10));
        }
        
        mem->_nextEvent = mem->_events;
    }
    else
    {
        if (mem->_eventCount > 0)
        {
            eventIndex = (mem->_nextEvent - mem->_events) + mem->_eventCount;
        }
        else
        {
            eventIndex = 0;
            mem->_nextEvent = mem->_events;
        }
    }

    if (!utils_get_elapsed_ms(&mem->_events[eventIndex].elapsedMS))
    {
        mem->_events[eventIndex].elapsedMS = 0;
    }

    mem->_events[eventIndex].eventId = g_MEMORY_EVENT_ID_COUNTER++;
    mem->_events[eventIndex].type = type;

    switch (type)
    {
        case MEMORY_EVENT_FREE:
        case MEMORY_EVENT_ALLOC:
        {
            mem->_events[eventIndex].as_alloc. = *(u64 *)((u8 *)eventData - sizeof(u64) - sizeof(struct memory_allocator));
        } break;

        default: break;
    }
    mem->_events[eventIndex].byteOffset = (u8 *)alloc - mem->_memory - sizeof(u64);

    char fullFilepathBuffer[MEMORY_MAX_NAME_LENGTH + sizeof("_memory.log") + 1];
    fullFilepathBuffer[0] = '\0';
    strcat(fullFilepathBuffer, mem->name);
    strcat(fullFilepathBuffer, "_memory.log");

    FILE *logFile = fopen(fullFilepathBuffer, "a+");

    if (logFile)
    {
        fprintf(logFile, "[EventID(%ld) :: %.2f]: Event-type = '%d', Alloc-Id = (%ld)\n", mem->_events[eventIndex].eventId, 
            (real32)mem->_events[eventIndex].elapsedMS/1000.f, (i32)mem->_events[eventIndex].type,
            ((struct memory_allocator *)((u8 *)alloc - sizeof(struct memory_allocator)))->wideId);
            
        fclose(logFile);
    }

    ++mem->_eventCount;
#endif
}

static void
_memory_init(struct memory *mem, u64 size, u64 labelRegionSize, const char *name)
{
    assert(size > MEMORY_MIN_ALLOC_SIZE);

#ifdef MEMORY_DEBUG
    static u64 privateKey = 0;
    if (!privateKey)
    {
        srand((u32)time(NULL));
        privateKey = (u64)(((real64)rand()/(real64)RAND_MAX)*(real64)UINT64_MAX);
    }

    assert(mem->_internal != privateKey);

    *(u64 *)&mem->_internal = privateKey;
#endif

    mem->id = g_MEMORY_ID_COUNTER++;
    mem->_freeList = (struct memory_allocator *)((u64 *)mem->_memory + 1);
    mem->_freeList->wideId = g_MEMORY_ALLOCATOR_ID_COUNTER++;
    mem->_tailFree = mem->_freeList;
    mem->_freeList->_next = NULL;
    mem->reserved = sizeof(u64) + sizeof(struct memory_allocator);
    *(u64 *)mem->_memory = size - mem->reserved;
    
    char fullFilepathBuffer[MEMORY_MAX_NAME_LENGTH + sizeof("_memory.log") + 1];
    fullFilepathBuffer[0] = '\0';
    strcat(fullFilepathBuffer, mem->name);
    strcat(fullFilepathBuffer, "_memory.log");

    // discard previous log
    FILE *logFile = fopen(fullFilepathBuffer, "w");

    if (logFile)
    {
        fclose(logFile);
    }

    _memory_event_push(mem, MEMORY_EVENT_FREE, (struct memory_allocator *)mem->_freeList + 1);
}

void
memory_init(struct memory *mem, u64 size, u64 labelRegionSize, const char *name)
{
    assert(mem);

    _memory_init(mem, size, name);
}

void *
memory_alloc(struct memory *mem, u64 size)
{
    const u64 clampedSize = size >= MEMORY_MIN_ALLOC_SIZE ? size : MEMORY_MIN_ALLOC_SIZE;

    struct memory_allocator *prev = NULL;

    struct memory_allocator *block;
    for (block = mem->_freeList;
         block; 
         block = block->_next)
    {
        assert((p64)block >= ((p64)mem->_memory + sizeof(u64)));
        assert((p64)block < (p64)(mem->_memory + mem->size));
        
        u64 *blockSize = (u64 *)block - 1;

        if (clampedSize <= (*blockSize))
        {
            const u64 diff = ((*blockSize) > clampedSize) ? (*blockSize) - clampedSize : 0;

            if (diff >= MEMORY_MIN_BLOCK_SIZE)
            {
                const u64 splitDiff = (diff >= clampedSize) ? (diff - clampedSize) : 0;
                
                struct memory_allocator *splitAlloc;
                
                if (splitDiff >= MEMORY_MIN_BLOCK_SIZE)
                {
                    splitAlloc = (struct memory_allocator *)(((u8 *)block + sizeof(struct memory_allocator) + clampedSize + sizeof(u64)));
                    *((u64 *)splitAlloc - 1) = splitDiff - sizeof(u64) - sizeof(struct memory_allocator);
                    prev->_next = splitAlloc;
                    splitAlloc->_next = block->_next;
                }
                else 
                {
                    splitAlloc = NULL;
                }
                
                mem->_freeList = prev;

                if (block != mem->_tailFree && block != mem->_freeList)
                {
                    mem->_tailFree->_next = splitAlloc;
                    mem->_tailFree = splitAlloc;
                }
                else if (block == mem->_freeList && block == mem->_tailFree)
                {
                    mem->_freeList = mem->_tailFree = splitAlloc;
                }
                else
                {
                    mem->_tailFree->_next = splitAlloc;
                    mem->_tailFree = splitAlloc;

                    if (!prev)
                    {
                        mem->_freeList = splitAlloc;
                    }
                    else
                    {
                        prev->_next = splitAlloc;
                    }
                }

                mem->reserved += clampedSize + sizeof(u64) + sizeof(struct memory_allocator);
            }
            else
            {
                if (mem->_freeList == mem->_tailFree)
                {
                    mem->_freeList = mem->_tailFree = NULL;
                }
                else if (prev)
                {
                    prev->_next = block->_next;
                }
                else
                {
                    mem->_freeList = block->_next;
                }
                
                mem->reserved += *((u64 *)block - 1);
            }


            break;
        }

        prev = block;
    }

    if (block)
    {
        block->wideId = g_MEMORY_ALLOCATOR_ID_COUNTER++;
        _memory_event_push(mem, (u8 *)block + sizeof(struct memory_allocator), MEMORY_EVENT_ALLOC);
    }

    return block;
}

void *
memory_realloc(struct memory *mem, void *alloc, u64 size)
{
    assert((u8 *)alloc >= mem->_memory && (u8 *)alloc < (mem->_memory + mem->size));
    assert(*((u64 *)alloc - 1) <= (mem->reserved - sizeof(u64)));

    struct memory_allocator *lhs = alloc;
    const u64 lhsSize = *((u64 *)lhs - 1);

    if (size > lhsSize)
    {
        struct memory_allocator *rhs = memory_alloc(mem, size);

        if (!rhs)
        {
            memory_clean(mem);
            rhs = memory_alloc(mem, size);
        }

        if (rhs)
        {
            memcpy(rhs, lhs, lhsSize);
            memory_free(mem, lhs);
        }

        return rhs;
    }
    else if (size < lhsSize)
    {
        const u64 diff = lhsSize - size;

        if (diff >= MEMORY_MIN_BLOCK_SIZE)
        {
            _memory_event_push(mem, (u8 *)lhs + sizeof(struct memory_allocator), MEMORY_EVENT_FREE);
            *((u64 *)lhs - 1) = size;
            _memory_event_push(mem, (u8 *)lhs + sizeof(struct memory_allocator), MEMORY_EVENT_ALLOC);
            struct memory_allocator *rhs = (struct memory_allocator *)((u8 *)lhs + size + sizeof(u64));
            rhs->wideId = g_MEMORY_ALLOCATOR_ID_COUNTER++;
            *((u64 *)rhs - 1) = diff - sizeof(u64);
            _memory_event_push(mem, (u8 *)rhs + sizeof(struct memory_allocator), MEMORY_EVENT_ALLOC);
            rhs->_next = NULL;

            if (mem->_tailFree)
            {
                mem->_tailFree->_next = rhs;
                mem->_tailFree = rhs;
            }
            else
            {
                mem->_freeList = mem->_tailFree = rhs;
            }

            mem->reserved -= diff - sizeof(u64);
        }
    }

    return lhs;
}

u64
memory_sizeof(struct memory *mem, void *alloc)
{
    assert(alloc);

    return *((u64 *)alloc - 1);
}

void
memory_clean(struct memory *mem)
{
    b32 isSorted = B32_FALSE;

    do
    {
        isSorted = B32_TRUE;

        for (struct memory_allocator *n = (struct memory_allocator *)&mem->_freeList;
             n && n->_next && n->_next->_next;
             n = n->_next)
        {
            struct memory_allocator *n1 = n->_next;
            struct memory_allocator *n2 = n->_next->_next;
            struct memory_allocator *n3 = n->_next->_next->_next;

            if (n2 < n1)
            {
                if (n1 == mem->_freeList)
                {
                    mem->_freeList = n2;
                }
                if (n2 == mem->_tailFree)
                {
                    mem->_tailFree = n1;
                }

                n->_next = n2;
                n1->_next = n3;
                n2->_next = n1;

                isSorted = B32_FALSE;
            }
        }
    } while(!isSorted);

    if (isSorted)
    {
        for (struct memory_allocator *n = (struct memory_allocator *)mem->_freeList;
             n;
             n = n->_next)
        {
            for (struct memory_allocator *n1 = n->_next;
                 n1;
                 n1 = n1->_next)
            {
                u64 nSize = memory_sizeof(mem, n);
                u64 nSize1 = memory_sizeof(mem, n1);
                
                if (((u8 *)n + nSize + sizeof(u64)) == (u8 *)n1)
                {
                    *((u64 *)n - 1) += sizeof(u64) + nSize1;
                    n->_next = n1->_next;

                    mem->reserved -= sizeof(u64);

                    if (n1 == mem->_tailFree)
                    {
                        mem->_tailFree = n;
                    }
                }
            }
        }
    }
}

void
memory_free(struct memory *mem, void *alloc)
{
    assert(((uint64_t)alloc >= (uint64_t)mem->_memory) && (uint64_t)alloc < ((uint64_t)mem->_memory + mem->size));

    struct memory_allocator *casted = alloc;

    if (mem->_tailFree)
    {
        mem->_tailFree->_next = casted;
        mem->_tailFree = casted;
    }
    else
    {
        mem->_freeList = mem->_tailFree = casted;
    }

    mem->_tailFree->_next = NULL;

    mem->reserved -= *((u64 *)casted - 1);

    _memory_event_push(mem, alloc, MEMORY_EVENT_FREE);
}

const struct memory_event *
memory_event_pop(struct memory *mem)
{
#ifdef MEMORY_DEBUG
    struct memory_event *ev = mem->_nextEvent;

    if (ev)
    {
        i32 eventElement = ev - mem->_events;

        if ((eventElement + 1) < (eventElement + mem->_eventCount))
        {
            mem->_nextEvent = &mem->_events[eventElement + 1];
        }
        else
        {
            mem->_nextEvent = NULL;
        }
        
        --mem->_eventCount;
    }
    return ev;
#else
    return NULL;
#endif
}

void
memory_set_offset(struct memory *memPtr, u64 memOffset, b32 isValueRepeating, u64 valueSize, const void *valuePtr)
{
    assert(((valueSize > 0) ? (valuePtr != NULL) : B32_TRUE));
    assert(memOffset < memPtr->size);

    const u64 valueSizeBounded = valueSize > 0 ? valueSize : sizeof(u32);

    u32 valueBlockSize;
    if (valueSizeBounded < 8)
    {
        valueBlockSize = 0x4;
    }
    else 
    {
        valueBlockSize = 0x8;
    }

    const u32 valueBlockSizeRemainder = valueSizeBounded%valueBlockSize;
    const u64 valueBlockCount = (memPtr->size - memOffset)/valueBlockSize;
    
    const u8 *valuePtrBounded;
    if (valueSize > 0)
    {
        valuePtrBounded = valuePtr;
    }
    else 
    {
        valuePtrBounded = memory_alloc(memPtr, valueBlockSize);
    }

    for (u64 valueBlockIndex = 0; valueBlockIndex < valueBlockCount; ++valueBlockIndex)
    {
        u64 valueBlockByteOffset = valueBlockIndex*(valueBlockSize + valueBlockSizeRemainder);

        switch (valueBlockSize)
        {
            case 0x4:
            {
                *(u32 *)(&memPtr->_memory[valueBlockByteOffset]) = *(const u32 *)valuePtrBounded;

                for (u32 remainderByteOffset = 0; remainderByteOffset < valueBlockSizeRemainder; ++remainderByteOffset)
                {
                    memPtr->_memory[valueBlockByteOffset + valueBlockSize + remainderByteOffset] = ((u8 *)valuePtrBounded)[valueBlockSize];
                }
            } break;
            
            case 0x8:
            {
                *(u64 *)(&memPtr->_memory[valueBlockByteOffset]) = *(const u64 *)valuePtrBounded;
                
                for (u32 remainderMultipleByteOffset = 0; remainderMultipleByteOffset < valueBlockSizeRemainder; ++remainderMultipleByteOffset)
                {
                    memPtr->_memory[valueBlockByteOffset + valueBlockSize + (remainderMultipleByteOffset*sizeof(u64))] = ((u8 *)valuePtrBounded)[valueBlockSize];
                }
                
                const u32 remainderMultipleByteRemainder = valueBlockSizeRemainder%sizeof(u64);
                const u32 remainderMultipleCount = valueBlockSizeRemainder/sizeof(u64);

                for (u32 remainderMultipleByteRemainderOffset = 0; remainderMultipleByteRemainderOffset < remainderMultipleByteRemainder; ++remainderMultipleByteRemainderOffset)
                {
                    memPtr->_memory[valueBlockByteOffset + valueBlockSize + (remainderMultipleCount*sizeof(u64)) + remainderMultipleByteRemainderOffset] = 
                        valuePtrBounded[valueBlockSize + (remainderMultipleCount*sizeof(u64)) + remainderMultipleByteRemainderOffset];
                }
            } break;

            default: break;
        }
    }

    if (valuePtrBounded != valuePtr)
    {
        memory_free(memPtr, (u8 *)valuePtrBounded);
    }
}
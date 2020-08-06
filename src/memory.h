#ifndef __MEMORY_H
#define __MEMORY_H

#include "types.h"

#define MEMORY_MIN_ALLOC_SIZE (sizeof(struct memory_allocator *))
#define MEMORY_MIN_BLOCK_SIZE (MEMORY_MIN_ALLOC_SIZE + sizeof(u64))

struct memory_allocator
{
    struct memory_allocator *_next;
};

enum memory_event_type
{
    MEMORY_EVENT_ALLOC,
    MEMORY_EVENT_FREE
};

// TODO: events should only be available in debug
struct memory_event
{
    enum memory_event_type type;
    u64 size;
    u64 byteOffset;
};

struct memory
{
#ifdef MEMORY_DEBUG
    const u64 _internal;
#endif
    const u64 size;
    u64 reserved;
    struct memory_allocator *_freeList;
    struct memory_allocator *_tailFree;
    struct memory_event *_events;
    struct memory_event *_nextEvent;
    u32 _eventCount;
    u32 _eventCapacity;
    u8 _memory[];
};

#define MEMORY_TYPE_NAME(name) name##_memory
#define MEMORY_PTR(name) ((struct memory *)&MEMORY_TYPE_NAME(name))
#ifdef MEMORY_DEBUG
    #define _MEMORY_DEFINE_HEAD(name) \
        struct MEMORY_TYPE_NAME(name)##_t \
        { \
            const u64 _internal;
    #define _MEMORY_DEFINE_INTERNAL_INIT \
        0,
#else
    #define _MEMORY_DEFINE_HEAD(name) \
        struct MEMORY_TYPE_NAME(name)##_t \
        {
    #define _MEMORY_DEFINE_INTERNAL_INIT
#endif
#define MEMORY_DEFINE(name, size) \
    _MEMORY_DEFINE_HEAD(name) \
        const u64 _size; \
        u64 _reserved; \
        struct memory_allocator *_freeList; \
        struct memory_allocator *_tailFree; \
        struct memory_event *_events; \
        struct memory_event *_nextEvent; \
        u32 _eventCount; \
        u32 _eventCapacity; \
        u8 _memory[size]; \
    } MEMORY_TYPE_NAME(name) = { \
        _MEMORY_DEFINE_INTERNAL_INIT sizeof(MEMORY_TYPE_NAME(name)._memory), \
            0, NULL, NULL, NULL, NULL, 0, 0, {0} \
    }
#define MEMORY_INIT(name) \
    memory_init(MEMORY_PTR(name));

void
memory_init(struct memory *mem);

void *
memory_alloc(struct memory *mem, u64 size);

void *
memory_realloc(struct memory *mem, void *alloc, u64 size);

u64
memory_sizeof(struct memory *mem, void *alloc);

void
memory_clean(struct memory *mem);

void
memory_free(struct memory *mem, void *alloc);

const struct memory_event *
memory_event_pop(struct memory *mem);

#endif
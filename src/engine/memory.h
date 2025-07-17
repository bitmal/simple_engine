#ifndef __MEMORY_H
#define __MEMORY_H

#include "types.h"

#define MEMORY_MIN_ALLOC_SIZE (sizeof(struct memory_allocator *))
#define MEMORY_MIN_BLOCK_SIZE (MEMORY_MIN_ALLOC_SIZE + sizeof(u64))
#define MEMORY_MAX_NAME_LENGTH 255

typedef i64 memory_wide_id;
typedef i32 memory_id;

struct memory_allocator
{
    memory_wide_id wideId;
    struct memory_allocator *_next;
};

enum memory_event_type
{
    MEMORY_EVENT_ALLOC,
    MEMORY_EVENT_FREE
};

struct memory_event
{
    memory_wide_id eventId;
    enum memory_event_type type;
    u32 elapsedMS;
    u64 size;
    u64 byteOffset;
};

struct memory
{
#ifdef MEMORY_DEBUG
    const u64 _internal;
#endif
    memory_id id;
    char name[MEMORY_MAX_NAME_LENGTH + 1];
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

#define MEMORY_TYPE_NAME(name)name##_memory##_t
#define MEMORY_TYPE_ALIAS(name)name##_memory
#define MEMORY_PTR(name)((struct memory *)&MEMORY_TYPE_ALIAS(g_##name))
#ifdef MEMORY_DEBUG
    #define _MEMORY_DEFINE_HEAD(name) \
        typedef struct MEMORY_TYPE_NAME(name) \
        { \
            const u64 _internal;
#else
    #define _MEMORY_DEFINE_HEAD(name) \
        typedef struct MEMORY_TYPE_NAME(name) \
        {
#endif
#define MEMORY_DEFINE(name, size) \
    _MEMORY_DEFINE_HEAD(name) \
        memory_id id; \
        char name[MEMORY_MAX_NAME_LENGTH + 1]; \
        const u64 _size; \
        u64 _reserved; \
        struct memory_allocator *_freeList; \
        struct memory_allocator *_tailFree; \
        struct memory_event *_events; \
        struct memory_event *_nextEvent; \
        u32 _eventCount; \
        u32 _eventCapacity; \
        u8 _memory[size]; \
    } MEMORY_TYPE_ALIAS(name); \
    static MEMORY_TYPE_ALIAS(name) g_##name##_memory
#define MEMORY_INIT(label, nameStr) \
    memory_init(MEMORY_PTR(label), sizeof(g_##label##_memory._memory), nameStr);
#define MEMORY_CLEAR(memoryPtr, patternPtr, patternSizePtr) \
    assert((patternPtr) ? ((patternSizePtr) && (*(u64 *)(patternSizePtr) > 0)) : B32_TRUE); \
    memory_set_offset(memoryPtr, (u64)0, B32_TRUE, (patternSizePtr) ? (*(u64 *)(patternSizePtr)) : (u64)0x0, patternPtr)
#define MEMORY_ZERO(memoryPtr) \
    memory_set_offset(memoryPtr, (u64)0, B32_TRUE, (u64)0, NULL)

void
memory_init(struct memory *mem, u64 size, const char *name);

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

void
memory_set_offset(struct memory *memPtr, u64 memOffset, b32 isValueRepeating, u64 valueSize, const void *valuePtr);

#endif
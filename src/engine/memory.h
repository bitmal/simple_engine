#ifndef __MEMORY_H
#define __MEMORY_H

#include "types.h"

#define MEMORY_MIN_ALLOC_SIZE (sizeof(struct memory_allocator *))
#define MEMORY_MIN_BLOCK_SIZE (MEMORY_MIN_ALLOC_SIZE + sizeof(u64))
#define MEMORY_MAX_NAME_LENGTH 255
#define MEMORY_LABEL_REGION_DEFAULT_PROPORTION_TO_SIZE (0.25f)

typedef i64 memory_wide_id;
typedef i32 memory_id;

struct memory_smart_ptr;

struct memory_allocator
{
    memory_wide_id wideId;
    char *allocLabel;
    u64 allocSize;
    struct memory_allocator *_next;
};

enum memory_event_type
{
    MEMORY_EVENT_BLOCK,
    MEMORY_EVENT_ALLOC,
    MEMORY_EVENT_FREE,
    MEMORY_EVENT_TYPE_COUNT
};

struct alloc_info
{
    memory_wide_id allocId;
    char *allocLabel;
    u64 allocSize;
};

struct memory_event
{
    memory_wide_id eventId;
    enum memory_event_type type;
    u32 elapsedMS;

    union
    {
        struct
        {
            u64 allocSize;
            u64 allocByteOffset;
        } as_alloc;
        
        struct
        {
            u64 allocSize;
            u64 allocByteOffset;
        } as_free;
    };
};

struct memory
{
#ifdef MEMORY_DEBUG
    const u64 _internal;
#endif
    memory_id id;
#ifdef MEMORY_DEBUG
    char name[MEMORY_MAX_NAME_LENGTH + 1];
#endif
    u64 bytesCapacity;
    u64 smartPtrRegionBytesCapacity;
#ifdef MEMORY_DEBUG
    u64 labelRegionBytesCapacity;
#endif
    u64 allocatedUserRegionBytes;
    struct memory_allocator *freeList;
    struct memory_allocator *tailFree;
    u32 eventQueueCount;
    u32 eventQueueCapacity;
    u8 userRegionBytes[];
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
        u64 bytesCapacity; \
        u64 smartPtrRegionBytesCapacity; \
        u64 labelRegionBytesCapacity; \
        u64 allocatedUserRegionBytes; \
        struct memory_allocator *freeList; \
        struct memory_allocator *tailFree; \
        u32 eventQueueCount; \
        u32 eventQueueCapacity; \
        u8 userRegionBytes[size]; \
    } MEMORY_TYPE_ALIAS(name); \
    static MEMORY_TYPE_ALIAS(name) g_##name##_memory
#define MEMORY_INIT(label, nameStr) \
    memory_init(MEMORY_PTR(label), sizeof(g_##label##_memory._memory), \
    (u64)(real64)sizeof(g_##label_##_memory._memory)*MEMORY_LABEL_REGION_DEFAULT_PROPORTION_TO_SIZE), nameStr);
#define MEMORY_CLEAR(memoryPtr, patternPtr, patternSizePtr) \
    assert((patternPtr) ? ((patternSizePtr) && (*(u64 *)(patternSizePtr) > 0)) : B32_TRUE); \
    memory_set_offset(memoryPtr, (u64)0, B32_TRUE, (patternSizePtr) ? (*(u64 *)(patternSizePtr)) : (u64)0x0, patternPtr)
#define MEMORY_ALLOC_DEFAULT_LABEL(memory, size) \
    memory_alloc(memory, size, "DEFAULT")

void
memory_init(struct memory *mem, u64 size, u64 labelRegionSize, const char *name);

void *
memory_alloc(struct memory *mem, u64 size, const char *label);

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
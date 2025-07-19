#ifndef __MEMORY_H
#define __MEMORY_H

#include "types.h"

#include <stdint.h>

typedef u64 memory_id;
#define MEMORY_ID_NULL ((memory_id)0)
#define MEMORY_ID_MAX ((memory_id)UINT64_MAX)

typedef u32 memory_int_id;
#define MEMORY_INT_ID_NULL ((memory_int_id)0)
#define MEMORY_INT_ID_MAX ((memory_id)UINT32_MAX)

typedef u16 memory_short_id;
#define MEMORY_SHORT_ID_NULL ((memory_short_id)0)
#define MEMORY_SHORT_ID_MAX ((memory_id)UINT16_MAX)

typedef u8 memory_byte_id;
#define MEMORY_BYTE_ID_NULL ((memory_byte_id)0)
#define MEMORY_BYTE_ID_MAX ((memory_id)UINT8_MAX)

typedef u8 memory_error_code;

enum memory_page_status_t
{
    MEMORY_PAGE_STATUS_UNKNOWN,
    MEMORY_PAGE_STATUS_FREED,
    MEMORY_PAGE_STATUS_UNLOCKED,
    MEMORY_PAGE_STATUS_LOCKED,
    MEMORY_PAGE_STATUS_PROTECTED,
    MEMORY_PAGE_STATUS_TYPE_COUNT
};

enum memory_event_type
{
    MEMORY_EVENT_BLOCK,
    MEMORY_EVENT_ALLOC,
    MEMORY_EVENT_FREE,
    MEMORY_EVENT_TYPE_COUNT
};

#define MEMORY_MAX_USER_REGION_PAGES (INT8_MAX)
#define MEMORY_MAX_NAME_LENGTH (UINT8_MAX - 1)

struct memory_context;

#define MEMORY_TYPE_NAME(name)name##_memory##_t
#define MEMORY_TYPE_ALIAS(name)name##_memory
#define MEMORY_PTR(name)((struct memory *)&MEMORY_TYPE_ALIAS(g_##name))
#ifdef MEMORY_DEBUG
    #define _MEMORY_DEFINE_HEAD(name) \
        struct MEMORY_TYPE_NAME(name) \
        { \
            const u64 _internal;
#else
    #define _MEMORY_DEFINE_HEAD(typeName) \
        typedef struct MEMORY_TYPE_NAME(name) \
        {
#endif
#define MEMORY_BEGIN_DECLARATION(typeName) \
    _MEMORY_DEFINE_HEAD(typeName) \
        memory_id id; \
        char label[MEMORY_MAX_NAME_LENGTH + 1]; \
        u64 bytesCapacity; \
        u64 labelRegionBytesCapacity; \
        u64 smartPtrRegionBytesCapacity; \
        u64 userRegionBytesCapacity; \
        struct memory_allocator *freeList; \
        struct memory_allocator *tailFree; \
        u32 eventQueueCapacity; \
        u32 eventQueueReadIndex; \
        u32 eventQueueWriteIndex; \
        u64 userSectionByteOffsets[MEMORY_MAX_USER_REGION_SECTIONS];
#define MEMORY_END_DECLARATION(name, heapCapacity) \
        u8 heap[heapCapacity]; \
    }; \
    static MEMORY_TYPE_ALIAS(name) g_##name##_memory
#define MEMORY_BASIC_DECLARATION(typeName, heapCapacity) \
    MEMORY_BEGIN_DECLARATION(name) \
    MEMORY_END_DECLARATION(name, heapCapacity)

#define MEMORY_ERROR_NULL_ID ((memory_error_code)0)
#define MEMORY_OK ((memory_error_code)1)
#define MEMORY_ERROR_INDEX_OUT_OF_RANGE ((memory_error_code)2)
#define MEMORY_ERROR_RANDOM_NOT_SEEDED ((memory_error_code)2)

memory_error_code
memory_create_debug_context(u8 *correspondingHeaps[3], p64 heapOffsets[3], u64 labelRegionCapacity, u64 smartPtrRegionCapacity, 
    u64 userRegionCapacity, memory_short_id *outputMemoryDebugContextId);

memory_error_code
memory_create_context(u8 *heap, p64 heapOffset, u64 heapCapacity, memory_short_id *outputMemoryContextId);

memory_error_code
memory_alloc_page(memory_short_id memoryContextId, u64 byteSize, memory_byte_id *outputPageId);

memory_error_code
memory_dealloc_page(memory_short_id memoryContextId, memory_byte_id pageId);

memory_error_code
memory_realloc_page(memory_short_id memoryContextId, memory_byte_id pageId);

enum memory_page_status_t
memory_page_get_status(memory_short_id memoryContextId, memory_byte_id pageId);

memory_error_code
memory_page_unlock(memory_short_id memoryContextId, memory_byte_id pageId);

memory_error_code
memory_page_lock(memory_short_id memoryContextId, memory_byte_id pageId);

memory_error_code
memory_alloc(memory_short_id memoryContextId, memory_byte_id pageId, memory_id *outputAllocId);

memory_error_code
memory_dealloc(memory_short_id memoryContextId, memory_byte_id pageId, memory_id allocId);

u64
memory_sizeof(memory_short_id memoryContextId, memory_byte_id pageId, memory_id allocId);

memory_error_code
memory_clean(memory_short_id memoryContextId);

#endif
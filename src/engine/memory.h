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

#define MEMORY_MAX_USER_REGION_PAGES (MEMORY_SHORT_ID_MAX)
#define MEMORY_MAX_NAME_LENGTH (UINT8_MAX - 1)

struct memory_context;

#define MEMORY_ERROR_NULL_ID ((memory_error_code)0)
#define MEMORY_OK ((memory_error_code)1)
#define MEMORY_ERROR_UNKNOWN ((memory_error_code)2)
#define MEMORY_ERROR_INDEX_OUT_OF_RANGE ((memory_error_code)3)
#define MEMORY_ERROR_RANDOM_NOT_SEEDED ((memory_error_code)4)
#define MEMORY_ERROR_SIZE_TOO_SMALL ((memory_error_code)5)
#define MEMORY_ERROR_NOT_IMPLEMENTED ((memory_error_code)6)
#define MEMORY_ERROR_TOO_MANY_OBJECTS ((memory_error_code)7)
#define MEMORY_ERROR_NULL_PARAMETER ((memory_error_code)8)
#define MEMORY_ERROR_FAILED_ALLOCATION ((memory_error_code)9)
#define MEMORY_ERROR_ZERO_PARAMETER ((memory_error_code)10)

memory_error_code
memory_alloc_raw_heap(u8 **outResult, u64 byteSize);

memory_error_code
memory_realloc_raw_heap(u8 **outResult, u64 byteSize);

memory_error_code
memory_free_raw_heap(u8 **heap);

void
memory_set_global_seed(i64 seed);

i64
memory_get_global_seed();

memory_error_code
memory_create_debug_context(u8 *heap, const char *label, p64 heapByteOffset, p64 labelRegionByteOffset, u64 labelRegionByteCapacity, 
    p64 safePtrRegionByteOffset, u64 safePtrRegionByteCapacity, p64 userRegionByteOffset, u64 userRegionByteCapacity, memory_short_id *outputMemoryDebugContextId);

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
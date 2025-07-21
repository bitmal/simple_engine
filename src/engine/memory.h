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

#define MEMORY_MAX_PAGES_REGION_PAGES (MEMORY_SHORT_ID_MAX)
#define MEMORY_MAX_RAW_ALLOCS (MEMORY_SHORT_ID_MAX - 1)
#define MEMORY_MAX_ALLOCS (MEMORY_INT_ID_MAX - 1)
#define MEMORY_MAX_NAME_LENGTH (UINT8_MAX - 1)

struct memory_raw_allocation_key;
struct memory_allocation_key;
struct memory_context_key;

struct memory_context_diagnostic_info
{
    const char *label;
    u64 heapReservedBytes;
    u64 pagesRegionReservedBytes;
    u16 pagesRegionActiveCount;
    u32 allocationCount;
    b32 isDebug;
};

struct memory_diagnostic_info
{
    u16 rawAllocationCount;
    u16 contextCount;
    u16 debugContextCount;
};

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
#define MEMORY_ERROR_REQUESTED_HEAP_REGION_SIZE_TOO_SMALL ((memory_error_code)11)
#define MEMORY_ERROR_REQUESTED_HEAP_REGION_SIZE_TOO_LARGE ((memory_error_code)12)

memory_error_code
memory_alloc_raw_allocation(const struct memory_raw_allocation_key *outRawAllocKeyPtr, u64 byteSize);

memory_error_code
memory_realloc_raw_allocation(const struct memory_raw_allocation_key *rawAllocKeyPtr, u64 byteSize);

memory_error_code
memory_free_raw_allocation(const struct memory_raw_allocation_key *outRawAllocKeyPtr);

memory_error_code
memory_map_raw_allocation(const struct memory_raw_allocation_key *rawAllocKeyPtr, void **outDataPtr);

memory_error_code
memory_unmap_raw_allocation(const struct memory_raw_allocation_key *rawAllocKeyPtr, void **outDataPtr);

memory_error_code
memory_create_debug_context(u64 safePtrRegionByteCapacity, u64 pagesRegionByteCapacity, u64 labelRegionByteCapacity, 
    const char *contextLabel, const struct memory_context_key *outputMemoryDebugContextKeyPtr);

memory_error_code
memory_create_context(u64 safePtrRegionByteCapacity, u64 pagesRegionByteCapacity, 
    const struct memory_context_key *outputMemoryContextKeyPtr);

memory_error_code
memory_alloc_page(memory_short_id memoryContextId, u64 byteSize, memory_short_id *outPageIdPtr);

memory_error_code
memory_free_page(memory_short_id memoryContextId, memory_short_id pageId);

memory_error_code
memory_realloc_page(memory_short_id memoryContextId, memory_short_id pageId);

enum memory_page_status_t
memory_page_get_status(memory_short_id memoryContextId, memory_short_id pageId);

memory_error_code
memory_page_unlock(memory_short_id memoryContextId, memory_short_id pageId);

memory_error_code
memory_page_lock(memory_short_id memoryContextId, memory_short_id pageId);

memory_error_code
memory_page_clean(memory_short_id memoryContextId, memory_short_id pageId);

memory_error_code
memory_pages_reorder(memory_short_id memoryContextId);

memory_error_code
memory_alloc(memory_short_id memoryContextId, const struct memory_allocation_key *outputAllocKeyPtr);

memory_error_code
memory_dealloc(memory_short_id memoryContextId, const struct memory_allocation_key *allocKeyPtr);

memory_error_code
memory_map_alloc(memory_short_id memoryContextId, const struct memory_allocation_key *allocKeyPtr, void **outAllocPtr);

memory_error_code
memory_unmap_alloc(memory_short_id memoryContextId, const struct memory_allocation_key *allocKeyPtr, void **outAllocPtr);

memory_error_code
memory_get_active_alloc_count(memory_short_id memoryContextId);

memory_error_code
memory_get_free_alloc_count(memory_short_id memoryContextId);

memory_error_code
memory_context_get_diagnostic_info(memory_short_id memoryContextId, const struct memory_context_diagnostic_info *outDiagInfoPtr);

memory_error_code
memory_get_diagnostic_info(const struct memory_diagnostic_info *outDiagInfoPtr);

u64
memory_sizeof(memory_short_id memoryContextId, const struct memory_allocation_key *allocKeyPtr);

u64
memory_raw_alloc_sizeof(memory_short_id memoryContextId, const struct memory_raw_allocation_key *rawAllocKeyPtr);

#endif
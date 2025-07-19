#include "memory.h"
#include "types.h"
#include "utils.h"

#include <stdint.h>
#include <math.h>
#include <time.h>
#include <assert.h>

#if defined(_WIN32) || defined(__WIN32) || defined(WIN32)
    #include <malloc.h>
#elif defined(linux)
    #include <malloc.h>
    #include <stdlib.h>
#endif

#define MEMORY_USER_REGION_PAGE_IDENTIFIER (0xDEAD)

struct memory_user_region_page_header
{
    u16 identifier;
    enum memory_page_status_t status;
    u64 byteSize;
    u64 allocCount;
};

struct memory_safe_ptr
{
    memory_id allocId;
    memory_id pageId;
    p64 allocByteOffset;
    u64 refCount;
    b32 isActive;
};

struct memory_user_region_page_allocator
{
    u16 identifier;
    u8 byteOffset;
    u8 __padding;
    u16 _padding[5];
    u64 byteSize;
};

#define MEMORY_DEBUG
struct memory_context
{
    memory_id id;
    u64 bytesCapacity;
    struct memory_allocator *freeList;
    struct memory_allocator *tailFree;
    u64 userRegionPageByteOffsets[MEMORY_MAX_USER_REGION_PAGES];
    u8 *heaps;
#if defined(MEMORY_DEBUG)
    char label[MEMORY_MAX_NAME_LENGTH + 1];
    u64 labelRegionBytesCapacity;
    u64 smartPtrRegionBytesCapacity;
    u64 userRegionBytesCapacity;
    u32 eventQueueCapacity;
    u32 eventQueueReadIndex;
    u32 eventQueueWriteIndex;
#endif
};

struct memory_event
{
    memory_id eventId;
    enum memory_event_type type;
    u32 elapsedMS;

    union
    {
        struct
        {
            memory_id userSectionId;
            memory_id allocId;
            u64 allocSize;
            u64 allocByteOffset;
        } as_alloc;
        
        struct
        {
            memory_id userSectionId;
            memory_id allocId;
            u64 allocSize;
            u64 allocByteOffset;
        } as_free;
    };
};

static memory_error_code
_memory_generate_unique_byte_id(i64 *seedPtr, memory_byte_id *outResult)
{
    assert(outResult);
    
    static b32 isSeeded = B32_FALSE;

    if (seedPtr)
    {
        isSeeded = B32_TRUE;
    }
    
    memory_error_code errorResult;

    if (isSeeded)
    {
        *outResult = (memory_byte_id)(round(UTILS_GENERATE_RANDOM_POSITIVE_REAL64(seedPtr)*UINT8_MAX));
        errorResult = MEMORY_OK;
    }
    else
    {
        *outResult = MEMORY_BYTE_ID_NULL;
        errorResult = MEMORY_ERROR_RANDOM_NOT_SEEDED;
    }

   return errorResult;
}

static memory_error_code
_memory_generate_unique_short_id(i64 *seedPtr, memory_short_id *outResult)
{
    assert(outResult);
    
    static b32 isSeeded = B32_FALSE;

    if (seedPtr)
    {
        isSeeded = B32_TRUE;
    }
    
    memory_error_code errorResult;

    if (isSeeded)
    {
        *outResult = (memory_short_id)(round(UTILS_GENERATE_RANDOM_POSITIVE_REAL64(seedPtr)*UINT16_MAX));
        errorResult = MEMORY_OK;
    }
    else
    {
        *outResult = MEMORY_SHORT_ID_NULL;
        errorResult = MEMORY_ERROR_RANDOM_NOT_SEEDED;
    }

   return errorResult;
}

static memory_error_code
_memory_generate_unique_int_id(i64 *seedPtr, memory_int_id *outResult)
{
    assert(outResult);
    
    static b32 isSeeded = B32_FALSE;

    if (seedPtr)
    {
        isSeeded = B32_TRUE;
    }
    
    memory_error_code errorResult;

    if (isSeeded)
    {
        *outResult = (memory_int_id)(round(UTILS_GENERATE_RANDOM_POSITIVE_REAL64(seedPtr)*UINT32_MAX));
        errorResult = MEMORY_OK;
    }
    else
    {
        *outResult = MEMORY_INT_ID_NULL;
        errorResult = MEMORY_ERROR_RANDOM_NOT_SEEDED;
    }

   return errorResult;
}

static memory_error_code
_memory_generate_unique_id(u16 *seedPtr, memory_id *outResult)
{
    assert(outResult);
    
    static b32 isSeeded = B32_FALSE;

    if (seedPtr)
    {
        isSeeded = B32_TRUE;
    }
    
    memory_error_code errorResult;

    if (isSeeded)
    {
        *outResult = (memory_id)(utils_generate_random_u64(seedPtr));
        errorResult = MEMORY_OK;
    }
    else
    {
        *outResult = MEMORY_INT_ID_NULL;
        errorResult = MEMORY_ERROR_RANDOM_NOT_SEEDED;
    }

   return errorResult;
}

memory_error_code
memory_create_debug_context(u8 *correspondingHeaps[3], p64 heapOffsets[3], u64 labelRegionCapacity, u64 smartPtrRegionCapacity, 
    u64 userRegionCapacity, memory_short_id *outputMemoryDebugContextId)
{
    assert(0);

    if (!outputMemoryDebugContextId)
    {
        return MEMORY_ERROR_NULL_ID;
    }

    *outputMemoryDebugContextId = _memory_generate_unique_short_id(); // TODO: seed ???

    return MEMORY_OK;
}

memory_error_code
memory_create_context(u8 *heap, p64 heapOffset, u64 heapCapacity, memory_short_id *outputMemoryContextId)
{
    assert(0);
    
    // TODO:

    return MEMORY_OK;
}
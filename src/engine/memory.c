#include "memory.h"
#include "types.h"

#include <stdint.h>

#if defined(_WIN32) || defined(__WIN32) || defined(WIN32)
    #include <malloc.h>
#elif defined(linux)
    #include <malloc.h>
#endif

#define MEMORY_USER_REGION_MEMORY_IDENTIFIER (0xDEAD)

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

struct memory_context
{
    memory_id id;
    char label[MEMORY_MAX_NAME_LENGTH + 1];
    u64 bytesCapacity;
    u64 labelRegionBytesCapacity;
    u64 smartPtrRegionBytesCapacity;
    u64 userRegionBytesCapacity;
    struct memory_allocator *freeList;
    struct memory_allocator *tailFree;
    u32 eventQueueCapacity;
    u32 eventQueueReadIndex;
    u32 eventQueueWriteIndex;
    u64 userSectionByteOffsets[MEMORY_MAX_USER_REGION_SECTIONS];
    u8 *heap;
};

static memory_byte_id
_memory_generate_unique_byte_id(void *seed, u64 seedSize)
{
    // TODO: generate using a seeded hash
    static i8 counter = -1;

    return ++counter;
}

static memory_short_id
_memory_generate_unique_short_id(void *seed, u64 seedSize)
{
    // TODO: generate using a seeded hash
    static i16 counter = -1;

    return ++counter;
}

static memory_int_id
_memory_generate_unique_int_id(void *seed, u64 seedSize)
{
    // TODO: generate using a seeded hash
    static i32 counter = -1;

    return ++counter;
}

static memory_id
_memory_generate_unique_id(void *seed, u64 seedSize)
{
    // TODO: generate using a seeded hash
    static i64 counter = -1;

    return ++counter;
}
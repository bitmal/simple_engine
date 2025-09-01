#include "circular_buffer.h"
#include "memory.h"

struct circular_buffer
{
    const struct memory_allocation_key bufferKey;
    u64 bufferByteCapacity;
    u64 bufferByteWriteIndex;
    u64 bufferByteReadIndex;
};

b32
circular_buffer_create(const struct memory_page_key *pageKeyPtr, u64 initByteSize,
    const struct memory_allocation_key *outAllocKey)
{
    if ((MEMORY_IS_PAGE_NULL(pageKeyPtr)))
    {
        if (outAllocKey)
        {
            memory_get_null_allocation_key(outAllocKey);
        }

        return B32_FALSE;
    }

    if (!outAllocKey)
    {
        return B32_FALSE;
    }

    const struct memory_allocation_key circularBufferKey;
    struct circular_buffer *circularBufferPtr;
    {
        memory_error_code resultCode = memory_alloc(pageKeyPtr, sizeof(struct circular_buffer), 
        NULL, &circularBufferKey);

        if (resultCode != MEMORY_OK)
        {
            memory_get_null_allocation_key(outAllocKey);

            return B32_FALSE;
        }

        resultCode = memory_map_alloc(&circularBufferKey, (void **)&circularBufferPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_free(&circularBufferKey);
            
            memory_get_null_allocation_key(outAllocKey);

            return B32_FALSE;
        }

        resultCode = memory_alloc(pageKeyPtr, initByteSize, NULL, 
        &circularBufferPtr->bufferKey);

        if (resultCode != MEMORY_OK)
        {
            memory_free(&circularBufferKey);
            
            memory_get_null_allocation_key(outAllocKey);

            return B32_FALSE;
        }
    }

    circularBufferPtr->bufferByteCapacity = initByteSize;
    circularBufferPtr->bufferByteReadIndex = circularBufferPtr->bufferByteWriteIndex = 0;

    memory_unmap_alloc((void **)&circularBufferPtr);

    memcpy((void *)outAllocKey, &circularBufferKey, sizeof(struct memory_allocation_key));

    return B32_TRUE;
}

b32
circular_buffer_get_byte_capacity(const struct memory_allocation_key *bufKeyPtr, u64 *outByteCapacity)
{
    if ((MEMORY_IS_ALLOCATION_NULL(bufKeyPtr)))
    {
        if (outByteCapacity)
        {
            *outByteCapacity = 0;
        }

        return B32_FALSE;
    }

    if (!outByteCapacity)
    {
        return B32_FALSE;
    }

    struct circular_buffer *bufPtr;
    {
        memory_error_code resultCode = memory_map_alloc(bufKeyPtr, (void **)&bufPtr);

        if (resultCode != MEMORY_OK)
        {
            *outByteCapacity = 0;

            return B32_FALSE;
        }
    }

    *outByteCapacity = bufPtr->bufferByteCapacity - 1;

    memory_unmap_alloc((void **)&bufPtr);

    return B32_TRUE;
}

b32
circular_buffer_get_read_index(const struct memory_allocation_key *bufKeyPtr, u64 *outByteIndex)
{
    if ((MEMORY_IS_ALLOCATION_NULL(bufKeyPtr)))
    {
        if (outByteIndex)
        {
            *outByteIndex = 0;
        }

        return B32_FALSE;
    }

    if (!outByteIndex)
    {
        return B32_FALSE;
    }

    struct circular_buffer *bufPtr;
    {
        memory_error_code resultCode = memory_map_alloc(bufKeyPtr, (void **)&bufPtr);

        if (resultCode != MEMORY_OK)
        {
            *outByteIndex = 0;

            return B32_FALSE;
        }
    }

    *outByteIndex = bufPtr->bufferByteReadIndex;

    memory_unmap_alloc((void **)&bufPtr);

    return B32_TRUE;
}

b32
circular_buffer_get_write_index(const struct memory_allocation_key *bufKeyPtr, u64 *outByteIndex)
{
    if ((MEMORY_IS_ALLOCATION_NULL(bufKeyPtr)))
    {
        if (outByteIndex)
        {
            *outByteIndex = 0;
        }

        return B32_FALSE;
    }

    if (!outByteIndex)
    {
        return B32_FALSE;
    }

    struct circular_buffer *bufPtr;
    {
        memory_error_code resultCode = memory_map_alloc(bufKeyPtr, (void **)&bufPtr);

        if (resultCode != MEMORY_OK)
        {
            *outByteIndex = 0;

            return B32_FALSE;
        }
    }

    *outByteIndex = bufPtr->bufferByteWriteIndex;

    memory_unmap_alloc((void **)&bufPtr);

    return B32_TRUE;
}

b32
circular_buffer_is_full(const struct memory_allocation_key *bufKeyPtr, b32 *outIsFull)
{
    if ((MEMORY_IS_ALLOCATION_NULL(bufKeyPtr)))
    {
        if (outIsFull)
        {
            *outIsFull = B32_FALSE;
        }

        return B32_FALSE;
    }

    if (!outIsFull)
    {
        return B32_FALSE;
    }

    struct circular_buffer *bufPtr;
    {
        memory_error_code resultCode = memory_map_alloc(bufKeyPtr, (void **)&bufPtr);

        if (resultCode != MEMORY_OK)
        {
            *outIsFull = B32_FALSE;

            return B32_FALSE;
        }
    }

    *outIsFull = (bufPtr->bufferByteWriteIndex + 1)%bufPtr->bufferByteCapacity == bufPtr->bufferByteReadIndex;

    memory_unmap_alloc((void **)&bufPtr);

    return B32_TRUE;
}

b32
circular_buffer_is_empty(const struct memory_allocation_key *bufKeyPtr, b32 *outIsEmpty)
{
    if ((MEMORY_IS_ALLOCATION_NULL(bufKeyPtr)))
    {
        if (outIsEmpty)
        {
            *outIsEmpty = B32_FALSE;
        }

        return B32_FALSE;
    }

    if (!outIsEmpty)
    {
        return B32_FALSE;
    }

    struct circular_buffer *bufPtr;
    {
        memory_error_code resultCode = memory_map_alloc(bufKeyPtr, (void **)&bufPtr);

        if (resultCode != MEMORY_OK)
        {
            *outIsEmpty = B32_FALSE;

            return B32_FALSE;
        }
    }

    *outIsEmpty = bufPtr->bufferByteReadIndex == bufPtr->bufferByteWriteIndex;

    memory_unmap_alloc((void **)&bufPtr);

    return B32_TRUE;
}

b32
circular_buffer_get_bytes_free(const struct memory_allocation_key *bufKeyPtr, u64 *outBytesFree)
{
    if ((MEMORY_IS_ALLOCATION_NULL(bufKeyPtr)))
    {
        if (outBytesFree)
        {
            *outBytesFree = 0;
        }

        return B32_FALSE;
    }

    if (!outBytesFree)
    {
        return B32_FALSE;
    }

    struct circular_buffer *bufPtr;
    {
        memory_error_code resultCode = memory_map_alloc(bufKeyPtr, (void **)&bufPtr);

        if (resultCode != MEMORY_OK)
        {
            *outBytesFree = 0;

            return B32_FALSE;
        }
    }
    
    u64 bytesFree;

    if (bufPtr->bufferByteWriteIndex != bufPtr->bufferByteReadIndex)
    {
        if (bufPtr->bufferByteWriteIndex > bufPtr->bufferByteReadIndex)
        {
            bytesFree = bufPtr->bufferByteReadIndex - bufPtr->bufferByteWriteIndex - 1;
        }
        else 
        {
            bytesFree = (bufPtr->bufferByteCapacity - 1 - bufPtr->bufferByteWriteIndex) + bufPtr->bufferByteReadIndex - 1;
        }
    }
    else 
    {
        bytesFree = bufPtr->bufferByteCapacity - 1;
    }

    *outBytesFree = bytesFree;

    memory_unmap_alloc((void **)&bufPtr);

    return B32_TRUE;
}

b32
circular_buffer_write_bytes(const struct memory_allocation_key *bufKeyPtr, u64 writeByteSize,
    u8 *bytesPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(bufKeyPtr)))
    {
        return B32_FALSE;
    }

    if (!bytesPtr)
    {
        return B32_FALSE;
    }

    // nothing to write, therefore already completed operation correctly!
    if (writeByteSize < 1)
    {
        return B32_TRUE;
    }

    struct circular_buffer *bufPtr;
    u8 *rhsBytesPtr;
    {
        memory_error_code resultCode = memory_map_alloc(bufKeyPtr, (void **)&bufPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        resultCode = memory_map_alloc(&bufPtr->bufferKey, (void **)&rhsBytesPtr);
    }

    u64 bytesFree;

    if (bufPtr->bufferByteWriteIndex != bufPtr->bufferByteReadIndex)
    {
        if (bufPtr->bufferByteWriteIndex > bufPtr->bufferByteReadIndex)
        {
            bytesFree = bufPtr->bufferByteReadIndex - bufPtr->bufferByteWriteIndex - 1;
        }
        else 
        {
            bytesFree = (bufPtr->bufferByteCapacity - 1 - bufPtr->bufferByteWriteIndex) + bufPtr->bufferByteReadIndex - 1;
        }
    }
    else 
    {
        bytesFree = bufPtr->bufferByteCapacity - 1;
    }

    if (writeByteSize > bytesFree)
    {
        memory_unmap_alloc((void **)&rhsBytesPtr);
        memory_unmap_alloc((void **)&bufPtr);

        return B32_FALSE;
    }

    u64 remainderBytes = (bufPtr->bufferByteWriteIndex + writeByteSize)%bufPtr->bufferByteCapacity + 1;
    u64 endByteIndex = remainderBytes - 1;

    if (remainderBytes > 0)
    {
        memcpy(&rhsBytesPtr[bufPtr->bufferByteWriteIndex], bytesPtr, writeByteSize - remainderBytes);
        memcpy(&rhsBytesPtr[0], &bytesPtr[writeByteSize - remainderBytes - 1], remainderBytes);
    }
    else 
    {
        memcpy(&rhsBytesPtr[bufPtr->bufferByteWriteIndex], bytesPtr, writeByteSize);
    }

    bufPtr->bufferByteWriteIndex = (bufPtr->bufferByteWriteIndex + writeByteSize)%bufPtr->bufferByteCapacity;

    memory_unmap_alloc((void **)&rhsBytesPtr);
    memory_unmap_alloc((void **)&bufPtr);

    return B32_TRUE;
}

b32
circular_buffer_read_bytes(const struct memory_allocation_key *bufKeyPtr, u64 readByteSize,
    u8 *outBytesPtr, u64 *outBytesReadPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(bufKeyPtr)))
    {
        if (outBytesReadPtr)
        {
            *outBytesReadPtr = 0;
        }

        return B32_FALSE;
    }

    if (!outBytesPtr)
    {
        if (outBytesReadPtr)
        {
            *outBytesReadPtr = 0;
        }

        return B32_FALSE;
    }

    if (!outBytesReadPtr)
    {
        return B32_FALSE;
    }

    // nothing to read, therefore already completed operation correctly!
    if (readByteSize < 1)
    {
        *outBytesReadPtr = 0;

        return B32_TRUE;
    }

    struct circular_buffer *bufPtr;
    u8 *rhsBytesPtr;
    {
        memory_error_code resultCode = memory_map_alloc(bufKeyPtr, (void **)&bufPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        resultCode = memory_map_alloc(&bufPtr->bufferKey, (void **)&rhsBytesPtr);
    }
    
    u64 readBytesAvailable;

    if (bufPtr->bufferByteWriteIndex != bufPtr->bufferByteReadIndex)
    {
        if (bufPtr->bufferByteWriteIndex > bufPtr->bufferByteReadIndex)
        {
            readBytesAvailable = bufPtr->bufferByteWriteIndex - bufPtr->bufferByteReadIndex - 1;
        }
        else 
        {
            readBytesAvailable = (bufPtr->bufferByteCapacity - 1 - bufPtr->bufferByteReadIndex) + bufPtr->bufferByteWriteIndex - 1;
        }
    }
    else 
    {
        readBytesAvailable = bufPtr->bufferByteCapacity - 1;
    }

    u64 readByteCount;
    
    if (readByteSize <= readBytesAvailable)
    {
        readByteCount = readByteSize;
    }
    else 
    {
        readByteCount = readBytesAvailable;
    }

    u64 remainderBytes = (bufPtr->bufferByteReadIndex + readByteCount)%bufPtr->bufferByteCapacity + 1;
    u64 endByteIndex = remainderBytes - 1;

    if (remainderBytes > 0)
    {
        memcpy(&outBytesPtr[0], &rhsBytesPtr[bufPtr->bufferByteReadIndex], readByteCount - remainderBytes);
        memcpy(&outBytesPtr[readByteCount - remainderBytes - 1], &rhsBytesPtr[0], readByteCount - remainderBytes);
    }
    else 
    {
        memcpy(&outBytesPtr[0], &rhsBytesPtr[bufPtr->bufferByteReadIndex], readByteCount);
    }

    bufPtr->bufferByteReadIndex = (bufPtr->bufferByteReadIndex + readByteCount)%bufPtr->bufferByteCapacity;

    memory_unmap_alloc((void **)&rhsBytesPtr);
    memory_unmap_alloc((void **)&bufPtr);

    *outBytesReadPtr = readByteCount;

    return B32_TRUE;
}
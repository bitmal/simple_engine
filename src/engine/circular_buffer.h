#ifndef __CIRCULAR_BUFFER_H
#define __CIRCULAR_BUFFER_H

#include "types.h"
#include "memory.h"
#include "utils.h"

b32
circular_buffer_create(const struct memory_page_key *pageKeyPtr, u64 initByteSize,
    const struct memory_allocation_key *outAllocKey);

b32
circular_buffer_get_byte_capacity(const struct memory_allocation_key *bufKeyPtr, u64 *outByteCapacity);

b32
circular_buffer_get_read_index(const struct memory_allocation_key *bufKeyPtr, u64 *outByteIndex);

b32
circular_buffer_get_write_index(const struct memory_allocation_key *bufKeyPtr, u64 *outByteIndex);

b32
circular_buffer_is_full(const struct memory_allocation_key *bufKeyPtr, b32 *outIsFull);

b32
circular_buffer_is_empty(const struct memory_allocation_key *bufKeyPtr, b32 *outIsEmpty);

b32
circular_buffer_get_bytes_free(const struct memory_allocation_key *bufKeyPtr, u64 *outBytesFree);

b32
circular_buffer_write_bytes(const struct memory_allocation_key *bufKeyPtr, u64 writeByteSize,
    u8 *bytesPtr);

b32
circular_buffer_read_bytes(const struct memory_allocation_key *bufKeyPtr, u64 readByteSize,
    u8 *outBytesPtr, u64 *outBytesReadPtr);

b32
circular_buffer_reset(const struct memory_allocation_key *bufKeyPtr);

b32
circular_buffer_shrink(const struct memory_allocation_key *bufKeyPtr, u64 byteSize);

b32
circular_buffer_grow(const struct memory_allocation_key *bufKeyPtr, u64 byteSize);

#endif
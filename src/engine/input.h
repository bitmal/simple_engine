#ifndef __INPUT_H
#define __INPUT_H

#include "types.h"
#include "memory.h"

typedef void (*input_key_callback)(const char *key, b32 isDown,
    struct memory_allocation_key *userKeyPtr);

b32
input_init(const struct memory_context_key *memoryContextKeyPtr, 
    const struct memory_allocation_key *userKeyPtr, const struct memory_allocation_key *outInputKeyPtr);

b32
input_add_keys(const struct memory_allocation_key *inputContextKeyPtr, 
    const char *keys[], i32 keyCount);

b32
input_bind_key(const struct memory_allocation_key *inputKeyPtr, 
    const char *key, input_key_callback cb);

b32
input_get_key_down(const struct memory_allocation_key *inputKeyPtr, const char *key, 
    b32 *outIsDown);

#endif
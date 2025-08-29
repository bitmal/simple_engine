#ifndef __INPUT_H
#define __INPUT_H

#include "types.h"
#include "memory.h"

typedef void(*input_key_callback)(const char *key, const char *action, void *dataPtr);

b32
input_init(const struct memory_context_key *memoryContextKeyPtr, 
    const struct memory_allocation_key *userKeyPtr, const struct memory_allocation_key *outInputKeyPtr);

b32
input_add_keys(const struct memory_allocation_key *inputContextKeyPtr, 
    const char *keys[], i32 keyCount);

void
input_bind_key(struct input *inputContext, struct memory *mem, const char *key, input_key_callback cb);

const struct input_key *
input_get_key(struct input *inputContext, const char *key);

const struct input_keybind *
input_get_keybind(struct input *inputContext, const char *key);

b32
input_set_key_down(struct input *inputContext, const char *key, b32 isKeyDown);

b32
input_get_key_down(struct input *inputContext, const char *key);

#endif
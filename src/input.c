#include "input.h"
#include "basic_dict.h"

#include <stdlib.h>
#include <string.h>

struct input *
input_init(struct memory *mem, void *dataPtr)
{
    struct input *inputContext = memory_alloc(mem, sizeof(struct input));
    
    inputContext->dataPtr = dataPtr;
    inputContext->keys = NULL;
    inputContext->keyCount = 0;
    inputContext->keyDict = DICTIONARY(mem, NULL);
    inputContext->keybinds = NULL;
    inputContext->keybindCount = 0;
    inputContext->keybindDict = DICTIONARY(mem, NULL);

    return inputContext;
}

void
input_add_keys(struct input *inputContext, struct memory *mem, const char *keys[], i32 keyCount)
{
    if (inputContext->keyCount > 0)
    {
        inputContext->keys = memory_realloc(mem, inputContext->keys, sizeof(struct input_key)*(inputContext->keyCount += keyCount));
    }
    else
    {
        inputContext->keys = memory_alloc(mem, sizeof(struct input_key)*(inputContext->keyCount = keyCount));
    }

    for (i32 key = inputContext->keyCount - keyCount; key < inputContext->keyCount; ++key)
    {
        const size_t keySize = strlen(keys[key]) + 1;

        inputContext->keys[key].isDown = B32_FALSE;
        inputContext->keys[key].key = memory_alloc(mem, keySize);
        sprintf((char *)inputContext->keys[key].key, "%s", keys[key]);

        basic_dict_set(inputContext->keyDict, mem, inputContext->keys, keys[key], keySize, &inputContext->keys[key]);
    }
}

void
input_bind_key(struct input *inputContext, struct memory *mem, const char *key, input_key_callback cb)
{
    const i32 index = inputContext->keybindCount;

    if (inputContext->keybinds)
    {
        inputContext->keybinds = memory_realloc(mem, inputContext->keybinds, sizeof(struct input_keybind)*(++inputContext->keybindCount));
    }
    else
    {
        inputContext->keybinds = memory_alloc(mem, sizeof(struct input_keybind)*(++inputContext->keybindCount));
    }

    inputContext->keybinds[index].callback = cb;
    inputContext->keybinds[index].key = key;

    basic_dict_set(inputContext->keybindDict, mem, inputContext->keybinds, key, strlen(key) + 1, cb);
}

b32
input_key_down(struct input *inputContext, const char *key)
{
    struct input_key *keyPtr = basic_dict_get(inputContext->keyDict, inputContext->keys, key);

    if (keyPtr)
    {
        return keyPtr->isDown;
    }

    return B32_FALSE;
}
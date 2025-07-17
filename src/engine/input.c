#include "input.h"
#include "basic_dict.h"
#include "memory.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>

struct input *
input_init(struct memory *mem, void *dataPtr)
{
    struct input *inputContext = memory_alloc(mem, sizeof(struct input));
    
    inputContext->dataPtr = dataPtr;
    inputContext->keys = NULL;
    inputContext->keyCount = 0;
    inputContext->keyDict = DICTIONARY(mem, NULL, NULL);
    inputContext->keybinds = NULL;
    inputContext->keybindCount = 0;
    inputContext->keybindDict = DICTIONARY(mem, NULL, NULL);

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

        basic_dict_set(inputContext->keyDict, mem, inputContext->keys, (char *)keys[key], keySize, &inputContext->keys[key]);
    }
}

void
input_bind_key(struct input *inputContext, struct memory *mem, const char *key, input_key_callback cb)
{
    const i32 index = inputContext->keybindCount;

    if (inputContext->keybindCount > 0)
    {
        inputContext->keybinds = memory_realloc(mem, inputContext->keybinds, sizeof(struct input_keybind)*(++inputContext->keybindCount));
    }
    else
    {
        inputContext->keybinds = memory_alloc(mem, sizeof(struct input_keybind)*(++inputContext->keybindCount));
    }

    inputContext->keybinds[index].callback = cb;
    inputContext->keybinds[index].key = memory_alloc(mem, strlen(key) + 1);

    sprintf(UTILS_MUTABLE_CAST(char *, inputContext->keybinds[index].key), "%s", key);

    basic_dict_set(inputContext->keybindDict, mem, inputContext->keybinds, (char *)key, strlen(key) + 1, &cb);
}

b32
input_set_key_down(struct input *inputContext, const char *key, b32 isKeyDown)
{
    struct input_key *keyPtr = basic_dict_get(inputContext->keyDict, inputContext->keys, (char *)key);

    if (keyPtr)
    {
        keyPtr->isDown = isKeyDown;

        return B32_TRUE;
    }
    
    return B32_FALSE;
}

b32
input_get_key_down(struct input *inputContext, const char *key)
{
    struct input_key *keyPtr = basic_dict_get(inputContext->keyDict, inputContext->keys, (char *)key);

    if (keyPtr)
    {
        return keyPtr->isDown;
    }

    return B32_FALSE;
}
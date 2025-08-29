#include "input.h"
#include "basic_dict.h"
#include "memory.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>

struct input_key
{
    const struct memory_raw_allocation_key rawKeyKey;
    b32 isDown;
};

struct input_keybind
{
    const struct memory_raw_allocation_key rawKeyKey;
    input_key_callback callback;
};

struct input
{
    const struct memory_context_key memoryKey;
    const struct memory_page_key pageKey;
    const struct memory_allocation_key keyArr;
    i32 keyCount;
    i32 keyCapacity;
    const struct memory_allocation_key keyDict;
    const struct memory_allocation_key keybindArr;
    i32 keybindCount;
    i32 keybindCapacity;
    const struct memory_allocation_key keybindDict;
    const struct memory_allocation_key userKey;
};

b32
input_init(const struct memory_context_key *memoryContextKeyPtr, 
    const struct memory_allocation_key *userKeyPtr, const struct memory_allocation_key *outInputKeyPtr)
{
    const struct memory_page_key pageKey;
    {
        memory_error_code resultCode = memory_alloc_page(memoryContextKeyPtr, 
        sizeof(struct input) + 8192, &pageKey);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    struct memory_allocation_key inputKey;
    struct input *inputPtr;
    {
        memory_error_code resultCode = memory_alloc(&pageKey, sizeof(struct input), NULL,
            &inputKey);

        if (resultCode != MEMORY_OK)
        {
            memory_free_page(&pageKey);

            return B32_FALSE;
        }

        resultCode = memory_map_alloc(&inputKey, (void **)&inputPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_free(&inputKey);
            memory_free_page(&pageKey);

            return B32_FALSE;
        }
    }

    memory_set_alloc_offset_width(&inputKey, 0, sizeof(struct input), '\0');

    memcpy((void *)&inputPtr->userKey, userKeyPtr, sizeof(struct memory_allocation_key));
    
    if (!(basic_dict_create(&pageKey, NULL, NULL, 
        utils_generate_next_prime_number(100), NULL, 
        userKeyPtr, &inputPtr->keybindDict)))
    {
        memory_unmap_alloc((void **)&inputPtr);
        memory_free_page(&pageKey);

        return B32_FALSE;
    }
    
    if (!(basic_dict_create(&pageKey, NULL, NULL, 
        utils_generate_next_prime_number(100), NULL, 
        userKeyPtr, &inputPtr->keyDict)))
    {
        memory_unmap_alloc((void **)&inputPtr);
        memory_free_page(&pageKey);

        return B32_FALSE;
    }

    memcpy((void *)&inputPtr->pageKey, &pageKey, sizeof(struct memory_page_key));
    memcpy((void *)&inputPtr->memoryKey, memoryContextKeyPtr, 
    sizeof(struct memory_context_key));

    memory_unmap_alloc((void **)&inputPtr);

    memcpy((void *)outInputKeyPtr, &inputKey, sizeof(struct memory_allocation_key));

    return B32_TRUE;
}

b32
input_add_keys(const struct memory_allocation_key *inputContextKeyPtr, 
    const char *keys[], i32 keyCount)
{
    if ((MEMORY_IS_ALLOCATION_NULL(inputContextKeyPtr)))
    {
        return B32_FALSE;
    }

    struct input *inputPtr;
    {
        memory_error_code resultCode = memory_map_alloc(inputContextKeyPtr, 
        (void **)&inputPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    i32 diff = inputPtr->keyCapacity - inputPtr->keyCount;

#define KEY_ALLOC_MULTIPLIER 2

    if (diff < keyCount)
    {
        if (inputPtr->keyCapacity > 0)
        {
            const struct memory_allocation_key resultKey;

            memory_error_code resultCode = memory_realloc(&inputPtr->keyArr, &inputPtr->pageKey, 
                sizeof(struct input_key)*(inputPtr->keyCapacity = keyCount*
                KEY_ALLOC_MULTIPLIER), NULL, &inputPtr->keyArr);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&inputPtr);

                return B32_FALSE;
            }
        }
        else 
        {
            memory_error_code resultCode = memory_alloc(&inputPtr->pageKey, 
                sizeof(struct input_key)*(inputPtr->keyCapacity = keyCount*
                KEY_ALLOC_MULTIPLIER), NULL, &inputPtr->keyArr);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&inputPtr);

                return B32_FALSE;
            }
        }
    }

    for (i32 i = 0; i < keyCount; ++i)
    {
        if ((!basic_dict_get_is_found(&inputPtr->keyDict, (void *)keys[i])))
        {
        }
    }

    memory_unmap_alloc((void **)&inputPtr);

    return B32_TRUE;
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
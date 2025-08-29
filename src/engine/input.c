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

// TODO: main handles this
static b32
_input_set_key_down(const struct memory_allocation_key *inputKeyPtr, const char *key, b32 isKeyDown)
{
    if ((MEMORY_IS_ALLOCATION_NULL(inputKeyPtr)))
    {
        return B32_FALSE;
    }

    if (!key)
    {
        return B32_FALSE;
    }
    
    struct input *inputPtr;
    {
        memory_error_code resultCode = memory_map_alloc(inputKeyPtr, (void **)&inputPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    struct input_key *keyPtr;

    if (!(basic_dict_map_data(&inputPtr->keyDict, (void *)key, (void **)&keyPtr)))
    {
        memory_unmap_alloc((void **)&inputPtr);

        return B32_FALSE;
    }

    keyPtr->isDown = isKeyDown;

    basic_dict_unmap_data(&inputPtr->keyDict, (void *)key, (void **)&keyPtr);
    memory_unmap_alloc((void **)&inputPtr);

    return B32_TRUE;
}

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

    i32 startKeyIndex = inputPtr->keyCount;
    i32 diff = inputPtr->keyCapacity - inputPtr->keyCount;

#define KEY_ALLOC_MULTIPLIER 2

    if (diff < keyCount)
    {
        if (inputPtr->keyCapacity > 0)
        {
            const struct memory_allocation_key resultKey;

            memory_error_code resultCode = memory_realloc(&inputPtr->keyArr,
                sizeof(struct input_key)*(inputPtr->keyCapacity = keyCount*
                KEY_ALLOC_MULTIPLIER), &resultKey);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&inputPtr);

                return B32_FALSE;
            }

            memcpy((void *)&inputPtr->keyArr, &resultKey, sizeof(struct memory_allocation_key));
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

    i32 negateCount = 0;

    for (i32 i = 0; i < keyCount; ++i)
    {
        if ((!basic_dict_get_is_found(&inputPtr->keyDict, (void *)keys[i])))
        {
            struct input_key *keyArrPtr;
            {
                memory_error_code resultCode = memory_map_alloc(&inputPtr->keyArr, 
                (void **)&keyArrPtr);

                if (resultCode != MEMORY_OK)
                {
                    memory_unmap_alloc((void **)&inputPtr);

                    return B32_FALSE;
                }
            }

            i32 keyIndex = startKeyIndex + i - negateCount;

            keyArrPtr[keyIndex].isDown = B32_FALSE;

            p64 dataByteOffset = keyIndex*sizeof(struct input_key);
            u64 dataByteSize = sizeof(struct input_key);

            i32 strLength = strlen(keys[i]);

            memory_error_code resultCode = memory_raw_alloc(&keyArrPtr[keyIndex].rawKeyKey, 
            strLength + 1);

            if (resultCode != MEMORY_OK)
            {
                ++negateCount;

                continue;
            }

            char *keyPtr;

            resultCode = memory_map_raw_allocation(&keyArrPtr[keyIndex].rawKeyKey, 
            (void **)&keyPtr);

            if (resultCode != MEMORY_OK)
            {
                memory_raw_free(&keyArrPtr[keyIndex].rawKeyKey);

                ++negateCount;

                continue;
            }

            strncpy(keyPtr, keys[i], strLength);

            memory_unmap_raw_allocation(&keyArrPtr[keyIndex].rawKeyKey, (void **)&keyPtr);
            
            memory_unmap_alloc((void **)&keyArrPtr);

            basic_dict_push_data(&inputPtr->keyDict, (void *)keys[i], 
            &dataByteOffset, &dataByteSize, 
            &inputPtr->keyArr);

            ++inputPtr->keyCount;
        }
        else 
        {
            ++negateCount;
        }
    }

    memory_unmap_alloc((void **)&inputPtr);

    return B32_TRUE;
}

b32
input_bind_key(const struct memory_allocation_key *inputKeyPtr, const char *key, 
    input_key_callback cb)
{
    if ((MEMORY_IS_ALLOCATION_NULL(inputKeyPtr)))
    {
        return B32_FALSE;
    }

    if (!key)
    {
        return B32_FALSE;
    }

    if (!cb)
    {
        return B32_FALSE;
    }

    struct input *inputPtr;
    {
        memory_error_code resultCode = memory_map_alloc(inputKeyPtr, 
        (void **)&inputPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    if ((basic_dict_get_is_found(&inputPtr->keybindDict, (void *)key)))
    {
        return B32_FALSE;
    }

    i32 keybindIndex = inputPtr->keybindCount;

    if (inputPtr->keybindCount == inputPtr->keybindCapacity)
    {
        #define INPUT_KEYBIND_REALLOC_MULTIPLIER 2

        if (inputPtr->keybindCapacity > 0)
        {
            const struct memory_allocation_key tempKey;

            memory_error_code resultCode = memory_alloc(&inputPtr->pageKey, sizeof(struct input_keybind)
                *(inputPtr->keybindCapacity = INPUT_KEYBIND_REALLOC_MULTIPLIER), 
                NULL, &tempKey);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&inputPtr);

                return B32_FALSE;
            }

            memcpy((void *)&inputPtr->keybindArr, &tempKey, sizeof(struct memory_allocation_key));
        }
        else 
        {
            memory_error_code resultCode = memory_alloc(&inputPtr->pageKey, sizeof(struct input_keybind)
                *(inputPtr->keybindCapacity = INPUT_KEYBIND_REALLOC_MULTIPLIER), 
                NULL, &inputPtr->keybindArr);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&inputPtr);

                return B32_FALSE;
            }
        }
    }

    struct input_keybind *keybindArrPtr;
    {
        memory_error_code resultCode = memory_map_alloc(&inputPtr->keybindArr, 
        (void **)&keybindArrPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&inputPtr);

            return B32_FALSE;
        }
    }

    {
        size_t keyLength = strlen(key);

        memory_error_code resultCode = memory_raw_alloc(&keybindArrPtr[keybindIndex].rawKeyKey, 
        keyLength + 1);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&keybindArrPtr);
            memory_unmap_alloc((void **)&inputPtr);

            return B32_FALSE;
        }

        char *rawMappedKeyPtr;

        resultCode = memory_map_raw_allocation(&keybindArrPtr[keybindIndex].rawKeyKey, 
        (void **)&rawMappedKeyPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_raw_free(&keybindArrPtr[keybindIndex].rawKeyKey);
            memory_unmap_alloc((void **)&keybindArrPtr);
            memory_unmap_alloc((void **)&inputPtr);

            return B32_FALSE;
        }

        strncpy(rawMappedKeyPtr, key, keyLength);

        memory_unmap_raw_allocation(&keybindArrPtr[keybindIndex].rawKeyKey, 
        (void **)&rawMappedKeyPtr);
    }

    keybindArrPtr[keybindIndex].callback = cb;

    ++inputPtr->keybindCount;

    p64 dataByteOffset = sizeof(struct input_keybind)*keybindIndex;
    u64 dataByteSize = sizeof(struct input_keybind);
    
    memory_unmap_alloc((void **)&keybindArrPtr);

    basic_dict_push_data(&inputPtr->keybindDict, (void *)key, &dataByteOffset, 
    &dataByteSize, &inputPtr->keybindArr);

    memory_unmap_alloc((void **)&inputPtr);

    return B32_TRUE;
}

b32
input_get_key_down(const struct memory_allocation_key *inputKeyPtr, const char *key, 
    b32 *outIsDown)
{
    if ((MEMORY_IS_ALLOCATION_NULL(inputKeyPtr)))
    {
        return B32_FALSE;
    }

    if (!key)
    {
        return B32_FALSE;
    }

    if (!outIsDown)
    {
        return B32_FALSE;
    }

    struct input *inputPtr;
    {
        memory_error_code resultCode = memory_map_alloc(inputKeyPtr, (void **)&inputPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    struct input_key *keyPtr;

    if (!(basic_dict_map_data(&inputPtr->keyDict, (void *)key, (void **)&keyPtr)))
    {
        memory_unmap_alloc((void **)&inputPtr);

        return B32_FALSE;
    }

    *outIsDown = keyPtr->isDown;

    basic_dict_unmap_data(&inputPtr->keyDict, (void *)key, (void **)&keyPtr);
    memory_unmap_alloc((void **)&inputPtr);

    return B32_TRUE;
}
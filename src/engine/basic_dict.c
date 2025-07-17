#include "basic_dict.h"
#include "utils.h"
#include "memory.h"

#include <stdlib.h>
#include <assert.h>

static u64 
_default_hash_func(struct basic_dict *dict, void *key)
{
    return utils_str_hash_func(key);
}

struct basic_dict *
basic_dict_create(struct memory *memory, basic_dict_hash_func hashFunc, i32 initBuckets, void *userPtr)
{
    struct basic_dict *dict = memory_alloc(memory, sizeof(struct basic_dict));
    dict->buckets = initBuckets;
    dict->__table = memory_alloc(memory, sizeof(struct basic_dict_pair *)*initBuckets);
    memset(dict->__table, 0, sizeof(struct basic_dict_pair *)*initBuckets);
    dict->__hashFunc = hashFunc ? hashFunc : &_default_hash_func;
    dict->userPtr = userPtr;

    return dict;
}

void *
basic_dict_get(struct basic_dict *dict, void *database, void *key)
{
    const u64 hash = dict->__hashFunc(dict, key);
    struct basic_dict_pair *pair = dict->__table[hash%dict->buckets];

    while (pair)
    {
        if (pair->hash == hash)
        {
            if (pair->dataPtr != BASIC_DICT_NULL_REL_PTR)
            {
                return (void *)((basic_dict_rel_ptr)database + pair->dataPtr);
            }
            else
            {
                return NULL;
            }
        }

        pair = pair->next;
    }

    return NULL;
}

b32
basic_dict_set(struct basic_dict *dict, struct memory *mem, void *database, void *key, size_t keySize, void *data)
{
    const u64 hash = dict->__hashFunc(dict, key);
    const basic_dict_id index = hash%dict->buckets;

    b32 result = B32_FALSE;

    if (dict->__table[index])
    {
        struct basic_dict_pair *pair = dict->__table[index];
        b32 isFound = B32_FALSE;

        while (pair && pair->next)
        {
            if (pair->hash == hash)
            {
                isFound = B32_TRUE;
                break;
            }

            pair = pair->next;
        }

        if (isFound)
        {
            if (data)
            {
                pair->dataPtr = (basic_dict_rel_ptr)data - (basic_dict_rel_ptr)database;
            }
            else
            {
                pair->dataPtr = BASIC_DICT_NULL_REL_PTR;
            }
        }
        else
        {
            pair->next = memory_alloc(mem, sizeof(struct basic_dict_pair));
            pair->next->key = memory_alloc(mem, keySize);
            memcpy((void *)pair->next->key, key, keySize);
            pair->next->hash = hash;
            pair->next->next = NULL;
            
            if (data)
            {
                pair->next->dataPtr = (basic_dict_rel_ptr)data - (basic_dict_rel_ptr)database;
            }
            else
            {
                pair->next->dataPtr = BASIC_DICT_NULL_REL_PTR;
            }
        }

        result = B32_TRUE;
    }
    else
    {
        dict->__table[index] = memory_alloc(mem, sizeof(struct basic_dict_pair));
        dict->__table[index]->key = memory_alloc(mem, keySize);
        memcpy((void *)dict->__table[index]->key, key, keySize);
        dict->__table[index]->hash = hash;
        dict->__table[index]->next = NULL;

        if (data)
        {
            dict->__table[index]->dataPtr = (basic_dict_rel_ptr)data - (basic_dict_rel_ptr)database;
        }
        else
        {
            dict->__table[index]->dataPtr = BASIC_DICT_NULL_REL_PTR;
        }

        result = B32_TRUE;
    }

    return result;
}

void
basic_dict_clear(struct basic_dict *dict)
{
    memset(dict->__table, 0, sizeof(struct basic_dict_pair *)*dict->buckets);
}

b32
basic_dict_get_is_found(struct basic_dict *dict, void *key)
{
    const u64 hash = dict->__hashFunc(dict, key);


    return B32_FALSE;
}
#ifndef __BASIC_DICT_H
#define __BASIC_DICT_H

#include "types.h"

#include <stdio.h>

typedef i32 basic_dict_id;
typedef u64 basic_dict_rel_ptr;

struct basic_dict;
typedef u64 (*basic_dict_hash_func)(struct basic_dict *dict, void *key);
typedef b32 (*basic_dict_data_is_found_equal_func)(struct basic_dict *dict, void *key);

struct memory;

#define BASIC_DICT_NULL_REL_PTR ((basic_dict_rel_ptr)0 - (basic_dict_rel_ptr)1)
#define BASIC_DICT_MAX_ADDRESS (BASIC_DICT_NULL_REL_PTR - 1)

#define BASIC_DICT_DEFAULT_BUCKET_COUNT 101

struct basic_dict_pair
{
    const void *key;
    u64 hash;
    basic_dict_rel_ptr dataPtr;
    struct basic_dict_pair *next;
};

struct basic_dict
{
    i32 buckets;
    struct basic_dict_pair **__table;
    basic_dict_hash_func __hashFunc;
    void *userPtr;
};

struct basic_dict *
basic_dict_create(struct memory *memory, basic_dict_hash_func hashFunc, i32 initBuckets, void *userPtr);
#define DICTIONARY(memoryContext, hashFunc, userPtr) basic_dict_create(memoryContext, hashFunc, BASIC_DICT_DEFAULT_BUCKET_COUNT, userPtr)

void *
basic_dict_get(struct basic_dict *dict, void *database, void *key);

b32
basic_dict_set(struct basic_dict *dict, struct memory *mem, void *database, void *key, size_t keySize, void *data);

void
basic_dict_clear(struct basic_dict *dict);

b32
basic_dict_get_is_found(struct basic_dict *dict, void *key);

#endif
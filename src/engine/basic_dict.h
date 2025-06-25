#ifndef __BASIC_DICT_H
#define __BASIC_DICT_H

#include "types.h"

#include <stdio.h>

typedef i32 basic_dict_id;
typedef u64 basic_dict_rel_ptr;

typedef u64 (*basic_dict_hash_func)(const struct basic_dict *dict, const void *key);

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
};

struct basic_dict *
basic_dict_create(struct memory *memory, basic_dict_hash_func hashFunc, i32 initBuckets);
#define DICTIONARY(memoryContext, hashFunc) basic_dict_create(memoryContext, hashFunc, BASIC_DICT_DEFAULT_BUCKET_COUNT)

void *
basic_dict_get(struct basic_dict *dict, void *database, const void *key);

b32
basic_dict_set(struct basic_dict *dict, struct memory *mem, void *database, const void *key, size_t keySize, void *data);

void
basic_dict_clear(struct basic_dict *dict);

#endif
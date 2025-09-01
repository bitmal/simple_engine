#ifndef __BASIC_DICT_H
#define __BASIC_DICT_H

#include "memory.h"
#include "types.h"
#include "utils.h"

#include <stdio.h>

typedef u32 basic_dict_id;

struct basic_dict;

typedef b32 (*basic_dict_hash_func)(struct basic_dict *dictPtr, void *key, 
    utils_hash *outHashPtr);

typedef b32 (*basic_dict_create_key_copy_func)(struct basic_dict *dictPtr, void *keyPtr, 
    const struct memory_raw_allocation_key *outRawKeyKeyPtr);

#define BASIC_DICT_NULL_PTR ((p64)0)

b32
basic_dict_create(const struct memory_page_key *memoryPageKeyPtr, basic_dict_hash_func hashFunc,
    basic_dict_create_key_copy_func keyCopyFunc, u32 initBucketCount, u64 *keySize, 
    const struct memory_allocation_key *userPtrKeyPtr, const struct memory_allocation_key *outDictKeyPtr);

b32
basic_dict_map_data(const struct memory_allocation_key *dictKeyPtr, void *keyPtr, 
    void **outDataPtr);

b32
basic_dict_unmap_data(const struct memory_allocation_key *dictKeyPtr, void *keyPtr,
    void **outDataPtr);

b32
basic_dict_push_data(const struct memory_allocation_key *dictKeyPtr,
    void *keyPtr, p64 *dataByteOffset, u64 *dataByteSize,
    const struct memory_allocation_key *dataKeyPtr);

b32
basic_dict_get_data(const struct memory_allocation_key *dictKeyPtr,
    void *keyPtr, p64 *dataByteOffset, u64 *dataByteSize,
    const struct memory_allocation_key *outDataKeyPtr);

b32
basic_dict_remove(const struct memory_allocation_key *dictKeyPtr, void *keyPtr);

b32
basic_dict_clear(const struct memory_allocation_key *dictKeyPtr);

b32
basic_dict_reset(const struct memory_allocation_key *dictKeyPtr, const struct memory_allocation_key *databaseKeyPtr);

b32
basic_dict_get_is_found(const struct memory_allocation_key *dictKeyPtr, void *keyPtr);

b32
basic_dict_destroy(const struct memory_allocation_key *dictKeyPtr);

#endif
#ifndef _CONFIG_H
#define _CONFIG_H

#include "memory.h"
#include "types.h"

#include <stdint.h>

struct basic_dict;
struct memory_context_key;

#define CONFIG_VAR_REAL32_SIZE (sizeof(real32))
#define CONFIG_VAR_REAL64_SIZE (sizeof(real64))
#define CONFIG_VAR_I32_SIZE (sizeof(i32))
#define CONFIG_VAR_U32_SIZE (sizeof(u32))
#define CONFIG_VAR_I64_SIZE (sizeof(i64))
#define CONFIG_VAR_U64_SIZE (sizeof(u64))
#define CONFIG_VAR_BOOL_SIZE (sizeof(b32))
#define CONFIG_VAR_CHAR_SIZE (sizeof(char))
#define CONFIG_VAR_PTR32_SIZE (sizeof(p32))
#define CONFIG_VAR_PTR64_SIZE (sizeof(p64))
#define CONFIG_VAR_PTR_SIZE (CONFIG_VAR_PTR64_SIZE)

enum config_var_type
{
    CONFIG_VAR_I32,
    CONFIG_VAR_U32,
    CONFIG_VAR_I64,
    CONFIG_VAR_U64,
    CONFIG_VAR_REAL32,
    CONFIG_VAR_REAL64,
    CONFIG_VAR_BOOL,
    CONFIG_VAR_STRING,
    CONFIG_VAR_STRUCTURE,
    CONFIG_VAR_PTR32,
    CONFIG_VAR_PTR
};

b32
config_init(const struct memory_context_key *memoryKeyPtr, u64 varMemoryByteSize, 
    const struct memory_allocation_key *outAllocationKeyPtr);

b32
config_load(const struct memory_allocation_key *configKeyPtr, const char *fileName);

b32
config_save(const struct memory_allocation_key *configKeyPtr, const char *fileName);

b32
config_set_var(const struct memory_allocation_key *configKeyPtr, const char *name, 
    enum config_var_type type, u32 arrLength, void *valueArr);

b32
config_map_var(const struct memory_allocation_key *configKeyPtr, const char *name, void **outMapPtr);

b32
config_unmap_var(const struct memory_allocation_key *configKeyPtr, void **outMapPtr);

#endif
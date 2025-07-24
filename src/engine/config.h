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

struct config_var_header
{
    const char *name;
    enum config_var_type type;
    u32 arrLength;
    u64 memoryOffset;
    u64 memoryAllocated;
    u64 memoryCapacity;
};

struct config
{
    const struct memory_context_key *memKeyPtr;
    const struct memory_page_key configContextPage;
    const struct memory_page_key configVarPage;
    i32 varCount;
    const struct memory_allocation_key *varArrKeyPtr;
    const struct memory_allocation_key *varDictKeyPtr;
    u64 arenaOffset;
    u64 arenaSize;
};

b32
config_init(const struct memory_context_key *memKeyPtr, u64 varMemoryByteSize, const struct memory_allocation_key *outAllocationKeyPtr);

b32
config_load_config(const struct memory_allocation_key *configKeyPtr, const char *fileName);

b32
config_save_var_config(const struct config *context, const char *fileName, const char *varName);

b32
config_set_var(const struct config *context, const char *name, enum config_var_type type, 
    i32 arrLength, const void *valueArr);

const struct config_var_header *
config_get_var_header(const struct config *context, const char *name);

const void *
config_get_var(const struct config *context, const char *name, i32 arrIndex);

#endif
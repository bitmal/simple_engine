#ifndef _CONFIG_H
#define _CONFIG_H

#include "types.h"

#include <stdint.h>

struct basic_dict;
struct memory;

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
    void *_var;
    i32 _varCount;
    struct basic_dict *_varMap;
    struct memory *_mem;
    u64 _arenaOffset;
    u64 _arenaSize;
};

struct config *
config_init(struct memory *mem);

b32
config_load_config(struct config *context, const char *fileName);

b32
config_save_var_config(struct config *context, const char *fileName, const char *varName);

b32
config_set_var(struct config *context, const char *name, enum config_var_type type, 
    i32 arrLength, const void *valueArr);

const struct config_var_header *
config_get_var_header(struct config *context, const char *name);

const void *
config_get_var(struct config *context, const char *name, i32 arrIndex);

#endif
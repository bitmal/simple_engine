#ifndef _CONFIG_H
#define _CONFIG_H

#include "types.h"

struct basic_dict;
struct memory;

#define CONFIG_VAR_NUMBER_SIZE (sizeof(real32))
#define CONFIG_VAR_BOOLEAN_SIZE (sizeof(b32))

enum config_var_type
{
    CONFIG_VAR_NUMBER,
    CONFIG_VAR_BOOLEAN,
    CONFIG_VAR_STRING,
    CONFIG_VAR_STRUCTURE
};

struct config_var_header
{
    const char *name;
    enum config_var_type type;
    u32 size;
    i32 valueCount;
    u32 _padding;
};

struct config
{
    void *_var;
    i32 _varCount;
    struct basic_dict *_varMap;
    struct memory *_mem;
    u32 _arenaOffset;
    u32 _arenaSize;
};

struct config *
config_init(struct memory *mem);

b32
config_load_config(struct config *context, const char *fileName);

void
config_save_var_config(struct config *context, const char *fileName);

b32
config_set_var_number(struct config *context, const char *name, real32 value);

b32
config_set_var_string(struct config *context, const char *name, const char *value);

b32
config_set_var_boolean(struct config *context, const char *name, b32 value);

b32
config_get_var_number(struct config *context, const char *name, real32 *out);

b32
config_get_var_string(struct config *context, const char *name, const char **out);

b32
config_get_var_boolean(struct config *context, const char *name, b32 *out);

#endif
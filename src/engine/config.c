#include "config.h"
#include "memory.h"
#include "basic_dict.h"
#include "types.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

i32
_config_get_type_size(enum config_var_type type)
{
    i32 typeSize;
    switch (type)
    {
        case CONFIG_VAR_REAL32:
        {
            typeSize = CONFIG_VAR_REAL32_SIZE;
        } break;
        
        case CONFIG_VAR_REAL64:
        {
            typeSize = CONFIG_VAR_REAL64_SIZE;
        } break;
        
        case CONFIG_VAR_I32:
        {
            typeSize = CONFIG_VAR_I32_SIZE;
        } break;
        
        case CONFIG_VAR_U32:
        {
            typeSize = CONFIG_VAR_U32_SIZE;
        } break;
        
        case CONFIG_VAR_BOOL:
        {
            typeSize = CONFIG_VAR_BOOL_SIZE;
        } break;
        
        case CONFIG_VAR_PTR:
        {
            typeSize = CONFIG_VAR_PTR_SIZE;
        } break;
        
        case CONFIG_VAR_PTR32:
        {
            typeSize = CONFIG_VAR_PTR32_SIZE;
        } break;

        default: 
        {
            typeSize = 0;
        } break;
    }

    return typeSize;
}

struct config *
config_init(struct memory *mem)
{
    assert(mem);

    struct config *context = memory_alloc(mem, sizeof(struct config));
    context->_arenaOffset = 0;
    context->_arenaSize = 0;
    context->_varCount = 0;
    context->_mem = mem;
    context->_varMap = DICTIONARY(mem, NULL);

    return context;
}

b32
config_load_config(struct config *context, const char *fileName)
{
    const u32 nameLength = strlen(fileName);
    char *filePath = memory_alloc(context->_mem, (sizeof("resources/configs/") - 1) + nameLength + sizeof(".conf"));
    strcpy(filePath, "resources/configs/");
    strcat(filePath, fileName);
    strcat(filePath, ".conf");

    FILE *confFile = fopen(filePath, "r");
    if (!confFile)
    {
        fprintf(stderr, "config_load_config error: Could not load config file \"%s\".\n", fileName);

        return B32_FALSE;
    }
    
    memory_free(context->_mem, filePath);

    const char *delims = ":=\r\n";

    char lineBuffer[256];
    while(fgets(lineBuffer, sizeof(lineBuffer), confFile))
    {
        char *token = strtok(lineBuffer, delims);

        char *name = NULL;
        i32 nameLength;
        char *type = NULL;
        i32 typeLength;
        char *value = NULL;
        i32 valueLength;

        while (token)
        {
            char *start = token;
            i32 length = 0;

            while (!isgraph(start[0]) && start[0] != '\0')
            {
                ++start;
            }

            if (!name)
            {
                if (isalpha(start[0]))
                {
                    while (isalnum(start[length]) || start[length] == '_')
                    {
                        ++length;
                    }

                    name = start;
                    nameLength = length;
                    name[nameLength] = '\0';
                }
                else
                {
                    fprintf(stderr, "config_load_config error: Invalid var name \"%s\" in \"%s\" config. Must start with an alphabetical.\n", 
                        start, fileName);
                    break;
                }
            }
            else if (!type)
            {
                while (isalnum(start[length]))
                {
                    ++length;
                }

                if (length > 0)
                {
                    type = start;
                    typeLength = length;
                    type[typeLength] = '\0';
                }
                else
                {
                    fprintf(stderr, "config_load_config error: Invalid type \"%s\" in \"%s\" config. Must contain only alphabeticals.\n", 
                        start, fileName);
                    break;
                }
            }
            else if (!value)
            {
                b32 isString = start[0] == '\"';

                if (isString)
                {
                    while(isgraph(start[length]) || isspace(start[length]))
                    {
                        ++length;

                        if (start[length] == '\"' && isString)
                        {
                            ++length;
                            break;
                        }
                    }
                }
                else
                {
                    while(isgraph(start[length]))
                    {
                        ++length;
                    }
                }

                if (length > 0)
                {
                    value = start;
                    valueLength = length;
                    value[valueLength] = '\0';
                }
                else
                {
                    fprintf(stderr, "config_load_config error: Invalid value for \"%s\" in \"%s\" config. Must be of appropriate type.\n", 
                        name, fileName);
                    break;
                }
            }
            else
            {
                break;
            }
            
            token = strtok(NULL, delims);
        }

        if (name && type && value)
        {
            if (!strcmp(type, "real32"))
            {
                real32 num;

                if (sscanf(value, "%f", &num) > 0)
                {
                    config_set_var(context, name, CONFIG_VAR_REAL32, 1, &num);
                }
                else
                {
                    fprintf(stderr, "config_load_config error: Invalid number value \"%s\" for var \"%s\" in \"%s\".\n", 
                        value, name, fileName);
                }
            }
            else if (!strcmp(type, "i32"))
            {
                i32 num;

                if (sscanf(value, "%d", &num) > 0)
                {
                    config_set_var(context, name, CONFIG_VAR_I32, 1, &num);
                }
                else
                {
                    fprintf(stderr, "config_load_config error: Invalid number value \"%s\" for var \"%s\" in \"%s\".\n", 
                        value, name, fileName);
                }
            }
            else if (!strcmp(type, "i64"))
            {
                i64 num;

                if (sscanf(value, "%ld", &num) > 0)
                {
                    config_set_var(context, name, CONFIG_VAR_I64, 1, &num);
                }
                else
                {
                    fprintf(stderr, "config_load_config error: Invalid number value \"%s\" for var \"%s\" in \"%s\".\n", 
                        value, name, fileName);
                }
            }
            else if (!strcmp(type, "u64"))
            {
                u64 num;

                if (sscanf(value, "%lu", &num) > 0)
                {
                    config_set_var(context, name, CONFIG_VAR_U64, 1, &num);
                }
                else
                {
                    fprintf(stderr, "config_load_config error: Invalid number value \"%s\" for var \"%s\" in \"%s\".\n", 
                        value, name, fileName);
                }
            }
            else if (!strcmp(type, "real64"))
            {
                real64 num;

                if (sscanf(value, "%lf", &num) > 0)
                {
                    config_set_var(context, name, CONFIG_VAR_REAL64, 1, &num);
                }
                else
                {
                    fprintf(stderr, "config_load_config error: Invalid number value \"%s\" for var \"%s\" in \"%s\".\n", 
                        value, name, fileName);
                }
            }
            else if (!strcmp(type, "real"))
            {
                real32 num;

                if (sscanf(value, "%f", &num) > 0)
                {
                    config_set_var(context, name, CONFIG_VAR_REAL32, 1, &num);
                }
                else
                {
                    fprintf(stderr, "config_load_config error: Invalid number value \"%s\" for var \"%s\" in \"%s\".\n", 
                        value, name, fileName);
                }
            }
            else if (!strcmp(type, "string"))
            {
                if (!strcmp(value, "NULL"))
                {
                    char value1[] = "\0";
                    config_set_var(context, name, CONFIG_VAR_STRING, 1, value1);
                }
                else if (value[0] == '\"' && value[valueLength - 1] == '\"')
                {
                    value[valueLength - 1] = '\0';
                    config_set_var(context, name, CONFIG_VAR_STRING, 1, value + 1);
                }
                else
                {
                    fprintf(stderr, "config_load_config error: Invalid string value \"%s\" for var \"%s\" in \"%s\". Must be \"NULL\" or in double quotations.\n", 
                        value, name, fileName);
                }
            }
            else if (!strcmp(type, "boolean"))
            {
                if (!strcmp(value, "false"))
                {
                    b32 value = B32_FALSE;
                    config_set_var(context, name, CONFIG_VAR_BOOL, 1, &value);
                }
                else if (!strcmp(value, "true"))
                {
                    b32 value = B32_TRUE;
                    config_set_var(context, name, CONFIG_VAR_BOOL, 1, &value);
                }
                else
                {
                    fprintf(stderr, "config_load_config error: Invalid boolean value \"%s\" for var \"%s\" in \"%s\". Must be \"true\" or \"false\".\n", value, name, fileName);
                }
            }
            else
            {
                fprintf(stderr, "config_load_config error: Invalid type \"%s\" for var \"%s\" in \"%s\".\n", type, name, fileName);
            }
        }
    }

    fclose(confFile);

    return B32_TRUE;
}

b32
config_save_var_config(struct config *context, const char *fileName, const char *varName)
{
    // TODO
    return B32_FALSE;
}

b32
config_set_var(struct config *context, const char *name, enum config_var_type type, 
    i32 arrLength, const void *valueArr)
{
    assert(arrLength > 0);

    for (const char *c = name; ; ++c)
    {
        if (c == name && !isalpha(*c))
        {
            if (*c != '\0')
            {
                fprintf(stderr, "config_set_var_number error: Invalid var name \"%s\". Must start with alphabetical.\n", name);
            }
            else
            {
                fprintf(stderr, "config_set_var_number error: Invalid var name. Empty string.\n");
            }
            
            return B32_FALSE;
        }
        else if (*c == '\0')
        {
            break;
        }
        else if (!isalnum(*c) && *c != '_')
        {
            fprintf(stderr, "config_set_var_number error: Invalid var name \"%s\". Must contain only alphanumerics, or '_'.\n", name);
            return B32_FALSE;
        }
    }
    
    struct config_var_header *varPtr = basic_dict_get(context->_varMap, context->_var, name);
    
    i32 typeSize = _config_get_type_size(type);

    if (!varPtr)
    {
        u32 diff = context->_arenaSize - context->_arenaOffset;

        u32 size = sizeof(struct config_var_header) + typeSize*arrLength;

        if (diff < size)
        {
            if (context->_arenaSize > 0)
            {
                context->_var = memory_realloc(context->_mem, context->_var, (context->_arenaSize += size));
            }
            else
            {
                context->_var = memory_alloc(context->_mem, (context->_arenaSize += size));
            }
        }

        varPtr = (struct config_var_header *)((u8 *)context->_var + context->_arenaOffset);
        u32 nameLength = strlen(name) + 1;
        varPtr->name = memory_alloc(context->_mem, nameLength);
        memcpy(UTILS_MUTABLE_CAST(char *, varPtr->name), name, nameLength);
        varPtr->memoryCapacity = typeSize*arrLength + sizeof(struct config_var_header);
        varPtr->memoryOffset = context->_arenaOffset;
        varPtr->arrLength = arrLength;

        context->_arenaOffset += size;
        ++context->_varCount;
    }
    else
    {
        if (varPtr->memoryCapacity < (typeSize*arrLength + sizeof(struct config_var_header)))
        {
            context->_var = memory_realloc(context->_mem, context->_var, (context->_arenaSize += typeSize*arrLength + sizeof(struct config_var_header)));

            memcpy((u8 *)context->_var + context->_arenaOffset, varPtr, sizeof(struct config_var_header));

            varPtr = (struct config_var_header *)((u8 *)context->_var + context->_arenaOffset);
            varPtr->memoryOffset = context->_arenaOffset;
            varPtr->memoryCapacity = sizeof(struct config_var_header) + typeSize*arrLength;

            context->_arenaOffset = context->_arenaSize;
            context->_arenaOffset = context->_arenaSize;
        }

        varPtr->arrLength = arrLength;
    }

    varPtr->type = type;

    if (valueArr)
    {
        memcpy((u8 *)context->_var + varPtr->memoryOffset + sizeof(struct config_var_header), valueArr, typeSize*arrLength);
    }

    basic_dict_set(context->_varMap, context->_mem, context->_var, name, strlen(name) + 1, varPtr);

    return B32_TRUE;
}

const struct config_var_header *
config_get_var_header(struct config *context, const char *name)
{
    return basic_dict_get(context->_varMap, context->_var, name);
}

const void *
config_get_var(struct config *context, const char *name, i32 arrIndex)
{
    struct config_var_header *varPtr = basic_dict_get(context->_varMap, context->_var, name);

    if (varPtr)
    {
        if (arrIndex >= varPtr->arrLength)
        {
            fprintf(stderr, "config_get_var('%d'): Error for var ('%s'), arrIndex out of range('%d').", 
                __LINE__, name, arrIndex);

            return NULL;
        }

        i32 typeSize = _config_get_type_size(varPtr->type);

        return &((u8 *)context->_var + varPtr->memoryOffset + sizeof(struct config_var_header))[typeSize*arrIndex];
    }

    fprintf(stderr, "config_get_var('%d'): Error for var ('%s'), var was not found.", 
        __LINE__, name);

    return NULL;
}
#include "config.h"
#include "memory.h"
#include "basic_dict.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

struct config *
config_init(struct memory *mem)
{
    assert(mem);

    struct config *context = memory_alloc(mem, sizeof(struct config));
    context->_mem = mem;
    context->_var = NULL;
    context->_varCount = 0;
    context->_varMap = DICTIONARY(mem, NULL);
    context->_arenaOffset = 0;
    context->_arenaSize = 0;

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
                while (isalpha(start[length]))
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
            if (!strcmp(type, "number"))
            {
                real32 num;

                if (sscanf(value, "%f", &num) > 0)
                {
                    config_set_var_number(context, name, num);
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
                    config_set_var_string(context, name, NULL);
                }
                else if (value[0] == '\"' && value[valueLength - 1] == '\"')
                {
                    value[valueLength - 1] = '\0';
                    config_set_var_string(context, name, value + 1);
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
                    config_set_var_boolean(context, name, B32_FALSE);
                }
                else if (!strcmp(value, "true"))
                {
                    config_set_var_boolean(context, name, B32_TRUE);
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

void
config_save_var_config(struct config *context, const char *fileName);

b32
config_set_var_number(struct config *context, const char *name, real32 value)
{
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

    if (!varPtr)
    {
        u32 diff = context->_arenaSize - context->_arenaOffset;
        u32 size = sizeof(struct config_var_header) + sizeof(value);

        if (diff < size)
        {
            if (context->_arenaSize > 0)
            {
                context->_var = memory_realloc(context->_mem, context->_var, context->_arenaSize += size);
            }
            else
            {
                context->_var = memory_alloc(context->_mem, context->_arenaSize += size);
            }
        }

        varPtr = (struct config_var_header *)((u8 *)context->_var + context->_arenaOffset);
        u32 nameLength = strlen(name) + 1;
        varPtr->name = memory_alloc(context->_mem, nameLength);
        memcpy(UTILS_MUTABLE_CAST(char *, varPtr->name), name, nameLength);
        varPtr->size = CONFIG_VAR_NUMBER_SIZE;
        varPtr->type = CONFIG_VAR_NUMBER;
        varPtr->valueCount = 1;
        
        basic_dict_set(context->_varMap, context->_mem, context->_var, name, nameLength, varPtr);

        context->_arenaOffset += size;
        ++context->_varCount;
    }
    else if (varPtr->type != CONFIG_VAR_NUMBER)
    {
        fprintf(stderr, "config_set_var_number error: Cannot set value \"%s\". Stored type is not a number.\n", name);

        return B32_FALSE;
    }

    *(real32 *)(varPtr + 1) = value;
    
    return B32_TRUE;
}

b32
config_set_var_string(struct config *context, const char *name, const char *value)
{
    for (const char *c = name; ; ++c)
    {
        if (c == name && !isalpha(*c))
        {
            if (*c != '\0')
            {
                fprintf(stderr, "config_set_var_string error: Invalid var name \"%s\". Must start with alphabetical.\n", name);
            }
            else
            {
                fprintf(stderr, "config_set_var_string error: Invalid var name. Empty string.\n");
            }
            
            return B32_FALSE;
        }
        else if (*c == '\0')
        {
            break;
        }
        else if (!isalnum(*c) && *c != '_')
        {
            fprintf(stderr, "config_set_var_string error: Invalid var name \"%s\". Must contain only alphanumerics, or '_'.\n", name);
            return B32_FALSE;
        }
    }

    struct config_var_header *varPtr = basic_dict_get(context->_varMap, context->_var, name);

    if (!varPtr)
    {
        u32 diff = context->_arenaSize - context->_arenaOffset;
        u32 size = sizeof(struct config_var_header) + sizeof(value);

        if (diff < size)
        {
            if (context->_arenaSize > 0)
            {
                context->_var = memory_realloc(context->_mem, context->_var, context->_arenaSize += size);
            }
            else
            {
                context->_var = memory_alloc(context->_mem, context->_arenaSize += size);
            }
        }

        varPtr = (struct config_var_header *)((u8 *)context->_var + context->_arenaOffset);
        u32 nameLength = strlen(name) + 1;
        varPtr->name = memory_alloc(context->_mem, nameLength);
        memcpy(UTILS_MUTABLE_CAST(char *, varPtr->name), name, nameLength);
        varPtr->type = CONFIG_VAR_STRING;
        varPtr->valueCount = 1;

        if (value)
        {
            u32 valueLength = strlen(value) + 1;
            varPtr->size = valueLength;
            *(char **)(varPtr + 1) = memory_alloc(context->_mem, valueLength);
            memcpy(*(char **)(varPtr + 1), value, valueLength);
        }
        else
        {
            varPtr->size = 0;
            *(char **)(varPtr + 1) = NULL;
        }

        basic_dict_set(context->_varMap, context->_mem, context->_var, name, nameLength, varPtr);

        context->_arenaOffset += size;
        ++context->_varCount;
    }
    else if (varPtr->type != CONFIG_VAR_STRING)
    {
        fprintf(stderr, "config_set_var_string error: Cannot set value \"%s\". Stored type is not a string.\n", name);

        return B32_FALSE;
    }
    else if (value)
    {
        u32 valueLength = strlen(value) + 1;

        if (valueLength > varPtr->size)
        {
            if (varPtr->size > 0)
            {
                *(char **)(varPtr + 1) = memory_realloc(context->_mem, *(char **)(varPtr + 1), valueLength);
            }
            else
            {
                *(char **)(varPtr + 1) = memory_alloc(context->_mem, valueLength);
            }
            
            varPtr->size = valueLength;
        }
        
        memcpy(*(char **)(varPtr + 1), value, valueLength);
    }
    else if (*(char **)(varPtr + 1))
    {
        memory_free(context->_mem, *(char **)(varPtr + 1));
        *(char **)(varPtr + 1) = NULL;
        varPtr->size = 0;
    }

    return B32_TRUE;
}

b32
config_set_var_boolean(struct config *context, const char *name, b32 value)
{
    for (const char *c = name; ; ++c)
    {
        if (c == name && !isalpha(*c))
        {
            if (*c != '\0')
            {
                fprintf(stderr, "config_set_var_boolean error: Invalid var name \"%s\". Must start with alphabetical.\n", name);
            }
            else
            {
                fprintf(stderr, "config_set_var_boolean error: Invalid var name. Empty string.\n");
            }
            
            return B32_FALSE;
        }
        else if (*c == '\0')
        {
            break;
        }
        else if (!isalnum(*c) && *c != '_')
        {
            fprintf(stderr, "config_set_var_boolean error: Invalid var name \"%s\". Must contain only alphanumerics, or '_'.\n", name);
            return B32_FALSE;
        }
    }

    assert(value == B32_FALSE || value == B32_TRUE);

    struct config_var_header *varPtr = basic_dict_get(context->_varMap, context->_var, name);

    if (!varPtr)
    {
        u32 diff = context->_arenaSize - context->_arenaOffset;
        u32 size = sizeof(struct config_var_header) + sizeof(value);

        if (diff < size)
        {
            if (context->_arenaSize > 0)
            {
                context->_var = memory_realloc(context->_mem, context->_var, context->_arenaSize += size);
            }
            else
            {
                context->_var = memory_alloc(context->_mem, context->_arenaSize += size);
            }
        }

        varPtr = (struct config_var_header *)((u8 *)context->_var + context->_arenaOffset);
        u32 nameLength = strlen(name) + 1;
        varPtr->name = memory_alloc(context->_mem, nameLength);
        memcpy(UTILS_MUTABLE_CAST(char *, varPtr->name), name, nameLength);
        varPtr->size = sizeof(b32);
        varPtr->type = CONFIG_VAR_BOOLEAN;
        varPtr->valueCount = 1;
        
        basic_dict_set(context->_varMap, context->_mem, context->_var, name, nameLength, varPtr);

        context->_arenaOffset += size;
        ++context->_varCount;
    }
    else if (varPtr->type != CONFIG_VAR_BOOLEAN)
    {
        fprintf(stderr, "config_set_var_boolean error: Cannot set value \"%s\". Stored type is not a boolean.\n", name);

        return B32_FALSE;
    }

    *(b32 *)(varPtr + 1) = value;

    return B32_TRUE;
}

b32
config_get_var_number(struct config *context, const char *name, real32 *out)
{
    assert(out);

    struct config_var_header *header = basic_dict_get(context->_varMap, context->_var, name);
    
    if (!header)
    {
        fprintf(stderr, "config_get_var_number error: Cannot get value \"%s\". Does not exist.\n", name);

        return B32_FALSE;
    }
    else if (header->type != CONFIG_VAR_NUMBER)
    {
        fprintf(stderr, "config_get_var_number error: Cannot get value \"%s\". Stored type is not a number.\n", name);

        return B32_FALSE;
    }

    *out = *(real32 *)(header + 1);

    return B32_TRUE;
}

b32
config_get_var_string(struct config *context, const char *name, const char **out)
{
    assert(out);

    struct config_var_header *header = basic_dict_get(context->_varMap, context->_var, name);
    
    if (!header)
    {
        fprintf(stderr, "config_get_var_string error: Cannot get value \"%s\". Does not exist.\n", name);

        return B32_FALSE;
    }
    else if (header->type != CONFIG_VAR_STRING)
    {
        fprintf(stderr, "config_get_var_string error: Cannot get value \"%s\". Stored type is not a string.\n", name);

        return B32_FALSE;
    }

    *out = *(const char **)(header + 1);

    return B32_TRUE;
}

b32
config_get_var_boolean(struct config *context, const char *name, b32 *out)
{
    assert(out);

    struct config_var_header *header = basic_dict_get(context->_varMap, context->_var, name);
    
    if (!header)
    {
        fprintf(stderr, "config_get_var_boolean error: Cannot get value \"%s\". Does not exist.\n", name);

        return B32_FALSE;
    }
    else if (header->type != CONFIG_VAR_BOOLEAN)
    {
        fprintf(stderr, "config_get_var_string error: Cannot get value \"%s\". Stored type is not a boolean.\n", name);

        return B32_FALSE;
    }

    *out = *(b32 *)(header + 1);

    return B32_TRUE;
}
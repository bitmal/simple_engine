#include "config.h"
#include "memory.h"
#include "basic_dict.h"
#include "types.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

struct config_var_header
{
    const struct memory_raw_allocation_key rawNameKey;
    enum config_var_type type;
    u64 byteSize;
    u64 byteOffset;
    u64 byteCapacity;
    p64 prevHeaderByteOffset;
    p64 nextHeaderByteOffset;
};

struct config
{
    const struct memory_context_key memoryKey;
    const struct memory_page_key configContextPage;
    const struct memory_page_key configVarPage;
    const struct memory_allocation_key varArrKey;
    u32 freeVarCount;
    p64 freeVarByteOffsetList;
    u32 activeVarCount;
    p64 activeVarByteOffsetList;
    const struct memory_allocation_key varDictKey;
    u64 varWriteByteOffset;
};

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

b32
config_init(const struct memory_context_key *memKeyPtr, u64 varMemoryByteSize, 
    const struct memory_allocation_key *outAllocationKeyPtr)
{
    assert(memKeyPtr);
    assert(outAllocationKeyPtr);

    const struct memory_page_key configPageKey;
    const struct memory_page_key varPageKey;
    struct config *configPtr;
    {
        memory_error_code resultCode;

        if ((resultCode = memory_alloc_page(memKeyPtr, 
            sizeof(struct config) + 256, &configPageKey)) != MEMORY_OK)
        {
            fprintf(stderr, "config_init(%d): Failure to allocate config context page.\n", __LINE__);

            return B32_FALSE;
        }

        if ((resultCode = memory_alloc(&configPageKey, 
            sizeof(struct config), NULL, outAllocationKeyPtr)) != MEMORY_OK)
        {
            fprintf(stderr, "config_init(%d): Failure to allocate config context.\n", __LINE__);
            
            memory_free_page(&configPageKey);
            
            return B32_FALSE;
        }

        memset(configPtr, '\0', sizeof(struct config));

        if ((resultCode = memory_alloc_page(memKeyPtr, varMemoryByteSize, &varPageKey)) != MEMORY_OK)
        {
            fprintf(stderr, "config_init(%d): Failure to allocate var page.\n", __LINE__);
            
            memory_free(outAllocationKeyPtr);
            memory_free_page(&configPageKey);

            return B32_FALSE;
        }
        
        if ((resultCode = memory_map_alloc(outAllocationKeyPtr, (void **)&configPtr)) != MEMORY_OK)
        {
            memory_free(outAllocationKeyPtr);
            memory_free_page(&configPageKey);
            memory_free_page(&varPageKey);
            
            return B32_FALSE;
        }
    }

    memcpy((void *)&configPtr->memoryKey, memKeyPtr, sizeof(struct memory_context_key));

    basic_dict_create(&configPtr->configVarPage, NULL, NULL, 
        utils_generate_next_prime_number(100), NULL, NULL, 
        &configPtr->varDictKey);

    memory_unmap_alloc((void **)&configPtr);

    return B32_TRUE;
}

b32
config_load(const struct memory_allocation_key *configKeyPtr, const char *fileName)
{
    assert(configKeyPtr);
    assert(fileName);

    const u32 nameLength = strlen(fileName);

    char filePath[256];
    
    strcpy(filePath, "resources/configs/");
    strcat(filePath, fileName);
    strcat(filePath, ".conf");

    FILE *confFile = fopen(filePath, "r");
    if (!confFile)
    {
        fprintf(stderr, "config_load_config error: Could not load config file \"%s\".\n", fileName);

        return B32_FALSE;
    }
    
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
                    config_set_var(configKeyPtr, name, CONFIG_VAR_REAL32, 1, &num);
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
                    config_set_var(configKeyPtr, name, CONFIG_VAR_I32, 1, &num);
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
                    config_set_var(configKeyPtr, name, CONFIG_VAR_I64, 1, &num);
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
                    config_set_var(configKeyPtr, name, CONFIG_VAR_U64, 1, &num);
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
                    config_set_var(configKeyPtr, name, CONFIG_VAR_REAL64, 1, &num);
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
                    config_set_var(configKeyPtr, name, CONFIG_VAR_REAL32, 1, &num);
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
                    config_set_var(configKeyPtr, name, CONFIG_VAR_STRING, 1, value1);
                }
                else if (value[0] == '\"' && value[valueLength - 1] == '\"')
                {
                    value[valueLength - 1] = '\0';
                    config_set_var(configKeyPtr, name, CONFIG_VAR_STRING, 1, value + 1);
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
                    config_set_var(configKeyPtr, name, CONFIG_VAR_BOOL, 1, &value);
                }
                else if (!strcmp(value, "true"))
                {
                    b32 value = B32_TRUE;
                    config_set_var(configKeyPtr, name, CONFIG_VAR_BOOL, 1, &value);
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
config_save(const struct memory_allocation_key *configKeyPtr, const char *fileName)
{
    // TODO:

    return B32_TRUE;
}

b32
config_set_var(const struct memory_allocation_key *configKeyPtr, const char *name, 
    enum config_var_type type, u32 arrLength, void *valueArr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(configKeyPtr)))
    {
        return B32_FALSE;
    }

    assert(arrLength > 0);

    struct config *configPtr;
    {
        memory_error_code resultCode = memory_map_alloc(configKeyPtr, (void **)&configPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    i32 arrLengthClamped = (arrLength > 0) ? arrLength : 1;

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
    
    struct config_var_header *varArrPtr;
    u8 *varDataPtr;

    b32 isResult = basic_dict_map_data(&configPtr->varDictKey, 
    (void *)name, (void **)&varDataPtr);

    i32 typeSize = _config_get_type_size(type);

    if (!isResult)
    {
        u64 diff = memory_sizeof(&configPtr->varArrKey) - configPtr->varWriteByteOffset;

        u64 size = sizeof(struct config_var_header) + typeSize*arrLengthClamped;

        if (diff < size)
        {
            assert(size);

#define CONFIG_ARENA_REALLOC_MULTIPLIER 2

            if (configPtr->activeVarCount > 0)
            {
                const struct memory_allocation_key tempKey;

                memory_error_code resultCode = memory_realloc(&configPtr->varArrKey, 
                    memory_sizeof(&configPtr->varArrKey)*CONFIG_ARENA_REALLOC_MULTIPLIER + size, 
                    &tempKey);

                if (resultCode != MEMORY_OK)
                {
                    memory_unmap_alloc((void **)&configPtr);

                    return B32_FALSE;
                }

                memcpy((void *)&configPtr->varArrKey, &tempKey, sizeof(struct memory_allocation_key));
                
                resultCode = memory_map_alloc(&configPtr->varArrKey, (void **)&varArrPtr);

                if (resultCode != MEMORY_OK)
                {
                    memory_unmap_alloc((void **)&configPtr);

                    return B32_FALSE;
                }
            }
            else 
            {
                memory_error_code resultCode = memory_alloc(&configPtr->configVarPage, size*
                    CONFIG_ARENA_REALLOC_MULTIPLIER, NULL, &configPtr->varArrKey);

                if (resultCode != MEMORY_OK)
                {
                    memory_unmap_alloc((void **)&configPtr);

                    return B32_FALSE;
                }

                resultCode = memory_map_alloc(&configPtr->varArrKey, (void **)&varArrPtr);
                
                if (resultCode != MEMORY_OK)
                {
                    memory_unmap_alloc((void **)&configPtr);

                    return B32_FALSE;
                }
            }
        }
        else 
        {
            memory_error_code resultCode = memory_map_alloc(&configPtr->varArrKey, 
            (void **)&varArrPtr);
        }

        varDataPtr = (void *)((p64)varArrPtr + configPtr->varWriteByteOffset);

        u32 nameByteSize = strlen(name) + 1;

        char *rawNamePtr;
        {
            memory_error_code resultCode = memory_raw_alloc(&((struct config_var_header *)
                varDataPtr)->rawNameKey, nameByteSize);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&configPtr->varArrKey);
                memory_unmap_alloc((void **)&configPtr);

                return B32_FALSE;
            }

            resultCode = memory_map_raw_allocation(&((struct config_var_header *)varDataPtr)->rawNameKey,
            (void **)&rawNamePtr);

            if (resultCode != MEMORY_OK)
            {
                memory_raw_free(&((struct config_var_header *)varDataPtr)->rawNameKey);
                memory_unmap_alloc((void **)&varArrPtr);
                memory_unmap_alloc((void **)&configPtr);

                return B32_FALSE;
            }

            memcpy(rawNamePtr, name, nameByteSize);

            memory_unmap_raw_allocation(&((struct config_var_header *)varDataPtr)->rawNameKey, (void **)&rawNamePtr);
        }

        ((struct config_var_header *)varDataPtr)->byteOffset = configPtr->varWriteByteOffset;
        ((struct config_var_header *)varDataPtr)->byteCapacity = typeSize*arrLengthClamped;
        ((struct config_var_header *)varDataPtr)->byteSize = ((struct config_var_header *)varDataPtr)->byteCapacity;

        configPtr->varWriteByteOffset += size;
        ++configPtr->activeVarCount;
    }
    else if (((struct config_var_header *)varDataPtr)->byteCapacity < (typeSize*arrLengthClamped))
    {
        memory_error_code resultCode = memory_map_alloc(&configPtr->varArrKey, (void **)&varArrPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&configPtr);

            return B32_FALSE;
        }

        struct config_var_header *freeVarPtr = NULL;

        if (configPtr->freeVarCount > 0)
        {
            freeVarPtr  = (void *)((u8 *)varArrPtr + configPtr->freeVarByteOffsetList);

            do 
            {
                if (freeVarPtr->byteCapacity >= (typeSize*arrLengthClamped))
                {
                    break;
                }

                if (freeVarPtr->nextHeaderByteOffset != configPtr->freeVarByteOffsetList)
                {
                    freeVarPtr = (void *)((u8 *)varArrPtr + freeVarPtr->nextHeaderByteOffset);
                }
                else 
                {
                    freeVarPtr = NULL;
                }
            } while (freeVarPtr);
        }

        if (!freeVarPtr)
        {
            memory_unmap_alloc((void **)&varArrPtr);

            const struct memory_allocation_key tempKey;

            memory_error_code resultCode = memory_realloc(&configPtr->varArrKey, 
                (memory_sizeof(&configPtr->varArrKey)*CONFIG_ARENA_REALLOC_MULTIPLIER 
                + typeSize*arrLengthClamped + sizeof(struct config_var_header)),
                &tempKey);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&configPtr);

                return B32_FALSE;
            }

            resultCode = memory_map_alloc(&configPtr->varArrKey, (void **)&varArrPtr);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)configPtr);

                return B32_FALSE;
            }

            varDataPtr += configPtr->varWriteByteOffset;

            configPtr->varWriteByteOffset += typeSize*arrLengthClamped + sizeof(struct config_var_header);
        }
        else 
        {
            struct config_var_header *prevFreeHeaderPtr = (void *)((u8 *)varArrPtr + ((struct config_var_header *)
                freeVarPtr)->prevHeaderByteOffset);
            struct config_var_header *nextFreeHeaderPtr = (void *)((u8 *)varArrPtr + ((struct config_var_header *)
                freeVarPtr)->nextHeaderByteOffset);

            if (configPtr->freeVarCount > 1)
            {
                prevFreeHeaderPtr->nextHeaderByteOffset = ((struct config_var_header *)freeVarPtr)->nextHeaderByteOffset;
                nextFreeHeaderPtr->prevHeaderByteOffset = ((struct config_var_header *)freeVarPtr)->prevHeaderByteOffset;
            }
            else 

            if (configPtr->freeVarByteOffsetList == ((struct config_var_header *)freeVarPtr)->byteOffset)
            {
                if (configPtr->freeVarCount > 1)
                {
                    configPtr->freeVarByteOffsetList = nextFreeHeaderPtr->byteOffset;
                }
            }

            --configPtr->freeVarCount;

            if (configPtr->activeVarCount > 0)
            {
                struct config_var_header *headActiveHeaderPtr = (void *)((p64)varArrPtr + configPtr->activeVarByteOffsetList);
                struct config_var_header *tailActiveHeaderPtr = (void *)((p64)varArrPtr + headActiveHeaderPtr->prevHeaderByteOffset);

                headActiveHeaderPtr->prevHeaderByteOffset = freeVarPtr->byteOffset;
                tailActiveHeaderPtr->nextHeaderByteOffset = freeVarPtr->byteOffset;

                freeVarPtr->prevHeaderByteOffset = tailActiveHeaderPtr->byteOffset;
                freeVarPtr->nextHeaderByteOffset = headActiveHeaderPtr->byteOffset;
            }
            else 
            {
                freeVarPtr->prevHeaderByteOffset = freeVarPtr->nextHeaderByteOffset = freeVarPtr->byteOffset;
            }

            configPtr->activeVarByteOffsetList = freeVarPtr->byteOffset;
            ++configPtr->activeVarCount;

            varDataPtr = (u8 *)freeVarPtr;
        }
    }
    
    ((struct config_var_header *)varDataPtr)->type = type;
    ((struct config_var_header *)varDataPtr)->byteSize = typeSize*arrLengthClamped;

    memcpy(varDataPtr + sizeof(struct config_var_header), valueArr, 
        typeSize*arrLengthClamped);

    p64 dataOffset = ((struct config_var_header *)varDataPtr)->byteOffset;
    u64 dataSize = typeSize*arrLengthClamped + sizeof(struct config_var_header);

    basic_dict_push_data(&configPtr->varDictKey, (void *)name, 
    &dataOffset, &dataSize, &configPtr->varArrKey);

    if (!isResult)
    {
        basic_dict_unmap_data(&configPtr->varDictKey, (void *)name, (void **)&varDataPtr);
    }

    memory_unmap_alloc((void **)&varArrPtr);
    memory_unmap_alloc((void **)&configPtr);

    return B32_TRUE;
}

b32
config_map_var(const struct memory_allocation_key *configKeyPtr, const char *name, void **outMapPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(configKeyPtr)))
    {
        if (outMapPtr)
        {
            *outMapPtr = NULL;
        }

        return B32_FALSE;
    }

    if (!name)
    {
        if (outMapPtr)
        {
            *outMapPtr = NULL;
        }

        return B32_FALSE;
    }

    if (!outMapPtr)
    {
        return B32_FALSE;
    }

    struct config *configPtr;
    {
        memory_error_code resultCode = memory_map_alloc(configKeyPtr, (void **)&configPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }
    
    void *resultPtr;

    b32 isResult = basic_dict_map_data(&configPtr->varDictKey, (void *)name, &resultPtr);

    *outMapPtr = (void *)((p64)resultPtr + sizeof(struct config_var_header));

    memory_unmap_alloc((void **)&configPtr);

    return isResult;
}

b32
config_unmap_var(const struct memory_allocation_key *configKeyPtr, void **outMapPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(configKeyPtr)))
    {
        if (outMapPtr)
        {
            *outMapPtr = NULL;
        }

        return B32_FALSE;
    }
    
    if (!outMapPtr)
    {
        return B32_FALSE;
    }
    
    struct config *configPtr;
    {
        memory_error_code resultCode = memory_map_alloc(configKeyPtr, (void **)&configPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    struct config_var_header *varHeaderPtr = (void *)((p64)(*outMapPtr) - sizeof(struct config_var_header));

    char *namePtr;
    {
        memory_error_code resultCode = memory_map_raw_allocation(&varHeaderPtr->rawNameKey, (void **)&namePtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&configPtr);

            return B32_FALSE;
        }
    }

    void *resultPtr = (void *)((p64)(*outMapPtr) - sizeof(struct config_var_header));

    b32 isResult = basic_dict_unmap_data(&configPtr->varDictKey, (void *)namePtr, &resultPtr);

    *outMapPtr = resultPtr;

    memory_unmap_raw_allocation(&varHeaderPtr->rawNameKey, (void **)&namePtr);
    memory_unmap_alloc((void **)&configPtr);

    return B32_TRUE;
}
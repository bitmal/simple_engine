#include "renderer.h"
#include "vec4.h"
#include "vec2.h"
#include "basic_dict.h"
#include "memory.h"
#include "opengl.h"

#include "glew/glew.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define RENDERER_VERTEX_BUFFER_SIZE 1024*1024
#define RENDERER_INDEX_BUFFER_SIZE 1024*1024
#define RENDERER_MATERIAL_PROPERTY_BUFFER_SIZE 1024*1024

#define RENDERER_TEXTURE_BYTES_COMPRESSION_FLAG (u8)(0x1 << 0)

// layer >> program >> mesh >> texture >> models

static u64
__renderer_batch_hash_func(const struct basic_dict *dict, const void *key)
{
    // TODO: limit these stored resource ids to 2 bytes per (currently i32s)
    const struct renderer_batch_key *keyCast = key;

    return ((u64)keyCast->layer << 48) |
        ((u64)keyCast->mesh << 32) |
        ((u64)keyCast->program << 16) |
        ((u64)keyCast->texture);
}

static i32 
__renderer_layer_sort_compare_func(struct memory *mem, void *lhs, size_t lhsIndex, void *rhs, size_t rhsIndex, void *userPtr)
{
    struct renderer_layer *lhsLayer = lhs;
    struct renderer_layer *rhsLayer = rhs;

    if (lhsLayer->layer > rhsLayer->layer)
    {
        return 1;
    }
    else if (lhsLayer->layer < rhsLayer->layer)
    {
        return -1;
    }

    return 0;
}

static void
__renderer_layer_sort_replace_func(struct memory *mem, void *lhs, size_t lhsIndex, void *rhs, size_t rhsIndex, 
    size_t elementSize, void *userPtr)
{
    struct renderer *context = userPtr;

    struct renderer_layer *lhsLayer = lhs;
    struct renderer_layer *rhsLayer = rhs;

    basic_dict_set(context->layerDict, mem, context->layers, lhsLayer->name, 
        strlen(lhsLayer->name) + 1, &context->layers[rhsIndex]);

    *lhsLayer = *rhsLayer;
}

struct renderer *
renderer_create_context(struct memory *memoryContext)
{
    struct renderer *context = memory_alloc(memoryContext, sizeof(struct renderer));
    memset(context, 0, sizeof(struct renderer));

    context->memoryContext = memoryContext;

    context->glContext = opengl_create_context(memoryContext);

    context->textureMap = DICTIONARY(memoryContext, NULL);
    context->programMap = DICTIONARY(memoryContext, NULL);
    context->meshMap = DICTIONARY(memoryContext, NULL);
    context->layerDict = DICTIONARY(memoryContext, NULL);
    context->batchMap = DICTIONARY(memoryContext, __renderer_batch_hash_func);

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return context;
}

void
renderer_set_background_color(struct renderer *context, const struct vec4 *color)
{
    opengl_set_background_color(context->glContext, (const real32 *)color);
}

b32
renderer_load_program(struct renderer *context, const char *name)
{
    if (!basic_dict_get(context->programMap, context->programs, name))
    {
        struct opengl_program_info program = opengl_load_program(context->glContext, name);

        if (program.program != OPENGL_NULL_ID)
        {
            const i32 index = context->programCount;

            if (context->programs)
            {
                context->programs = memory_realloc(context->memoryContext, context->programs, 
                    sizeof(struct opengl_program_info)*(++context->programCount));
            }
            else
            {
                context->programs = memory_alloc(context->memoryContext, sizeof(struct opengl_program_info)*(++context->programCount));
            }

            memcpy(&context->programs[index], &program, sizeof(struct opengl_program_info));
            basic_dict_set(context->programMap, context->memoryContext, context->programs, name, strlen(name) + 1, &context->programs[index]);

            return B32_TRUE;
        }
    }

    return B32_FALSE;
}

b32
renderer_load_mesh(struct renderer *context, const char *name)
{
    // TODO:
    if (!basic_dict_get(context->meshMap, context->meshes, name))
    {
        char *path = memory_alloc(context->memoryContext, sizeof("meshes/") + strlen(name) + sizeof(".obj") - 1);
        sprintf(path, "meshes/%s.obj", name);

        FILE *meshFile = fopen(path, "r");
        memory_free(context->memoryContext, path);
        if (!meshFile)
        {
            fprintf(stderr, "renderer_load_mesh error: Unable to open mesh file '%s'. Aborting.\n", name);
            return B32_FALSE;
        }

        i32 verticesCount = 0;
        struct vec4 *vertices = malloc(sizeof(struct vec4)*100);
        
        i32 texCount = 0;
        struct vec2 *texes = malloc(sizeof(struct vec2)*100);
        
        i32 indicesCount = 0;
        u16 *indices = malloc(sizeof(u16)*100);

        char objReadBuffer[256];
        while (fgets(objReadBuffer, sizeof(objReadBuffer), meshFile))
        {
            char **symbols = NULL;
            size_t symbolCount = 0;

            const char *c = objReadBuffer;
            while (*c)
            {
                while (isspace(*c)){++c;};

                size_t symbolLength = strcspn(c, "\n\r\t ");

                if (symbolLength > 0)
                { 
                    const size_t symbolIndex = symbolCount; 

                    if (symbols) 
                    { 
                        symbols = realloc(symbols, sizeof(char *)*(++symbolCount));
                    }
                    else
                    {
                        symbols = malloc(sizeof(char *)*(++symbolCount));
                    }

                    symbols[symbolIndex] = malloc(symbolLength + 1);
                    snprintf(symbols[symbolIndex], symbolLength + 1, "%s", c);

                    c += symbolLength;
                }
            }

            b32 lineSuccess = B32_TRUE;
            for (i32 i = 0; i < symbolCount;)
            {
                if (lineSuccess)
                {
                    if (!strcmp(symbols[i], "#"))
                    {
                        continue;
                    }

                    if (!strcmp(symbols[i], "v"))
                    {
                        if ((i + 3 < symbolCount) &&
                            (isdigit(symbols[i + 1][0]) || symbols[i + 1][0] == '.') &&
                            (isdigit(symbols[i + 2][0]) || symbols[i + 2][0] == '.') &&
                            (isdigit(symbols[i + 3][0]) || symbols[i + 3][0] == '.'))
                        {
                            const i32 vertexIndex = verticesCount++;

                            vertices[vertexIndex].x = atof(symbols[++i]);
                            vertices[vertexIndex].y = atof(symbols[++i]);
                            vertices[vertexIndex].z = atof(symbols[++i]);

                            if ((i + 4 < symbolCount) &&
                                (isdigit(symbols[i + 1][0]) || symbols[i + 1][0] == '.'))
                            {
                                vertices[vertexIndex].w = atof(symbols[++i]);
                            }
                            else
                            {
                                vertices[vertexIndex].w = 1.f;
                            }
                        }
                        else
                        {
                            lineSuccess = B32_FALSE;
                        }
                    }
                    else if (!strcmp(symbols[i], "vt"))
                    {
                        if ((i + 1 < symbolCount) &&
                            (isdigit(symbols[i + 1][0]) || symbols[i + 1][0] == '.'))
                        {
                            const i32 texIndex = texCount++;

                            texes[texIndex].x = atof(symbols[i + 1]);

                            if ((i + 2 < symbolCount) &&
                                (isdigit(symbols[i + 2][0]) || symbols[i + 2][0] == '.'))
                            {
                                texes[texIndex].y = atof(symbols[i + 2]);
                            }
                            else
                            {
                                texes[texIndex].y = 0.f;
                            }
                        }
                        else
                        {
                            lineSuccess = B32_FALSE;
                        }
                    }
                    else if (!strcmp(symbols[i], "p"))
                    {
                    }
                    else
                    {
                        lineSuccess = B32_FALSE;
                    }
                }
            }
            free (symbols);
        }

        free (vertices);
        free (texes);
        free (indices);

        fclose(meshFile);

        return B32_TRUE;
    }

    return B32_FALSE;
}

b32
renderer_create_mesh(struct renderer *context, const char *name, void *vertices, size_t vertexSize, size_t verticesCount,
    u16 *indices, size_t indicesCount)
{
    if (!basic_dict_get(context->meshMap, context->meshes, name))
    {
        struct renderer_mesh mesh;

        const size_t nameSize = strlen(name) + 1;
        mesh.name = memory_alloc(context->memoryContext, nameSize);
        memcpy((char *)mesh.name, name, nameSize);

        const size_t verticesSize = vertexSize*verticesCount;
        const size_t indicesSize = sizeof(indices[0])*indicesCount;

        struct opengl_vao_info vao = opengl_create_vao(context->glContext);
        if (context->vertexArrayCount > 0)
        {
            context->vertexArrays = memory_realloc(context->memoryContext, context->vertexArrays, 
                sizeof(struct opengl_vao_info)*(++context->vertexArrayCount));
        }
        else
        {
            context->vertexArrays = memory_alloc(context->memoryContext,
                sizeof(struct opengl_vao_info)*(++context->vertexArrayCount));
        }
        memcpy(&context->vertexArrays[context->vertexArrayCount - 1], &vao, sizeof(vao));
        mesh.vertexArray = context->vertexArrayCount - 1;
        
        i64 lastVertexBufferIndex, lastVertexBufferSize;
        if (context->vertexBufferCount > 0)
        {
            lastVertexBufferIndex = context->vertexBufferCount - 1;
            lastVertexBufferSize = context->vertexBuffers[lastVertexBufferIndex].size;
        }
        else
        {
            lastVertexBufferIndex = lastVertexBufferSize = -1;
        }

        if (lastVertexBufferIndex >= 0 && lastVertexBufferSize >= 0)
        {
            mesh.vertexBuffer = lastVertexBufferIndex;
            mesh.verticesSize = verticesSize;
            mesh.verticesOffset = context->vertexBuffers[lastVertexBufferIndex].size;
            mesh.verticesStride = vertexSize;

            struct renderer_vertex_buffer *vertexBuffer = &context->vertexBuffers[lastVertexBufferIndex];
            memcpy((u8 *)vertexBuffer->vertices + lastVertexBufferSize, vertices, verticesSize);
            vertexBuffer->size += verticesSize;
            vertexBuffer->isDirty = B32_TRUE;
        }
        else
        {
            struct renderer_vertex_buffer vertexBuffer;
            vertexBuffer.glInfo = opengl_create_buffer(context->glContext, OPENGL_ARRAY_TARGET, 
               RENDERER_VERTEX_BUFFER_SIZE, NULL, B32_TRUE);
            vertexBuffer.size = verticesSize;
            vertexBuffer.vertices = memory_alloc(context->memoryContext, RENDERER_VERTEX_BUFFER_SIZE);
            memcpy(vertexBuffer.vertices, vertices, verticesSize);
            vertexBuffer.isDirty = B32_TRUE;

            const size_t index = context->vertexBufferCount;

            if (context->vertexBuffers)
            {
                context->vertexBuffers = memory_realloc(context->memoryContext, context->vertexBuffers, 
                    sizeof(struct renderer_vertex_buffer)*(++context->vertexBufferCount));
            }
            else
            {
                context->vertexBuffers = memory_alloc(context->memoryContext,
                    sizeof(struct renderer_vertex_buffer)*(++context->vertexBufferCount));
            }
            context->vertexBuffers[index] = vertexBuffer;
            mesh.vertexBuffer = index;
            mesh.verticesOffset = 0;
            mesh.verticesSize = verticesSize;
            mesh.verticesStride = vertexSize;
        }
        
        i64 lastIndexBufferIndex, lastIndexBufferSize;
        if (context->indexBufferCount > 0)
        {
            lastIndexBufferIndex = context->indexBufferCount - 1;
            lastIndexBufferSize = context->indexBuffers[lastIndexBufferIndex].length*
                sizeof(context->indexBuffers[lastIndexBufferIndex].indices[0]);
        }
        else
        {
            lastIndexBufferIndex = lastIndexBufferSize = -1;
        }

        mesh.indicesLength = indicesCount;
        if (lastIndexBufferIndex >= 0 && lastIndexBufferSize >= 0)
        {
            mesh.indexBuffer = lastIndexBufferIndex;
            mesh.indicesOffset = context->indexBuffers[lastIndexBufferIndex].length;

            struct renderer_index_buffer *indexBuffer = &context->indexBuffers[lastIndexBufferIndex];
            memcpy(&indexBuffer->indices[indexBuffer->length], indices, indicesSize);
            indexBuffer->length += indicesCount;
            indexBuffer->isDirty = B32_TRUE;
        }
        else
        {
            struct renderer_index_buffer indexBuffer;
            indexBuffer.glInfo = opengl_create_buffer(context->glContext, OPENGL_INDEX_TARGET, 
               RENDERER_INDEX_BUFFER_SIZE, NULL, B32_TRUE);
            indexBuffer.indices = memory_alloc(context->memoryContext, RENDERER_INDEX_BUFFER_SIZE);
            memcpy(indexBuffer.indices, indices, indicesSize);
            indexBuffer.length = indicesCount;
            indexBuffer.isDirty = B32_TRUE;

            const size_t index = context->indexBufferCount;

            if (context->indexBuffers)
            {
                context->indexBuffers = memory_realloc(context->memoryContext, context->indexBuffers, 
                    sizeof(struct renderer_index_buffer)*(++context->indexBufferCount));
            }
            else
            {
                context->indexBuffers = memory_alloc(context->memoryContext,
                    sizeof(struct renderer_index_buffer)*(++context->indexBufferCount));
            }
            context->indexBuffers[index] = indexBuffer;
            mesh.indexBuffer = index;
            mesh.indicesOffset = 0;
            mesh.indicesLength = context->indexBuffers[index].length;
        }

        const i32 meshIndex = context->meshCount;
        mesh.mesh = meshIndex;
        if (context->meshes)
        {
            context->meshes = memory_realloc(context->memoryContext, context->meshes, 
                sizeof(struct renderer_mesh)*(++context->meshCount));
        }
        else
        {
            context->meshes = memory_alloc(context->memoryContext,
                sizeof(struct renderer_mesh)*(++context->meshCount));
        }

        memcpy(&context->meshes[meshIndex], &mesh, sizeof(mesh));
        basic_dict_set(context->meshMap, context->memoryContext, context->meshes, name, strlen(name) + 1, &context->meshes[meshIndex]);
        
        return B32_TRUE;
    }
    else
    {
        fprintf(stderr, "renderer_create_mesh error: Mesh by name '%s' already exists!\n", name);
    }

    return B32_FALSE;
}

b32
renderer_save_texture(struct renderer *context, const char *name, const char* fileName, b32 isCompressed)
{
    struct renderer_texture *texPtr;
    if (!(texPtr = basic_dict_get(context->textureMap, context->textures, name)))
    {
        fprintf(stderr, "renderer_save_texture error: Texture \"%s\" does not exist in memory. Cannot save to disk as \"%s\".\n", name, fileName);

        return B32_FALSE;
    }

    if (!texPtr->isDynamic)
    {
        fprintf(stderr, "renderer_save_texture error: Texture \"%s\" is static. (filename: \"%s\") TODO: copy from VRAM for static textures implementation.\n", name, fileName);

        return B32_FALSE;
    }
    
    struct renderer_texture_buffer *bufferPtr = &context->textureBuffers[texPtr->buffer];

    struct renderer_texture_header header;
    header.identifier = RENDERER_TEXTURE_HEADER_ID;
    header.format = texPtr->format;
    header.width = bufferPtr->width;
    header.height = bufferPtr->height;
    header.isCompressed = isCompressed;

    u32 componentSize;
    switch(texPtr->format)
    {
        case RENDERER_TEXTURE_RGBAF32:
        {
            componentSize = sizeof(real32);
        } break;
        
        case RENDERER_TEXTURE_RGBAU32:
        default:
        {
            componentSize = 1;
        } break;
    }

    const u64 size = bufferPtr->width*bufferPtr->height*componentSize*4;
    
    i32 nameLength = strlen(fileName);
    char *pathBuffer = memory_alloc(context->memoryContext, strlen("resources/textures/") + nameLength + sizeof(".tex"));
    strcpy(pathBuffer, "resources/textures/");
    strcat(pathBuffer, fileName);
    strcat(pathBuffer, ".tex");

    FILE *texFile = fopen(pathBuffer, "wb");

    if (isCompressed)
    {
        struct tex_node
        {
            // bottom left of partition
            u32 x;
            u32 y;
            u32 width;
            u32 height;
            union
            {
                struct
                {
                    real32 f32[4];
                };
                struct
                {
                    u8 u32[4];
                    u8 _padding[12];
                };
            } color;
        };

        struct tex_node *nodes = NULL;
        u32 nodeCount = 0;
    
        // split texture into bounding box partitions,
        // according to overlapping color

        while (B32_TRUE)
        {
            // locate next non child overlapping pixel (lowest y)
            u32 x = 0;
            u32 y = 0;

            if (nodeCount > 0)
            {
                for (y = 0; y < bufferPtr->height;)
                {
                    b32 isOverlap = B32_FALSE;

                    for (x = 0; x < bufferPtr->width;)
                    {
                        for (i32 i = 0; i < nodeCount; ++i)
                        {
                            isOverlap = ((x >= nodes[i].x && x < (nodes[i].x + nodes[i].width)) &&
                                (y >= nodes[i].y && y < (nodes[i].y + nodes[i].height)));

                            if (isOverlap)
                            {
                                x += nodes[i].width;
                                break;
                            }
                        }

                        if (!isOverlap)
                        {
                            break;
                        }
                    }

                    if (!isOverlap)
                    {
                        break;
                    }
                    else
                    {
                        ++y;
                    }
                }
            }

            if (x == bufferPtr->width && y == bufferPtr->height)
            {
                break;
            }

            // find out bounds of current child partition of current node
            u32 width = bufferPtr->width - x;
            u32 height = bufferPtr->height - y;

            for (u32 y1 = y; y1 < (y + height); ++y1)
            {
                b32 isOverlap = B32_FALSE;
                b32 isDiffColor = B32_FALSE;

                for (u32 x1 = x; x1 < (x + width); ++x1)
                {
                    for (i32 i = 0; i < nodeCount; ++i)
                    {
                        isOverlap = ((x1 >= nodes[i].x && x1 < (nodes[i].x + nodes[i].width)) &&
                            (y1 >= nodes[i].y && y1 < (nodes[i].y + nodes[i].height)));

                        if (isOverlap)
                        {
                            break;
                        }
                    }

                    switch(texPtr->format)
                    {
                        case RENDERER_TEXTURE_RGBAF32:
                        {
                            if (texPtr->isDynamic)
                            {
                                u64 *lhs = (u64 *)(&((real32 *)bufferPtr->pixels)[(y*bufferPtr->width + x)*4]);
                                u64 *rhs = (u64 *)(&((real32 *)bufferPtr->pixels)[(y1*bufferPtr->width + x1)*4]);

                                isDiffColor = rhs[0] != lhs[0] || rhs[1] != lhs[1];
                            }
                            else
                            {
                                // TODO: read from vram
                            }
                        } break;
                        
                        case RENDERER_TEXTURE_RGBAU32:
                        default:
                        {
                            if (texPtr->isDynamic)
                            {
                                u32 *lhs = (u32 *)(&((u8 *)bufferPtr->pixels)[(y*bufferPtr->width + x)*4]);
                                u32 *rhs = (u32 *)(&((u8 *)bufferPtr->pixels)[(y1*bufferPtr->width + x1)*4]);

                                isDiffColor = *rhs != *lhs;
                            }
                            else
                            {
                                // TODO: read from vram
                            }
                        } break;
                    }
                    
                    if (isOverlap || isDiffColor)
                    {
                        if (x1 == x)
                        {
                            height = y1 - y;
                        }
                        else
                        {
                            width = x1 - x;
                        }
                        break;
                    }
                }
            }

            // insert partition into sub partitions of node
            i32 nodeIndex = nodeCount;
            if (nodeCount > 0)
            {
                nodes = memory_realloc(context->memoryContext, nodes, sizeof(struct tex_node)*(++nodeCount));
            }
            else
            {
                nodes = memory_alloc(context->memoryContext, sizeof(struct tex_node)*(++nodeCount));
            }

            nodes[nodeIndex].x = x;
            nodes[nodeIndex].y = y;
            nodes[nodeIndex].width = width;
            nodes[nodeIndex].height = height;
            switch(texPtr->format)
            {
                case RENDERER_TEXTURE_RGBAF32:
                {
                    real32 *pixel = &((real32 *)bufferPtr->pixels)[(y*bufferPtr->width + x)*4];
                    ((u64 *)nodes[nodeIndex].color.f32)[0] = ((u64 *)pixel)[0];
                    ((u64 *)nodes[nodeIndex].color.f32)[1] = ((u64 *)pixel)[1];
                } break;

                case RENDERER_TEXTURE_RGBAU32:
                default:
                {
                    u8 *pixel = &((u8 *)bufferPtr->pixels)[(y*bufferPtr->width + x)*4];
                    *((u32 *)nodes[nodeIndex].color.u32) = *((u32 *)pixel);
                } break;
            }
        }

        header.partitions = nodeCount;
        fwrite(&header, sizeof(header), 1, texFile);

        if (nodeCount > 0)
        {
            fwrite(nodes, sizeof(struct tex_node), nodeCount, texFile);
            memory_free(context->memoryContext, nodes);
        }
    }
    else
    {
        header.partitions = 0;
        fwrite(&header, sizeof(header), 1, texFile);
        
        if (texPtr->isDynamic)
        {
            fwrite(bufferPtr->pixels, size, 1, texFile);
        }
        else
        {
            // TODO: copy/write from vram
        }
    }

    fclose(texFile);

    memory_free(context->memoryContext, pathBuffer);

    return B32_TRUE;
}

b32
renderer_load_texture(struct renderer *context, const char* fileName, const char *name, b32 isDynamic)
{
    if (basic_dict_get(context->textureMap, context->textures, name))
    {
        fprintf(stderr, "renderer_load_texture error: Cannot load texture \"%s\" as \"%s\". Name already exists.\n", fileName, name);

        return B32_FALSE;
    }
    
    i32 nameLength = strlen(fileName);
    char *pathBuffer = memory_alloc(context->memoryContext, strlen("resources/textures/") + nameLength + sizeof(".tex"));
    strcpy(pathBuffer, "resources/textures/");
    strcat(pathBuffer, fileName);
    strcat(pathBuffer, ".tex");

    FILE *texFile = fopen(pathBuffer, "rb");
    if (!texFile)
    {
        fprintf(stderr, "renderer_load_texture error: Cannot load texture \"%s\" as \"%s\". Cannot locate file/resource.\n", fileName, name);
        memory_free(context->memoryContext, pathBuffer);

        return B32_FALSE;
    }

    struct renderer_texture_header header = {0};
    fread(&header, sizeof(header), 1, texFile);

    if (header.identifier != RENDERER_TEXTURE_HEADER_ID ||
        (header.isCompressed != B32_FALSE && header.isCompressed != B32_TRUE))
    {
        fprintf(stderr, "renderer_load_texture error: Cannot load texture \"%s\" as \"%s\". Invalid header.\n", fileName, name);
        fclose(texFile);
        memory_free(context->memoryContext, pathBuffer);

        return B32_FALSE;
    }

    u32 componentSize;
    switch(header.format)
    {
        case RENDERER_TEXTURE_RGBAF32:
        {
            componentSize = sizeof(real32);
        } break;
        
        case RENDERER_TEXTURE_RGBAU32:
        default:
        {
            componentSize = 1;
        } break;
    }

    const u64 size = header.width*header.height*componentSize*4;
    void *pixelBuffer = memory_alloc(context->memoryContext, size);

    if (header.isCompressed)
    {
        u8 partition[32];

        for (i32 i = 0; i < header.partitions; ++i)
        {
            fread(partition, sizeof(partition), 1, texFile);

            const u32 pX = ((u32 *)partition)[0];
            const u32 pY = ((u32 *)partition)[1];
            const u32 pW = ((u32 *)partition)[2];
            const u32 pH = ((u32 *)partition)[3];

            for (u32 y = pY; y < (pY + pH); ++y)
            {
                for (u32 x = pX; x < (pX + pW); ++x)
                {
                    memcpy(&((u8 *)pixelBuffer)[(y*header.width + x)*componentSize*4], &partition[16], componentSize*4);
                }
            }
        }
    }
    else
    {
        fread(pixelBuffer, 1, size, texFile);
    }

    if (!renderer_create_texture(context, name, header.format, isDynamic, header.width, header.height, pixelBuffer))
    {
        fprintf(stderr, "renderer_load_texture error: Cannot load texture \"%s\" as \"%s\". Problem creating/allocating texture.\n", 
            fileName, name);

        fclose(texFile);
        memory_free(context->memoryContext, pixelBuffer);
        memory_free(context->memoryContext, pathBuffer);

        return B32_FALSE;
    }

    fclose(texFile);
    memory_free(context->memoryContext, pixelBuffer);
    memory_free(context->memoryContext, pathBuffer);

    return B32_TRUE;
}

b32
renderer_create_texture(struct renderer *context, const char *name, enum renderer_texture_format format, b32 isDynamic,
    u32 width, u32 height, void *pixels)
{
    if (!isDynamic && !pixels)
    {
        fprintf(stderr, "renderer_create_texture error: Cannot create static texture \"%s\" with no data.\n", name);

        return B32_FALSE;
    }

    if (basic_dict_get(context->textureMap, context->textures, name))
    {
        fprintf(stderr, "renderer_create_texture error: Cannot create texture \"%s\". Name already exists.\n", name);

        return B32_FALSE;
    }
    
    i32 bufferIndex = context->textureBufferCount++;
    if (context->textureBuffers)
    {
        context->textureBuffers = memory_realloc(context->memoryContext, context->textureBuffers, 
            sizeof(struct renderer_texture_buffer)*context->textureBufferCount);
    }
    else
    {
        context->textureBuffers = memory_alloc(context->memoryContext,
            sizeof(struct renderer_texture_buffer)*context->textureBufferCount);
    }

    i32 componentSize;
    switch(format)
    {
        case RENDERER_TEXTURE_RGBAU32:
        {
            componentSize = 1;
        } break;;
        
        case RENDERER_TEXTURE_RGBAF32:
        {
            componentSize = 4;
        } break;;
        
        default:
        {
            componentSize = 1;
        } break;;
    }

    context->textureBuffers[bufferIndex].glInfo =
        opengl_create_buffer(context->glContext, OPENGL_TEXTURE_TARGET, width*height*(componentSize*4), pixels, isDynamic);
    context->textureBuffers[bufferIndex].width = width;
    context->textureBuffers[bufferIndex].height = height;
    if (isDynamic)
    {
        context->textureBuffers[bufferIndex].pixels = memory_alloc(context->memoryContext, width*height*(componentSize*4));
        if (pixels)
        {
            memcpy(context->textureBuffers[bufferIndex].pixels, pixels, width*height*(componentSize*4));
        }
        else
        {
            memset(context->textureBuffers[bufferIndex].pixels, 0, width*height*(componentSize*4));
        }
        
        context->textureBuffers[bufferIndex].isDirty = B32_TRUE;
    }
    else
    {
        context->textureBuffers[bufferIndex].pixels = NULL;
        context->textureBuffers[bufferIndex].isDirty = B32_FALSE;
    }

    i32 texIndex = context->textureCount++;
    if (context->textures)
    {
        context->textures = memory_realloc(context->memoryContext, context->textures, 
            sizeof(struct renderer_texture)*context->textureCount);
    }
    else
    {
        context->textures = memory_alloc(context->memoryContext,
            sizeof(struct renderer_texture)*context->textureCount);
    }
    
    const u64 nameLength = strlen(name) + 1;

    GLenum internalFormat;
    switch(format)
    {
        case RENDERER_TEXTURE_RGBAU32:
        {
            internalFormat = GL_RGBA8;
        } break;
        
        case RENDERER_TEXTURE_RGBAF32:
        {
            internalFormat = GL_RGBA32F;
        } break;
        
        default:
        {
            internalFormat = GL_RGBA8;
        } break;
    }

    context->textures[texIndex].id = texIndex; 
    context->textures[texIndex].format = format; 
    context->textures[texIndex].glInfo =
        opengl_create_texture(context->glContext, internalFormat, context->textureBuffers[bufferIndex].glInfo.buffer, width, height);
    context->textures[texIndex].buffer = bufferIndex;
    context->textures[texIndex].name = memory_alloc(context->memoryContext, nameLength);
    context->textures[texIndex].isDynamic = isDynamic;
    memcpy((void *)context->textures[texIndex].name, name, nameLength);
    basic_dict_set(context->textureMap, context->memoryContext, context->textures, name, nameLength, &context->textures[texIndex]);
    
    return B32_TRUE;
}

void
renderer_write_texture(struct renderer *context, renderer_id texture, u32 x, u32 y, u32 width, u32 height, void *pixels)
{
    struct renderer_texture *texPtr = &context->textures[texture];

    if (!texPtr->isDynamic)
    {
        fprintf(stderr, "renderer_write_texture error: Cannot write to static texture \"%s\".\n", texPtr->name);
        
        return;
    }

    struct renderer_texture_buffer *bufferPtr = &context->textureBuffers[texPtr->buffer];

    i32 componentSize;
    switch(texPtr->format)
    {
        case RENDERER_TEXTURE_RGBAU32:
        {
            componentSize = 1;
        } break;
        
        case RENDERER_TEXTURE_RGBAF32:
        {
            componentSize = sizeof(real32);
        } break;

        default:
        {
            componentSize = 1;
        } break;
    }

    for (u32 row = y; row < (y + height); ++row)
    {
        memcpy(&((u8 *)bufferPtr->pixels)[((row*bufferPtr->width) + x)*componentSize*4], &((u8 *)pixels)[(row - y)*width*componentSize*4], componentSize*4*width);
    }

    bufferPtr->isDirty = B32_TRUE;
}

void
renderer_lerp_texture_color(struct renderer *context, renderer_id texture, real32 t, u32 x, u32 y, u32 width, u32 height, u8 color[4])
{
    struct renderer_texture *texPtr = &context->textures[texture];

    if (!texPtr->isDynamic)
    {
        fprintf(stderr, "renderer_lerp_texture_color error: Cannot write to static texture \"%s\".\n", texPtr->name);
        
        return;
    }

    struct renderer_texture_buffer *bufferPtr = &context->textureBuffers[texPtr->buffer];
    
    real32 clampedT = t >= 0.f ? (t <= 1.f ? t : 1.f) : 0.f;

    switch (texPtr->format)
    {
        case RENDERER_TEXTURE_RGBAF32:
        {
            for (u32 dY = y; dY < (y + height); ++dY)
            {
                for (u32 dX = x; dX < (x + width); ++dX)
                {
                    real32 *texPixel = &((real32 *)bufferPtr->pixels)[(dY*bufferPtr->width + dX)*4];

                    real32 colorNorm[] = {
                        (real32)color[0]/255.f,
                        (real32)color[1]/255.f,
                        (real32)color[2]/255.f,
                        (real32)color[3]/255.f
                    };

                    texPixel[0] += (colorNorm[0] - texPixel[0])*t;
                    texPixel[1] += (colorNorm[1] - texPixel[1])*t;
                    texPixel[2] += (colorNorm[2] - texPixel[2])*t;
                    texPixel[3] += (colorNorm[3] - texPixel[3])*t;
                }
            }
        } break;

        case RENDERER_TEXTURE_RGBAU32:
        default:
        {
            for (u32 dY = y; dY < (y + height); ++dY)
            {
                for (u32 dX = x; dX < (x + width); ++dX)
                {
                    u32 *texPixel = &((u32 *)bufferPtr->pixels)[dY*bufferPtr->width + dX];

                    i16 dR = (i16)(*(u32 *)color & 0x000000FF) - (i16)(*texPixel & 0x000000FF);
                    i16 dG = (i16)((*(u32 *)color >> 8) & 0x000000FF) - (i16)((*texPixel >> 8) & 0x000000FF);
                    i16 dB = (i16)((*(u32 *)color >> 16) & 0x000000FF) - (i16)((*texPixel >> 16) & 0x000000FF);
                    i16 dA = (i16)((*(u32 *)color >> 24) & 0x00000FF) - (i16)((*texPixel >> 24) & 0x000000FF);

                    ((u8 *)texPixel)[0] = (u8)((i16)(((u8 *)texPixel)[0]) + (i16)roundf((real32)dR*clampedT));
                    ((u8 *)texPixel)[1] = (u8)((i16)(((u8 *)texPixel)[1]) + (i16)roundf((real32)dG*clampedT));
                    ((u8 *)texPixel)[2] = (u8)((i16)(((u8 *)texPixel)[2]) + (i16)roundf((real32)dB*clampedT));
                    ((u8 *)texPixel)[3] = (u8)((i16)(((u8 *)texPixel)[3]) + (i16)roundf((real32)dA*clampedT));
                }
            }
        } break;
    }

    bufferPtr->isDirty = B32_TRUE;
}

void
renderer_clear_texture(struct renderer *context, renderer_id texture, u32 x, u32 y, u32 width, u32 height, u8 color[4])
{
    struct renderer_texture *texPtr = &context->textures[texture];
    
    if (!texPtr->isDynamic)
    {
        fprintf(stderr, "renderer_clear_texture error: Cannot write to static texture \"%s\".\n", texPtr->name);
        
        return;
    }

    struct renderer_texture_buffer *bufferPtr = &context->textureBuffers[texPtr->buffer];
    
    switch(texPtr->format)
    {
        case RENDERER_TEXTURE_RGBAF32:
        {
            real32 colorNorm[] = {
                (real32)color[0]/255.f,
                (real32)color[1]/255.f,
                (real32)color[2]/255.f,
                (real32)color[3]/255.f
            };

            for (u32 row = y; row < (y + height); ++row)
            {
                for (u32 col = x; col < (x + width); ++col)
                {
                    memcpy(&((u8 *)bufferPtr->pixels)[(row*bufferPtr->width + col)*sizeof(real32)*4], colorNorm, sizeof(real32)*4);
                }
            }
        } break;

        case RENDERER_TEXTURE_RGBAU32:
        default:
        {
            for (u32 row = y; row < (y + height); ++row)
            {
                for (u32 col = x; col < (x + width); ++col)
                {
                    memcpy(&((u8 *)bufferPtr->pixels)[(row*bufferPtr->width + col)*4], color, 4);
                }
            }
        } break;
    }


    bufferPtr->isDirty = B32_TRUE;
}

renderer_id
renderer_get_texture(struct renderer *context, const char *name)
{
    struct renderer_texture *tex = basic_dict_get(context->textureMap, context->textures, name);

    return tex ? tex->id : RENDERER_NULL_ID;
}

renderer_id
renderer_instantiate_model(struct renderer *context, const char *meshName, const char *programName)
{
    struct opengl_program_info *program;
    assert(program = basic_dict_get(context->programMap, context->programs, programName));

    struct renderer_mesh *mesh;
    assert(mesh = basic_dict_get(context->meshMap, context->meshes, meshName));

    const renderer_id materialIndex = (renderer_id)context->materialCount;
    if (context->materialCount > 0)
    {
        context->materials = memory_realloc(context->memoryContext, context->materials, 
            sizeof(struct renderer_material)*(++context->materialCount));
    }
    else
    {
        context->materials = memory_alloc(context->memoryContext,
            sizeof(struct renderer_material)*(++context->materialCount));
    }
    struct renderer_material *material = &context->materials[materialIndex];
    material->material = materialIndex;
    material->texture = RENDERER_NULL_ID;
    material->program = (renderer_id)(program - context->programs);
    material->propertyCount = program->uniformCount;
    material->properties = memory_alloc(context->memoryContext, sizeof(struct renderer_material_property)*program->uniformCount);
    material->propertyDict = DICTIONARY(context->memoryContext, NULL);
    for (i32 property = 0; property < program->uniformCount; ++property)
    {
        const size_t typeSize = opengl_helper_get_type_size(program->uniforms[property].type);
        const u32 typeLength = program->uniforms[property].typeLength;
        const u32 uniformSize = typeSize*typeLength;

        struct renderer_material_property_buffer *bufferPtr = NULL;
        for (i32 buffer = 0; buffer < context->materialPropertyBufferCount; ++buffer)
        {
            if ((context->materialPropertyBuffers[buffer].capacity - context->materialPropertyBuffers[buffer].size)
                >= uniformSize)
            {
                bufferPtr = &context->materialPropertyBuffers[buffer];
                break;
            }
        }
        if (!bufferPtr)
        {
            const size_t bufferIndex = context->materialPropertyBufferCount;

            if (context->materialPropertyBufferCount > 0)
            {
                context->materialPropertyBuffers = memory_realloc(context->memoryContext, context->materialPropertyBuffers, 
                    sizeof(struct renderer_material_property_buffer)*(++context->materialPropertyBufferCount));
            }
            else
            {
                context->materialPropertyBuffers = memory_alloc(context->memoryContext,
                    sizeof(struct renderer_material_property_buffer)*(++context->materialPropertyBufferCount));
            }
            context->materialPropertyBuffers[bufferIndex].capacity = RENDERER_MATERIAL_PROPERTY_BUFFER_SIZE;
            context->materialPropertyBuffers[bufferIndex].size = 0;
            context->materialPropertyBuffers[bufferIndex].propertyData = memory_alloc(context->memoryContext, RENDERER_MATERIAL_PROPERTY_BUFFER_SIZE);

            assert(context->materialPropertyBuffers[bufferIndex].propertyData);

            bufferPtr = &context->materialPropertyBuffers[bufferIndex];
        }

        memset(&bufferPtr->propertyData[bufferPtr->size], 0, uniformSize);

        material->properties[property].propertyBufferIndex = context->materialPropertyBufferCount - 1;
        material->properties[property].propertyBufferOffset = bufferPtr->size;
        material->properties[property].uniform = program->uniforms[property].uniform;
        
        bufferPtr->size += uniformSize;

        basic_dict_set(material->propertyDict, context->memoryContext, material->properties, 
            program->uniforms[property].name, strlen(program->uniforms[property].name) + 1, &material->properties[property]);
    }
    
    const renderer_id modelIndex = context->modelCount;
    if (context->modelCount > 0)
    {
        context->models = memory_realloc(context->memoryContext, context->models, 
            sizeof(struct renderer_model)*(++context->modelCount));
    }
    else
    {
        context->models = memory_alloc(context->memoryContext,
            sizeof(struct renderer_model)*(++context->modelCount));
    }

    struct renderer_model *model = &context->models[modelIndex];
    model->mesh = (renderer_id)(mesh - context->meshes);
    model->material = materialIndex;
    model->model = modelIndex;
    
    return modelIndex;
}

b32
renderer_model_set_mesh(struct renderer *context, renderer_id model, const char *meshName)
{
    assert(model > RENDERER_NULL_ID && model < context->modelCount);

    const struct renderer_mesh *mesh = basic_dict_get(context->meshMap, context->meshes, meshName);

    if (mesh)
    {
        context->models[model].mesh = mesh->mesh;

        return B32_TRUE;
    }
    else
    {
        fprintf(stderr, "renderer_model_set_mesh error: Cannot locate mesh '%s'!\n", meshName);
    }

    return B32_FALSE;
}

b32
renderer_material_update_property(struct renderer *context, renderer_id material, const char *name, const void *value)
{
    assert(material > RENDERER_NULL_ID && material < context->materialCount);

    struct renderer_material *materialPtr = &context->materials[material];

    struct renderer_material_property *propertyPtr = basic_dict_get(materialPtr->propertyDict, materialPtr->properties, name);
    if (!propertyPtr)
    {
        fprintf(stderr, "renderer_material_update_property: Could not find property '%s' on material.\n", name);
        return B32_FALSE;
    }
    
    assert(propertyPtr->propertyBufferIndex > RENDERER_NULL_ID && propertyPtr->propertyBufferIndex < context->materialPropertyBufferCount);
    
    const u32 typeSize = opengl_helper_get_type_size(context->programs[materialPtr->program].uniforms[propertyPtr->uniform].type);
    const u32 typeLength = context->programs[materialPtr->program].uniforms[propertyPtr->uniform].typeLength;
    
    if (typeSize > 0)
    {
        memcpy(&context->materialPropertyBuffers[propertyPtr->propertyBufferIndex].
            propertyData[propertyPtr->propertyBufferOffset],
            value, typeSize*typeLength);
    }
    else
    {
        fprintf(stderr, "renderer_material_update_property error: Property '%s' has an invalid type.\n", name);
        return B32_FALSE;
    }

    return B32_TRUE;
}

b32
renderer_material_set_texture(struct renderer *context, renderer_id material, const char *textureName)
{
    assert(material > RENDERER_NULL_ID && material < context->materialCount);

    struct renderer_material *materialPtr = &context->materials[material];

    struct renderer_texture *texPtr = basic_dict_get(context->textureMap, context->textures, textureName);
    if (!texPtr)
    {
        fprintf(stderr, "renderer_material_set_texture error: Cannot set material's texture to \"%s\". Texture not found.\n", textureName);

        return B32_FALSE;
    }

    materialPtr->texture = texPtr->id;
    
    return B32_TRUE;
}

void
renderer_init_layer(struct renderer *context, const char *name, i32 layer)
{
    if (!basic_dict_get(context->layerDict, context->layers, name))
    {
        u32 layerIndex = context->layerCount;

        if (context->layerCount > 0)
        {
            context->layers = memory_realloc(context->memoryContext, context->layers, 
                sizeof(struct renderer_layer)*(++context->layerCount));
        }
        else
        {
            context->layers = memory_alloc(context->memoryContext, sizeof(struct renderer_layer)*(++context->layerCount));
        }
        
        const size_t nameSize =  strlen(name) + 1;

        context->layers[layerIndex].layer = layer;
        context->layers[layerIndex].name = memory_alloc(context->memoryContext, nameSize);
        sprintf((char *)context->layers[layerIndex].name, "%s", name);

        basic_dict_set(context->layerDict, context->memoryContext, context->layers, context->layers[layerIndex].name, 
            nameSize, &context->layers[layerIndex]);

        utils_sort(context->memoryContext, context->layers, sizeof(struct renderer_layer), 
            context->layerCount, __renderer_layer_sort_compare_func, __renderer_layer_sort_replace_func, context);
    }
    else
    {
        fprintf(stderr, "renderer_init_layer error: Layer '%s' already exists!\n", name);
    }
}

void
renderer_queue_model(struct renderer *context, renderer_id model, const char *layer)
{
    assert(model >= RENDERER_NULL_ID && model < context->modelCount);

    struct renderer_layer *layerPtr = basic_dict_get(context->layerDict, context->layers, layer);

    if (layerPtr)
    {
        const struct renderer_model *modelPtr = &context->models[model];

        struct renderer_batch_key batchKey;
        batchKey.layer = layerPtr->layer;
        batchKey.mesh = modelPtr->mesh;
        batchKey.program = renderer_model_get_material_program(context, modelPtr->model);
        batchKey.texture = context->materials[modelPtr->material].texture;

        struct renderer_batch *batch = basic_dict_get(context->batchMap, context->batches, &batchKey);

        if (!batch)
        {
            i32 batchIndex = context->batchCount;
            if (context->batchCount > 0)
            {
                context->batches = memory_realloc(context->memoryContext, context->batches, 
                    sizeof(struct renderer_batch)*(++context->batchCount));
            }
            else
            {
                context->batches = memory_alloc(context->memoryContext, 
                    sizeof(struct renderer_batch)*(++context->batchCount));
            }

            context->batches[batchIndex].layer = batchKey.layer;
            context->batches[batchIndex].program = batchKey.program;
            context->batches[batchIndex].texture = batchKey.texture;
            context->batches[batchIndex].mesh = batchKey.mesh;
            context->batches[batchIndex].length = 0;
            context->batches[batchIndex].models = memory_alloc(context->memoryContext, sizeof(renderer_id)*
                (context->batches[batchIndex].capacity = 10));

            batch = &context->batches[batchIndex];
            basic_dict_set(context->batchMap, context->memoryContext, context->batches, &batchKey, sizeof(batchKey), batch);
        }

        if (batch->length >= batch->capacity)
        {
            batch->models = memory_realloc(context->memoryContext, batch->models, 
                sizeof(renderer_id)*(batch->capacity *= 10));
        }

        batch->models[batch->length++] = model;
    }
    else
    {
        fprintf(stderr, "renderer_queue_model error: Cannot queue model. Invalid layer '%s'\n", layer);
    }
}

void
renderer_draw(struct renderer *context)
{
    glClear(GL_COLOR_BUFFER_BIT);

    for (i32 batchIndex = 0; batchIndex < context->batchCount; ++batchIndex)
    {
        struct renderer_batch *batchPtr = &context->batches[batchIndex];

        if (batchPtr->length > 0)
        {
            struct opengl_program_info *programPtr = &context->programs[batchPtr->program];

            opengl_use_program(context->glContext, context->programs[batchPtr->program].program);

            struct renderer_mesh *meshPtr = &context->meshes[batchPtr->mesh];
            struct opengl_vao_info *meshVao = &context->vertexArrays[meshPtr->vertexArray];
            struct renderer_vertex_buffer *meshVertexBuffer = &context->vertexBuffers[meshPtr->vertexBuffer];
            struct renderer_index_buffer *meshIndexBuffer = &context->indexBuffers[meshPtr->indexBuffer];
            
            opengl_bind_vao(context->glContext, meshVao->vao);
            opengl_bind_buffer_target(context->glContext, meshVertexBuffer->glInfo.buffer, OPENGL_ARRAY_TARGET);
            opengl_bind_buffer_target(context->glContext, meshIndexBuffer->glInfo.buffer, OPENGL_INDEX_TARGET);

            // TODO: only update parts that needed updating (can't set to false until all parts have been on the buffer)
            if (meshVertexBuffer->isDirty)
            {
                opengl_write_buffer_target(context->glContext, OPENGL_ARRAY_TARGET, 0, 
                    meshVertexBuffer->size, meshVertexBuffer->vertices);
                meshVertexBuffer->isDirty = B32_FALSE;
            }
            else if (meshIndexBuffer->isDirty)
            {
                opengl_write_buffer_target(context->glContext, OPENGL_INDEX_TARGET, 0,
                    meshIndexBuffer->length*sizeof(u16), meshIndexBuffer->indices);
                meshIndexBuffer->isDirty = B32_FALSE;
            }

            if (batchPtr->texture != RENDERER_NULL_ID)
            {
                struct renderer_texture *texPtr = &context->textures[batchPtr->texture];
                struct renderer_texture_buffer *texBufferPtr = &context->textureBuffers[texPtr->buffer];
            
                if (texBufferPtr->isDirty)
                {
                    opengl_bind_buffer_target(context->glContext, texBufferPtr->glInfo.buffer, OPENGL_TEXTURE_TARGET);
                    opengl_write_buffer_target(context->glContext, OPENGL_TEXTURE_TARGET, 0, 
                        texBufferPtr->glInfo.capacity, texBufferPtr->pixels);
                    texBufferPtr->isDirty = B32_FALSE;
                }

                opengl_bind_texture(context->glContext, texPtr->glInfo.texture);
            }

            size_t attribOffset = 0;
            for (i32 attribIndex = 0; attribIndex < programPtr->attribCount; ++attribIndex)
            {
                const GLuint location = context->glContext->programs[programPtr->program].
                    attribs[programPtr->attribs[attribIndex].attrib].location;
                
                glEnableVertexAttribArray(location);

                glVertexAttribPointer(location, programPtr->attribs[attribIndex].typeLength, 
                    programPtr->attribs[attribIndex].type, GL_FALSE,
                    meshPtr->verticesStride, (void *)(attribOffset + meshPtr->verticesOffset));

                attribOffset += opengl_helper_get_type_size(programPtr->attribs[attribIndex].type)*
                    programPtr->attribs[attribIndex].typeLength;
            }
            
            for (i32 modelIndex = 0;
                 modelIndex < batchPtr->length;
                 ++modelIndex)
            {
                struct renderer_model *modelPtr = &context->models[batchPtr->models[modelIndex]];
                struct renderer_material *materialPtr = &context->materials[modelPtr->material];

                for (i32 propertyIndex = 0; propertyIndex < materialPtr->propertyCount; ++propertyIndex)
                {
                    opengl_assign_program_uniform(context->glContext, programPtr->program, 
                        materialPtr->properties[propertyIndex].uniform,
                        &context->materialPropertyBuffers[materialPtr->properties[propertyIndex].
                        propertyBufferIndex].propertyData[materialPtr->properties[propertyIndex].propertyBufferOffset]);
                }

                glEnable(GL_BLEND);
                glDrawElements(GL_TRIANGLES, meshPtr->indicesLength, GL_UNSIGNED_SHORT, 
                    (void *)(meshPtr->indicesOffset*sizeof(u16)));
                glDisable(GL_BLEND);
            }
            
            for (i32 attribIndex = 0; attribIndex < programPtr->attribCount; ++attribIndex)
            {
                const GLuint location = context->glContext->programs[programPtr->program].
                    attribs[programPtr->attribs[attribIndex].attrib].location;
                
                glDisableVertexAttribArray(location);
            }
            
            batchPtr->length = 0;
        }
    }
}

renderer_id
renderer_model_get_mesh(const struct renderer *context, renderer_id model)
{
    assert(model > RENDERER_NULL_ID && model < context->modelCount);
    return context->models[model].mesh;
}

renderer_id
renderer_model_get_material(const struct renderer *context, renderer_id model)
{
    assert(model > RENDERER_NULL_ID && model < context->modelCount);
    return context->models[model].material;
}

renderer_id
renderer_model_get_material_program(const struct renderer *context, renderer_id model)
{
    assert(model > RENDERER_NULL_ID && model < context->modelCount);
    return context->materials[context->models[model].material].program;
}

void
renderer_material_set_program(struct renderer *context, renderer_id material, const char *program)
{
    assert(material > RENDERER_NULL_ID && material < context->materialCount);
    
    struct opengl_program_info *programInfo = basic_dict_get(context->programMap, context->programs, program);
    struct renderer_material *materialPtr = &context->materials[material];

    materialPtr->program = (renderer_id)(programInfo - context->programs);
    materialPtr->propertyCount = programInfo->uniformCount;
    materialPtr->properties = memory_realloc(context->memoryContext, materialPtr->properties, 
        sizeof(struct renderer_material_property)*programInfo->uniformCount);
    basic_dict_clear(materialPtr->propertyDict);
    for (i32 property = 0; property < programInfo->uniformCount; ++property)
    {
        const size_t typeSize = opengl_helper_get_type_size(programInfo->uniforms[property].type);
        const u32 typeLength = programInfo->uniforms[property].typeLength;
        const u32 uniformSize = typeSize*typeLength;

        struct renderer_material_property_buffer *bufferPtr = NULL;
        for (i32 buffer = 0; buffer < context->materialPropertyBufferCount; ++buffer)
        {
            if ((context->materialPropertyBuffers[buffer].capacity - context->materialPropertyBuffers[buffer].size)
                >= uniformSize)
            {
                bufferPtr = &context->materialPropertyBuffers[buffer];
                break;
            }
        }
        if (!bufferPtr)
        {
            const size_t bufferIndex = context->materialPropertyBufferCount;

            if (context->materialPropertyBufferCount > 0)
            {
                context->materialPropertyBuffers = memory_realloc(context->memoryContext, context->materialPropertyBuffers, 
                    sizeof(struct renderer_material_property_buffer)*(++context->materialPropertyBufferCount));
            }
            else
            {
                context->materialPropertyBuffers = memory_alloc(context->memoryContext,
                    sizeof(struct renderer_material_property_buffer)*(++context->materialPropertyBufferCount));
            }
            context->materialPropertyBuffers[bufferIndex].capacity = RENDERER_MATERIAL_PROPERTY_BUFFER_SIZE;
            context->materialPropertyBuffers[bufferIndex].size = 0;
            context->materialPropertyBuffers[bufferIndex].propertyData = memory_alloc(context->memoryContext, RENDERER_MATERIAL_PROPERTY_BUFFER_SIZE);

            bufferPtr = &context->materialPropertyBuffers[bufferIndex];
        }

        memset(&bufferPtr->propertyData[bufferPtr->size], 0, uniformSize);

        materialPtr->properties[property].propertyBufferIndex = context->materialPropertyBufferCount - 1;
        materialPtr->properties[property].propertyBufferOffset = bufferPtr->size;
        materialPtr->properties[property].uniform = programInfo->uniforms[property].uniform;
        
        bufferPtr->size += uniformSize;

        basic_dict_set(materialPtr->propertyDict, context->memoryContext, materialPtr->properties, 
            programInfo->uniforms[property].name, strlen(programInfo->uniforms[property].name) + 1, &materialPtr->properties[property]);
    }
}
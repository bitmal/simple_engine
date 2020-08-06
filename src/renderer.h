#ifndef __RENDERER_H
#define __RENDERER_H

#include "types.h"
#include "opengl.h"

typedef i32 renderer_id;
#define RENDERER_NULL_ID -1

struct memory;
struct vec2;
struct vec4;
struct basic_dict;

struct renderer_layer
{
    const char *name;
    i32 layer;
};

struct renderer_vertex_buffer
{
    struct opengl_buffer_info glInfo;
    void *vertices;
    size_t size;
    b32 isDirty;
};

struct renderer_index_buffer
{
    struct opengl_buffer_info glInfo;
    u16 *indices;
    size_t length;
    b32 isDirty;
};

struct renderer_texture_buffer
{
    struct opengl_buffer_info glInfo;
    void *pixels;
    u32 width;
    u32 height;
    b32 isDirty;
};

enum renderer_texture_format
{
    RENDERER_TEXTURE_RGBAU32,
    RENDERER_TEXTURE_RGBAF32
};

#define RENDERER_TEXTURE_HEADER_ID 0xFADEFADE

#pragma pack(push, 8)
struct renderer_texture_header
{
    u32 identifier;
    enum renderer_texture_format format;
    u32 width;
    u32 height;
    u32 partitions;
    b32 isCompressed;
};
#pragma pack(pop)

struct renderer_texture
{
    renderer_id id;
    const char *name;
    enum renderer_texture_format format;
    struct opengl_texture_info glInfo;
    renderer_id buffer;
    b32 isDynamic;
};

struct renderer_material_property
{
    opengl_id uniform;
    opengl_id propertyBufferIndex;
    size_t propertyBufferOffset;
};

struct renderer_material_property_buffer
{
    u8 *propertyData;
    size_t capacity;
    size_t size;
};

struct renderer_material
{
    renderer_id material;
    renderer_id program;
    renderer_id texture;
    i32 propertyCount;
    struct renderer_material_property *properties;
    struct basic_dict *propertyDict;
};

struct renderer_mesh_bounds
{
    real32 left;
    real32 bottom;
    real32 right;
    real32 top;
    real32 front;
    real32 back;
};

struct renderer_mesh
{
    const char *name;
    renderer_id mesh;
    renderer_id vertexArray;
    renderer_id vertexBuffer;
    renderer_id indexBuffer;
    size_t verticesSize;
    size_t verticesStride;
    size_t verticesOffset;
    size_t indicesLength;
    size_t indicesOffset;
};

struct renderer_model
{
    renderer_id model;
    renderer_id mesh;
    renderer_id material;
};

struct renderer_batch_key
{
    renderer_id layer;
    renderer_id program;
    renderer_id mesh;
    renderer_id texture;
};

struct renderer_batch
{
    renderer_id layer;
    renderer_id program;
    renderer_id mesh;
    renderer_id texture;
    i32 length;
    i32 capacity;
    renderer_id *models;
};

struct renderer
{
    struct opengl *glContext;
    struct memory *memoryContext;
    i32 programCount;
    struct opengl_program_info *programs;
    struct basic_dict *programMap;
    i32 vertexArrayCount;
    struct opengl_vao_info *vertexArrays;
    i32 vertexBufferCount;
    struct renderer_vertex_buffer *vertexBuffers;
    i32 indexBufferCount;
    struct renderer_index_buffer *indexBuffers;
    i32 textureBufferCount;
    struct renderer_texture_buffer *textureBuffers;
    i32 textureCount;
    struct renderer_texture *textures;
    struct basic_dict *textureMap;
    i32 materialCount;
    struct renderer_material *materials;
    i32 materialPropertyBufferCount;
    struct renderer_material_property_buffer *materialPropertyBuffers;
    i32 meshCount;
    struct renderer_mesh *meshes;
    struct basic_dict *meshMap;
    i32 modelCount;
    struct renderer_model *models;
    i32 layerCount;
    struct renderer_layer *layers;
    struct basic_dict *layerDict;
    i32 batchCount;
    struct renderer_batch *batches;
    struct basic_dict *batchMap;
};

struct renderer *
renderer_create_context(struct memory *memoryContext);

void
renderer_set_background_color(struct renderer *context, const struct vec4 *color);

b32
renderer_load_program(struct renderer *context, const char *name);

b32
renderer_load_mesh(struct renderer *context, const char *name);

b32
renderer_create_mesh(struct renderer *context, const char *name, void *vertices, size_t vertexSize, size_t verticesCount,
    u16 *indices, size_t indicesCount);

b32
renderer_save_texture(struct renderer *context, const char *name, const char* fileName, b32 isCompressed);

b32
renderer_load_texture(struct renderer *context, const char* fileName, const char *name, b32 isDynamic);

b32
renderer_create_texture(struct renderer *context, const char *name, enum renderer_texture_format format, b32 isDynamic, 
    u32 width, u32 height, void *pixels);

void
renderer_write_texture(struct renderer *context, renderer_id texture, u32 x, u32 y, u32 width, u32 height, void *pixels);

void
renderer_lerp_texture_color(struct renderer *context, renderer_id texture, real32 t, u32 x, u32 y, u32 width, u32 height, u8 color[4]);

void
renderer_clear_texture(struct renderer *context, renderer_id texture, u32 x, u32 y, u32 width, u32 height, u8 color[4]);

renderer_id
renderer_get_texture(struct renderer *context, const char *name);

renderer_id
renderer_instantiate_model(struct renderer *context, const char *meshName, const char *programName);

b32
renderer_model_set_mesh(struct renderer *context, renderer_id model, const char *meshName);

b32
renderer_material_update_property(struct renderer *context, renderer_id material, const char *name, const void *value);

b32
renderer_material_set_texture(struct renderer *context, renderer_id material, const char *textureName);

void
renderer_init_layer(struct renderer *context, const char *name, i32 layer);

void
renderer_queue_model(struct renderer *context, renderer_id model, const char *layer);

void
renderer_draw(struct renderer *context);

renderer_id
renderer_model_get_mesh(const struct renderer *context, renderer_id model);

renderer_id
renderer_model_get_material(const struct renderer *context, renderer_id model);

renderer_id
renderer_model_get_material_program(const struct renderer *context, renderer_id model);

void
renderer_material_set_program(struct renderer *context, renderer_id material, const char *program);

#endif
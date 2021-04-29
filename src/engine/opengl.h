#ifndef __OPENGL_H
#define __OPENGL_H

#include "types.h"

#define OPENGL_PROGRAM_ATTRIB_POSITION 0
#define OPENGL_PROGRAM_ATTRIB_COLOR 1 
#define OPENGL_PROGRAM_ATTRIB_TEX 2
#define OPENGL_PROGRAM_ATTRIB_ANY 3 

typedef i32 opengl_id;
struct opengl;
struct basic_dict;
struct memory;
#define OPENGL_NULL_ID (opengl_id)-1

enum opengl_buffer_target_type
{
    OPENGL_ARRAY_TARGET = 0,
    OPENGL_INDEX_TARGET,
    OPENGL_TEXTURE_TARGET,
    OPENGL_BUFFER_TARGET_LENGTH
};

struct opengl_buffer_info
{
    opengl_id buffer;
    size_t capacity;
    b32 isDynamic;
};

struct opengl_texture_info
{
    opengl_id texture;
    opengl_id buffer;
    u32 width;
    u32 height;
};

struct opengl_vao_info
{
    opengl_id vao;
};

struct opengl_attrib_info
{
    const char *name;
    const opengl_id attrib;
    const u32 type;
    const u32 typeLength;
};

struct opengl_uniform_info
{
    const char *name;
    const opengl_id uniform;
    const u32 type;
    const u32 typeLength;
};

struct opengl_program_info
{
    const char *name;
    const opengl_id program;
    const i32 attribCount;
    const struct opengl_attrib_info *attribs;
    const struct basic_dict *attribDict;
    const i32 uniformCount;
    const struct opengl_uniform_info *uniforms;
    const struct basic_dict *uniformDict;
};

const char *
opengl_helper_type_to_literal_str(u32 type);

u32
opengl_helper_literal_str_to_type(const char *typeStr);

size_t
opengl_helper_get_type_size(u32 type);

struct opengl *
opengl_create_context(struct memory *memoryContext);

void 
opengl_set_background_color(struct opengl *context, const real32 *v4);

struct opengl_program_info
opengl_load_program(struct opengl *context, const char *programName);

void 
opengl_use_program(struct opengl *context, opengl_id program);

void 
opengl_assign_program_uniform(struct opengl *context, opengl_id program, opengl_id uniform, const void *data);

struct opengl_vao_info 
opengl_create_vao(struct opengl *context);

void 
opengl_bind_vao(struct opengl *context, opengl_id vao);

void 
opengl_bind_buffer_target(struct opengl *context, opengl_id buffer, enum opengl_buffer_target_type target);

void 
opengl_write_buffer_target(struct opengl *context, enum opengl_buffer_target_type target, size_t offset, size_t size, void *data);

struct opengl_buffer_info 
opengl_create_buffer(struct opengl *context, enum opengl_buffer_target_type initTarget, size_t size, void *data, b32 isDynamic);

struct opengl_texture_info
opengl_create_texture(struct opengl *context, GLenum internalFormat, opengl_id buffer, u32 width, u32 height);

void
opengl_bind_texture(struct opengl *context, opengl_id texture);

#endif
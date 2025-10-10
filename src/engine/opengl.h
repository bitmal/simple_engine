#ifndef __OPENGL_H
#define __OPENGL_H

#include "types.h"
#include "memory.h"

#include <GL/glew.h>
#include <SDL3/SDL.h>

#include <stdlib.h>
#include <stdint.h>

#define OPENGL_PROGRAM_ATTRIB_POSITION 0
#define OPENGL_PROGRAM_ATTRIB_COLOR 1 
#define OPENGL_PROGRAM_ATTRIB_TEX 2
#define OPENGL_PROGRAM_ATTRIB_ANY 3 

typedef u32 opengl_id;
#define OPENGL_NULL_ID ((opengl_id)UINT32_MAX)

typedef i32 opengl_result_code;

#define OPENGL_OK ((opengl_result_code)0)
#define OPENGL_NOT_OK ((opengl_result_code)-1)

#define OPENGL_ERROR_CODE_BUFFER_TOO_SMALL ((opengl_result_code)-2)
#define OPENGL_ERROR_NULL_ARGUMENT ((opengl_result_code)-3)
#define OPENGL_ERROR_UNKNOWN_TYPE ((opengl_result_code)-4)
#define OPENGL_ERROR_NULL_CONTEXT ((opengl_result_code)-5)

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

struct opengl_vertex_array_info
{
    opengl_id vertexArrayId;
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

opengl_result_code
opengl_helper_type_to_literal_str(u32 type, u64 bufferLength, char *outBuffer);

opengl_result_code
opengl_helper_literal_str_to_type(const char *typeStr, u32 *outResult);

opengl_result_code
opengl_helper_get_type_size(u32 type, u32 *outResult);

opengl_result_code
opengl_create_context(const struct memory_context_key *memoryKeyPtr,
    SDL_Window *sdlWindow,
    const struct memory_allocation_key *outContextKeyPtr);

opengl_result_code
opengl_make_current(const struct memory_allocation_key *contextKeyPtr,
    SDL_Window *sdlWindow);

opengl_result_code
opengl_destroy_context(const struct memory_allocation_key *ioContextKeyPtr);

opengl_result_code
opengl_get_sdl_gl_context(const struct memory_alocation_key *contextKeyPtr,
    SDL_GLContext *outSDLGLContext);

opengl_result_code
opengl_get_sdl_window(const struct memory_alocation_key *contextKeyPtr,
    SDL_Window *outSDLWindow);

opengl_result_code 
opengl_set_background_color(const struct memory_allocation_key *contextKeyPtr, 
    const real32 v[4]);

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
#include "opengl.h"
#include "vec4.h"
#include "types.h"
#include "memory.h"

#include "glew/glew.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

struct opengl_attrib
{
    GLuint location;
};

struct opengl_uniform
{
    GLuint location;
    GLenum type;
    GLsizei typeLength;
};

struct opengl_program
{
    GLuint glId;
    i32 attribCount;
    struct opengl_attrib *attribs;
    i32 uniformCount;
    struct opengl_uniform *uniforms;
};

struct opengl_vao
{
    GLuint glId;

    opengl_id program;
    opengl_id vertexBuffer;
    opengl_id elementBuffer;
};

struct opengl_buffer
{
    GLuint glId;
};

struct opengl_buffer_target
{
    enum opengl_buffer_target_type type;
    opengl_id buffer;
};

struct opengl_texture
{
    GLuint textureGLid;
    GLuint samplerGLid;
    GLenum internalFormat;
    opengl_id buffer;
};

struct opengl
{
    struct memory *memoryContext;
    real32 bgColor[4];
    i32 programCount;
    struct opengl_program *programs;
    opengl_id activeProgram;
    i32 vaoCount;
    struct opengl_vao *vertexArrays;
    opengl_id boundVao;
    i32 bufferCount;
    struct opengl_buffer *buffers;
    struct opengl_buffer_target bufferTargets[OPENGL_BUFFER_TARGET_LENGTH];
    i32 textureCount;
    struct opengl_texture *textures;
};

struct opengl_sort_context
{
    struct opengl_program *program;
    struct opengl_program_info *programInfo;
};

static GLenum
__opengl_get_gl_target(enum opengl_buffer_target_type target)
{
    GLenum result;

    switch(target)
    {
        case OPENGL_ARRAY_TARGET:
        {
            result = GL_ARRAY_BUFFER;
        } break;
        
        case OPENGL_INDEX_TARGET:
        {
            result = GL_ELEMENT_ARRAY_BUFFER;
        } break;
        
        case OPENGL_TEXTURE_TARGET:
        {
            result = GL_TEXTURE_BUFFER;
        } break;

        default:
        {
            result = GL_ARRAY_BUFFER;
        } break;
    }

    return result;
}

static i32 
__opengl_sort_attrib_info_compare_func(struct memory *mem, void *lhs, size_t lhsIndex, void *rhs, size_t rhsIndex, void *userPtr)
{
    struct opengl_sort_context *context = userPtr;

    struct opengl_attrib *lhsAttrib = &context->program->attribs[((struct opengl_attrib_info *)lhs)->attrib];
    struct opengl_attrib *rhsAttrib = &context->program->attribs[((struct opengl_attrib_info *)rhs)->attrib];

    if (lhsAttrib->location > rhsAttrib->location)
    {
        return 1;
    }
    else if (lhsAttrib->location < rhsAttrib->location)
    {
        return -1;
    }

    return 0;
}

static void
__opengl_sort_attrib_info_replace_func(struct memory *mem, void *lhs, size_t lhsIndex, void *rhs, 
    size_t rhsIndex, size_t elementSize, void *userPtr)
{
    struct opengl_sort_context *context = userPtr;

    struct opengl_attrib_info *lhsInfo = (struct opengl_attrib_info *)lhs;
    struct opengl_attrib_info *rhsInfo = (struct opengl_attrib_info *)rhs;

    basic_dict_set((struct basic_dict *)context->programInfo->attribDict, mem, (struct opengl_attrib_info *)context->programInfo->attribs, 
        lhsInfo->name, strlen(lhsInfo->name) + 1, (struct opengl_attrib_info *)&context->programInfo->attribs[lhsIndex]);
    const opengl_id attribId = lhsInfo->attrib;
    memcpy(lhsInfo, rhsInfo, sizeof(struct opengl_attrib_info));
    UTILS_MUTABLE_CAST(opengl_id, lhsInfo->attrib) = attribId;
}

static i32 
__opengl_sort_attrib_compare_func(struct memory *mem, void *lhs, size_t lhsIndex, void *rhs, size_t rhsIndex, void *userPtr)
{
    struct opengl_attrib *lhsAttrib = lhs;
    struct opengl_attrib *rhsAttrib = rhs;

    if (lhsAttrib->location > rhsAttrib->location)
    {
        return 1;
    }
    else if (lhsAttrib->location < rhsAttrib->location)
    {
        return -1;
    }

    return 0;
}

static void
__opengl_sort_attrib_replace_func(struct memory *mem, void *lhs, size_t lhsIndex, void *rhs, 
    size_t rhsIndex, size_t elementSize, void *userPtr)
{
    struct opengl_attrib *lhsAttrib = lhs;
    struct opengl_attrib *rhsAttrib = rhs;

    *lhsAttrib = *rhsAttrib;
}

const char *
opengl_helper_type_to_literal_str(u32 type)
{
    const u64 offset = sizeof("GL_") - 1;

    switch(type)
    {
        case GL_FLOAT:
        {
            return "GL_FLOAT" + offset;
        } break;
        
        case GL_INT:
        {
            return "GL_INT" + offset;
        } break;
        
        case GL_UNSIGNED_INT:
        {
            return "GL_UNSIGNED_INT" + offset;
        } break;
        
        case GL_FLOAT_VEC2:
        {
            return "GL_FLOAT_VEC2" + offset;
        } break;
        
        case GL_FLOAT_VEC3:
        {
            return "GL_FLOAT_VEC3" + offset;
        } break;
        
        case GL_FLOAT_VEC4:
        {
            return "GL_FLOAT_VEC4" + offset;
        } break;
        
        case GL_INT_VEC2:
        {
            return "GL_INT_VEC2" + offset;
        } break;
        
        case GL_INT_VEC3:
        {
            return "GL_INT_VEC3" + offset;
        } break;
        
        case GL_INT_VEC4:
        {
            return "GL_INT_VEC4" + offset;
        } break;
        
        case GL_UNSIGNED_INT_VEC2:
        {
            return "GL_UNSIGNED_INT_VEC2" + offset;
        } break;
        
        case GL_UNSIGNED_INT_VEC3:
        {
            return "GL_UNSIGNED_INT_VEC3" + offset;
        } break;
        
        case GL_UNSIGNED_INT_VEC4:
        {
            return "GL_UNSIGNED_INT_VEC4" + offset;
        } break;
        
        case GL_FLOAT_MAT4:
        {
            return "GL_FLOAT_MAT4" + offset;
        } break;

        default:
        {
            return "GL_NONE" + offset;
        }
    }
}

u32
opengl_helper_literal_str_to_type(const char *typeStr)
{
    const char *offsetPtr = typeStr - (sizeof("GL_") - 1);

    if (offsetPtr == "GL_FLOAT")
    {
        return GL_FLOAT;
    }
    else if (offsetPtr == "GL_INT")
    {
        return GL_INT;
    }
    else if (offsetPtr == "GL_UNSIGNED_INT")
    {
        return GL_UNSIGNED_INT;
    }
    else if (offsetPtr == "GL_FLOAT_VEC2")
    {
        return GL_FLOAT_VEC2;
    }
    else if (offsetPtr == "GL_INT_VEC2")
    {
        return GL_INT_VEC2;
    }
    else if (offsetPtr == "GL_UNSIGNED_INT_VEC2")
    {
        return GL_UNSIGNED_INT_VEC2;
    }
    else if (offsetPtr == "GL_FLOAT_VEC3")
    {
        return GL_FLOAT_VEC3;
    }
    else if (offsetPtr == "GL_INT_VEC3")
    {
        return GL_INT_VEC3;
    }
    else if (offsetPtr == "GL_UNSIGNED_INT_VEC3")
    {
        return GL_UNSIGNED_INT_VEC3;
    }
    else if (offsetPtr == "GL_FLOAT_VEC4")
    {
        return GL_FLOAT_VEC4;
    }
    else if (offsetPtr == "GL_INT_VEC4")
    {
        return GL_INT_VEC4;
    }
    else if (offsetPtr == "GL_UNSIGNED_INT_VEC4")
    {
        return GL_UNSIGNED_INT_VEC4;
    }
    else if (offsetPtr == "GL_FLOAT_MAT4")
    {
        return GL_FLOAT_MAT4;
    }

    return GL_NONE;
}

size_t
opengl_helper_get_type_size(u32 type)
{
    switch(type)
    {
        case GL_FLOAT:
        {
            return sizeof(GLfloat);
        } break;
        
        case GL_INT:
        {
            return sizeof(GLint);
        } break;
        
        case GL_UNSIGNED_INT:
        {
            return sizeof(GLuint);
        } break;
        
        case GL_FLOAT_VEC2:
        {
            return sizeof(GLfloat)*2;
        } break;
        
        case GL_FLOAT_VEC3:
        {
            return sizeof(GLfloat)*3;
        } break;
        
        case GL_FLOAT_VEC4:
        {
            return sizeof(GLfloat)*4;
        } break;
        
        case GL_INT_VEC2:
        {
            return sizeof(GLint)*2;
        } break;
        
        case GL_INT_VEC3:
        {
            return sizeof(GLint)*3;
        } break;
        
        case GL_INT_VEC4:
        {
            return sizeof(GLint)*4;
        } break;
        
        case GL_UNSIGNED_INT_VEC2:
        {
            return sizeof(GLuint)*2;
        } break;
        
        case GL_UNSIGNED_INT_VEC3:
        {
            return sizeof(GLuint)*3;
        } break;
        
        case GL_UNSIGNED_INT_VEC4:
        {
            return sizeof(GLuint)*4;
        } break;
        
        case GL_FLOAT_MAT4:
        {
            return sizeof(GLfloat)*16;
        } break;

        default:
        {
            return 0;
        }
    }
}

struct opengl *
opengl_create_context(struct memory *memoryContext)
{
    struct opengl *context = memory_alloc(memoryContext, sizeof(struct opengl));
    memset(context, 0, sizeof(struct opengl));

    context->memoryContext = memoryContext;

    context->activeProgram = OPENGL_NULL_ID;
    context->boundVao = OPENGL_NULL_ID;

    for (int target = 0; target < OPENGL_BUFFER_TARGET_LENGTH; ++target)
    {
        context->bufferTargets[target].type = target;
        context->bufferTargets[target].buffer = OPENGL_NULL_ID;
    }

    return context;
}

void 
opengl_set_background_color(struct opengl *context, const real32 *v4)
{
    glClearColor(v4[0], v4[1], v4[2], v4[3]);
    memcpy(context->bgColor, v4, sizeof(context->bgColor));
}

struct opengl_program_info
opengl_load_program(struct opengl *context, const char *programName)
{
    struct opengl_program_info info = {0};
    UTILS_MUTABLE_CAST(opengl_id, info.program) = OPENGL_NULL_ID;

    struct opengl_program program = {0};

    i32 vertShaderStringCount = 0;
    char **vertShaderSource = NULL;
    i32 fragShaderStringCount = 0;
    char **fragShaderSource = NULL;

    char filePath[256];
    sprintf(filePath, "resources/shaders/%s.prog", programName);

    FILE *programFile = fopen(filePath, "r");
    if (!programFile)
    {
        fprintf(stderr, "opengl_load_program: Failure to load program '%s'. File not found.\n", programName);

        return info;
    }

    enum
    {
        VERT, FRAG
    } program_parse_mode;

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), programFile))
    {
        if (strstr(buffer, "@vert"))
        {
            program_parse_mode = VERT;
            continue;
        }
        else if (strstr(buffer, "@frag"))
        {
            program_parse_mode = FRAG;
            continue;
        }
        
        if (program_parse_mode == VERT)
        {
            if (vertShaderSource)
            {
                vertShaderSource = memory_realloc(context->memoryContext, vertShaderSource, sizeof(char *)*(++vertShaderStringCount));
            }
            else
            {
                vertShaderSource = memory_alloc(context->memoryContext, sizeof(char *)*(++vertShaderStringCount));
            }

            memcpy(vertShaderSource[vertShaderStringCount - 1] = memory_alloc(context->memoryContext, sizeof(buffer)), buffer, strlen(buffer) + 1);
        }       
        else if (program_parse_mode == FRAG)
        {
            if (fragShaderSource)
            {
                fragShaderSource = memory_realloc(context->memoryContext, fragShaderSource, sizeof(char *)*(++fragShaderStringCount));
            }
            else
            {
                fragShaderSource = memory_alloc(context->memoryContext, sizeof(char *)*(++fragShaderStringCount));
            }
            
            memcpy(fragShaderSource[fragShaderStringCount - 1] = memory_alloc(context->memoryContext, sizeof(buffer)), buffer, strlen(buffer) + 1);
        }
    }

    fclose(programFile);

    GLint success;

    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, vertShaderStringCount, vertShaderSource, NULL);
    glCompileShader(vertShader);
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        fprintf(stderr, 
            "opengl_load_program error: Could not compile vertex shader for program '%s'. Aborting.\n", 
            programName);
        
        glDeleteShader(vertShader);

        return info;
    }
    
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, fragShaderStringCount, fragShaderSource, NULL);
    glCompileShader(fragShader);
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        fprintf(stderr, 
            "opengl_load_program error: Could not compile fragment shader for program '%s'. Aborting.\n", 
            programName);

        glDeleteShader(vertShader);
        glDeleteShader(fragShader);

        return info;
    }

    program.glId = glCreateProgram();
    glAttachShader(program.glId, vertShader);
    glAttachShader(program.glId, fragShader);

    i32 anyAttribLocation = OPENGL_PROGRAM_ATTRIB_ANY;
    for (i32 i = 0; i < vertShaderStringCount; ++i)
    {
        const i32 bufferLength = strlen(vertShaderSource[i]) + 1;

        char *lineBuffer = memory_alloc(context->memoryContext, bufferLength);
        memcpy(lineBuffer, vertShaderSource[i], bufferLength);

        char *token = strtok(lineBuffer, " ");

        b32 isAttrib = B32_FALSE;
        b32 result = B32_FALSE;

        char *name = NULL;
        char *type = NULL;

        while (token)
        {
            if (!isAttrib)
            {
                if (!strcmp(token, "in"))
                {
                    isAttrib = B32_TRUE;
                }
            }

            if (isAttrib)
            {
                token = strtok(NULL, " ");

                if (token && !type)
                {
                    type = token;
                }
                else if (token && !name)
                {
                    name = token;
                    char *endOfStatement = strchr(name, ';');

                    if (endOfStatement)
                    {
                        *endOfStatement = '\0';
                    }
                }
                else
                {
                    break;
                }
            }
            else
            {
                break;
            }
        }

        if (isAttrib && name && type)
        {
            if (!strcmp(name, "a_Position"))
            {
                glBindAttribLocation(program.glId, OPENGL_PROGRAM_ATTRIB_POSITION, "a_Position");
            }
            else if (!strcmp(name, "a_Color"))
            {
                glBindAttribLocation(program.glId, OPENGL_PROGRAM_ATTRIB_COLOR, "a_Color");
            }
            else if (!strcmp(name, "a_Tex"))
            {
                glBindAttribLocation(program.glId, OPENGL_PROGRAM_ATTRIB_TEX, "a_Tex");
            }
            else
            {
                glBindAttribLocation(program.glId, anyAttribLocation++, name);
            }
        }

        memory_free(context->memoryContext, lineBuffer);
    }
    
    glLinkProgram(program.glId);

    GLint attribCount;
    glGetProgramiv(program.glId, GL_ACTIVE_ATTRIBUTES, &attribCount);
    
    struct opengl_attrib_info *attribInfo = memory_alloc(context->memoryContext, sizeof(struct opengl_attrib_info)*attribCount);
    memset((struct opengl_attrib_info *)attribInfo, 0, sizeof(struct opengl_attrib_info)*attribCount);
    UTILS_MUTABLE_CAST(i32, info.attribCount) = attribCount;
    program.attribs = memory_alloc(context->memoryContext, sizeof(struct opengl_attrib)*attribCount);
    memset(program.attribs, 0, sizeof(struct opengl_attrib)*attribCount);
    program.attribCount = attribCount;
    
    struct basic_dict *attribDict = DICTIONARY(context->memoryContext, NULL);

    i32 attribSetOffset = 0;
    for (i32 i = 0; i < attribCount; ++i)
    {
        char nameBuffer[256];
        GLsizei nameLength;
        glGetProgramResourceName(program.glId, GL_PROGRAM_INPUT, i, sizeof(nameBuffer), &nameLength, nameBuffer);

        attribInfo[i].name = memory_alloc(context->memoryContext, nameLength + 1);
        memcpy((char *)attribInfo[i].name, nameBuffer, nameLength + 1);

        const GLenum props[] = {GL_TYPE, GL_ARRAY_SIZE, GL_LOCATION};
        GLsizei propsResultLength;
        GLint propsResult[sizeof(props)];

        glGetProgramResourceiv(program.glId, GL_PROGRAM_INPUT, i, sizeof(props)/sizeof(props[0]), 
            props, sizeof(propsResult)/sizeof(propsResult[0]), &propsResultLength, propsResult);
        
        program.attribs[i].location = propsResult[2];
        UTILS_MUTABLE_CAST(opengl_id, attribInfo[i].attrib) = i;

        switch (propsResult[0])
        {
            case GL_FLOAT_VEC2:
            {
                UTILS_MUTABLE_CAST(u32, attribInfo[i].type) = GL_FLOAT;
                UTILS_MUTABLE_CAST(u32, attribInfo[i].typeLength) = 2;
            } break;
            
            case GL_FLOAT_VEC3:
            {
                UTILS_MUTABLE_CAST(u32, attribInfo[i].type) = GL_FLOAT;
                UTILS_MUTABLE_CAST(u32, attribInfo[i].typeLength) = 3;
            } break;
            
            case GL_FLOAT_VEC4:
            {
                UTILS_MUTABLE_CAST(u32, attribInfo[i].type) = GL_FLOAT;
                UTILS_MUTABLE_CAST(u32, attribInfo[i].typeLength) = 4;
            } break;
        }

        basic_dict_set(attribDict, context->memoryContext, attribInfo, attribInfo[i].name, nameLength + 1, &attribInfo[i]);
    }

    info.attribs = attribInfo;
    info.attribDict = attribDict;

    struct opengl_sort_context attribSortContext;
    attribSortContext.program = &program;
    attribSortContext.programInfo = &info;

    utils_sort(context->memoryContext, (struct opengl_attrib_info *)info.attribs, sizeof(struct opengl_attrib_info), info.attribCount, 
        __opengl_sort_attrib_info_compare_func, __opengl_sort_attrib_info_replace_func, &attribSortContext);

    utils_sort(context->memoryContext, (struct opengl_attrib *)program.attribs, sizeof(struct opengl_attrib), program.attribCount, 
        __opengl_sort_attrib_compare_func, __opengl_sort_attrib_replace_func, &attribSortContext);

    GLint uniformCount; 
    glGetProgramiv(program.glId, GL_ACTIVE_UNIFORMS, &uniformCount);

    struct opengl_uniform *uniforms = memory_alloc(context->memoryContext, sizeof(struct opengl_uniform)*uniformCount);
    memset(uniforms, 0, sizeof(struct opengl_uniform)*uniformCount);
    struct opengl_uniform_info *uniformInfo = memory_alloc(context->memoryContext, sizeof(struct opengl_uniform_info)*uniformCount);
    memset(uniformInfo, 0, sizeof(struct opengl_uniform_info)*uniformCount);
    struct basic_dict *uniformDict = DICTIONARY(context->memoryContext, NULL);

    GLuint *uniformIndices = memory_alloc(context->memoryContext, sizeof(GLuint)*uniformCount);
    for (u32 i = 0; i < uniformCount; ++i)
    {
        uniformIndices[i] = i;
    }

    GLint *uniformNameLengths = memory_alloc(context->memoryContext, sizeof(GLint)*uniformCount);
    glGetActiveUniformsiv(program.glId, uniformCount, uniformIndices, GL_UNIFORM_NAME_LENGTH, uniformNameLengths);

    for (i32 uniformIndex = 0; uniformIndex < uniformCount; ++uniformIndex)
    {
        GLint uniformLength = 0;
        GLenum uniformType = 0;
        char *uniformName = memory_alloc(context->memoryContext, uniformNameLengths[uniformIndex] + 1);

        glGetActiveUniform(program.glId, uniformIndex, uniformNameLengths[uniformIndex], NULL, &uniformLength, &uniformType, uniformName);
        
        if (uniformLength > 0)
        {
            uniforms[uniformIndex].type = uniformType;
            uniforms[uniformIndex].typeLength = uniformLength;
            uniforms[uniformIndex].location = glGetProgramResourceLocation(program.glId, GL_UNIFORM, uniformName);

            uniformInfo[uniformIndex].name = uniformName;
            UTILS_MUTABLE_CAST(u32, uniformInfo[uniformIndex].type) = uniformType;
            UTILS_MUTABLE_CAST(u32, uniformInfo[uniformIndex].typeLength) = uniformLength;
            UTILS_MUTABLE_CAST(u32, uniformInfo[uniformIndex].uniform) = uniformIndex;

            basic_dict_set(uniformDict, context->memoryContext, uniformInfo, uniformName, 
                uniformNameLengths[uniformIndex] + 1, &uniformInfo[uniformIndex]);
        }
        else
        {
            memory_free(context->memoryContext, uniformName);
        }
    }

    program.uniforms = uniforms;
    program.uniformCount = uniformCount;
    info.uniforms = uniformInfo;
    info.uniformDict = uniformDict;
    UTILS_MUTABLE_CAST(i32, info.uniformCount) = uniformCount;

    memory_free(context->memoryContext, uniformNameLengths);
    memory_free(context->memoryContext, uniformIndices);

    glDetachShader(program.glId, fragShader);
    glDetachShader(program.glId, vertShader);
    
    glDeleteShader(fragShader);
    glDeleteShader(vertShader);

    if (context->programs)
    {
        context->programs = memory_realloc(context->memoryContext, context->programs, 
            sizeof(struct opengl_program)*(++context->programCount));
    }
    else
    {
        context->programs = memory_alloc(context->memoryContext, sizeof(struct opengl_program)*(++context->programCount));
    }
    
    context->programs[context->programCount - 1] = program;

    info.name = programName;
    UTILS_MUTABLE_CAST(opengl_id, info.program) = context->programCount - 1;

    return info;
}

void 
opengl_use_program(struct opengl *context, opengl_id program)
{
    assert(program > OPENGL_NULL_ID && program < context->programCount);
    
    glUseProgram(context->programs[program].glId);
    context->activeProgram = program;
}

void 
opengl_assign_program_uniform(struct opengl *context, opengl_id program, opengl_id uniform, const void *data)
{
    assert(program == context->activeProgram);
    assert(program >= 0 && program < context->programCount);
    assert(uniform >= 0 && uniform < context->programs[program].uniformCount);

    const struct opengl_program *programPtr = &context->programs[program];
    
    if (data)
    {
        switch(programPtr->uniforms[uniform].type)
        {
            case GL_FLOAT:
            {
                if (programPtr->uniforms[uniform].typeLength == 1)
                {
                    glUniform1f(programPtr->uniforms[uniform].location, *(GLfloat *)data);
                }
                else
                {
                    glUniform1fv(programPtr->uniforms[uniform].location, programPtr->uniforms[uniform].typeLength,
                        data);
                }
            } break;
    
            case GL_INT:
            {
                if (programPtr->uniforms[uniform].typeLength == 1)
                {
                    glUniform1i(programPtr->uniforms[uniform].location, *(GLint *)data);
                }
                else
                {
                    glUniform1iv(programPtr->uniforms[uniform].location, programPtr->uniforms[uniform].typeLength,
                        data);
                }
            } break;
    
            case GL_UNSIGNED_INT:
            {
                if (programPtr->uniforms[uniform].typeLength == 1)
                {
                    glUniform1ui(programPtr->uniforms[uniform].location, *(GLuint *)data);
                }
                else
                {
                    glUniform1uiv(programPtr->uniforms[uniform].location, programPtr->uniforms[uniform].typeLength,
                        data);
                }
            } break;
    
            case GL_FLOAT_VEC2:
            {
                if (programPtr->uniforms[uniform].typeLength == 1)
                {
                    glUniform2f(programPtr->uniforms[uniform].location, 
                        ((GLfloat *)data)[0],
                        ((GLfloat *)data)[1]);
                }
                else
                {
                    glUniform2fv(programPtr->uniforms[uniform].location, programPtr->uniforms[uniform].typeLength,
                        data);
                }
            } break;
    
            case GL_FLOAT_VEC3:
            {
                if (programPtr->uniforms[uniform].typeLength == 1)
                {
                    glUniform3f(programPtr->uniforms[uniform].location, 
                        ((GLfloat *)data)[0],
                        ((GLfloat *)data)[1],
                        ((GLfloat *)data)[2]);
                }
                else
                {
                    glUniform3fv(programPtr->uniforms[uniform].location, programPtr->uniforms[uniform].typeLength,
                        data);
                }
            } break;
    
            case GL_FLOAT_VEC4:
            {
                if (programPtr->uniforms[uniform].typeLength == 1)
                {
                    glUniform4f(programPtr->uniforms[uniform].location, 
                        ((GLfloat *)data)[0],
                        ((GLfloat *)data)[1],
                        ((GLfloat *)data)[2],
                        ((GLfloat *)data)[3]);
                }
                else
                {
                    glUniform4fv(programPtr->uniforms[uniform].location, programPtr->uniforms[uniform].typeLength,
                        data);
                }
            } break;
    
            case GL_INT_VEC2:
            {
                if (programPtr->uniforms[uniform].typeLength == 1)
                {
                    glUniform2i(programPtr->uniforms[uniform].location, 
                        ((GLint *)data)[0],
                        ((GLint *)data)[1]);
                }
                else
                {
                    glUniform2iv(programPtr->uniforms[uniform].location, programPtr->uniforms[uniform].typeLength,
                        data);
                }
            } break;
    
            case GL_INT_VEC3:
            {
                if (programPtr->uniforms[uniform].typeLength == 1)
                {
                    glUniform3i(programPtr->uniforms[uniform].location, 
                        ((GLint *)data)[0],
                        ((GLint *)data)[1],
                        ((GLint *)data)[2]);
                }
                else
                {
                    glUniform3iv(programPtr->uniforms[uniform].location, programPtr->uniforms[uniform].typeLength,
                        data);
                }
            } break;
    
            case GL_INT_VEC4:
            {
                if (programPtr->uniforms[uniform].typeLength == 1)
                {
                    glUniform4i(programPtr->uniforms[uniform].location, 
                        ((GLint *)data)[0],
                        ((GLint *)data)[1],
                        ((GLint *)data)[2],
                        ((GLint *)data)[3]);
                }
                else
                {
                    glUniform4iv(programPtr->uniforms[uniform].location, programPtr->uniforms[uniform].typeLength,
                        data);
                }
            } break;
    
            case GL_UNSIGNED_INT_VEC2:
            {
                if (programPtr->uniforms[uniform].typeLength == 1)
                {
                    glUniform2ui(programPtr->uniforms[uniform].location, 
                        ((GLuint *)data)[0],
                        ((GLuint *)data)[1]);
                }
                else
                {
                    glUniform2uiv(programPtr->uniforms[uniform].location, programPtr->uniforms[uniform].typeLength,
                        data);
                }
            } break;
    
            case GL_UNSIGNED_INT_VEC3:
            {
                if (programPtr->uniforms[uniform].typeLength == 1)
                {
                    glUniform3ui(programPtr->uniforms[uniform].location, 
                        ((GLuint *)data)[0],
                        ((GLuint *)data)[1],
                        ((GLuint *)data)[2]);
                }
                else
                {
                    glUniform3uiv(programPtr->uniforms[uniform].location, programPtr->uniforms[uniform].typeLength,
                        data);
                }
            } break;
    
            case GL_UNSIGNED_INT_VEC4:
            {
                if (programPtr->uniforms[uniform].typeLength == 1)
                {
                    glUniform4ui(programPtr->uniforms[uniform].location, 
                        ((GLuint *)data)[0],
                        ((GLuint *)data)[1],
                        ((GLuint *)data)[2],
                        ((GLuint *)data)[3]);
                }
                else
                {
                    glUniform4uiv(programPtr->uniforms[uniform].location, programPtr->uniforms[uniform].typeLength,
                        data);
                }
            } break;
    
            case GL_FLOAT_MAT4:
            {
                glUniformMatrix4fv(programPtr->uniforms[uniform].location, programPtr->uniforms[uniform].typeLength,
                    GL_FALSE, data);
            } break;
        }
    }
    else
    {
        fprintf(stderr, "opengl_use_program error: Uniform not assigned. Data is NULL.\n");
    }
}

void
opengl_program_push_uniforms(struct opengl *context, opengl_id program)
{
	// TODO: maybe???
}

struct opengl_vao_info 
opengl_create_vao(struct opengl *context)
{
    GLuint vao;
    glCreateVertexArrays(1, &vao);

    struct opengl_vao_info info;

    if (context->vertexArrays)
    {
        context->vertexArrays = memory_realloc(context->memoryContext, context->vertexArrays, 
            sizeof(struct opengl_vao)*(++context->vaoCount));

        const i32 vaoIndex = context->vaoCount - 1;

        context->vertexArrays[vaoIndex].glId = vao;
        context->vertexArrays[vaoIndex].program = OPENGL_NULL_ID;
        context->vertexArrays[vaoIndex].vertexBuffer = OPENGL_NULL_ID;
        info.vao = vaoIndex;
    }
    else
    {
        context->vertexArrays = memory_alloc(context->memoryContext, sizeof(struct opengl_vao)*(++context->vaoCount));
        context->vertexArrays[0].glId = vao;
        context->vertexArrays[0].program = OPENGL_NULL_ID;
        context->vertexArrays[0].vertexBuffer = OPENGL_NULL_ID;
        info.vao = 0;
    }

    return info;
}

void 
opengl_bind_vao(struct opengl *context, opengl_id vao)
{
    assert(vao >= 0);

    glBindVertexArray(context->vertexArrays[vao].glId);
    context->boundVao = vao;
}

struct opengl_buffer_info 
opengl_create_buffer(struct opengl *context, enum opengl_buffer_target_type initTarget, 
    size_t size, void *data, b32 isDynamic)
{
    GLuint buffer;
    glGenBuffers(1, &buffer);
    
    const GLenum glTarget = __opengl_get_gl_target(initTarget);

    glBindBuffer(glTarget, buffer);

    glBufferData(glTarget, size, data, isDynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

    const i32 bufferIndex = context->bufferCount;
    if (context->buffers)
    {
        context->buffers = memory_realloc(context->memoryContext, context->buffers, 
            sizeof(struct opengl_buffer)*(++context->bufferCount));
    }
    else
    {
        context->buffers = memory_alloc(context->memoryContext, sizeof(struct opengl_buffer)*(++context->bufferCount));
    }
    
    context->buffers[bufferIndex].glId = buffer;
    context->bufferTargets[initTarget].buffer = bufferIndex;

    struct opengl_buffer_info info = {bufferIndex, size, isDynamic};

    return info;
}

void 
opengl_bind_buffer_target(struct opengl *context, opengl_id buffer, enum opengl_buffer_target_type target)
{
    assert(buffer >= OPENGL_NULL_ID);

    if (buffer >= 0)
    {
        glBindBuffer(__opengl_get_gl_target(target), context->buffers[buffer].glId);
    }
    else
    {
        glBindBuffer(__opengl_get_gl_target(target), 0);
    }
    
    context->bufferTargets[target].buffer = buffer;
}

void 
opengl_write_buffer_target(struct opengl *context, enum opengl_buffer_target_type target, 
    size_t offset, size_t size, void *data)
{
    glBufferSubData(__opengl_get_gl_target(target), offset, size, data);
}

struct opengl_texture_info
opengl_create_texture(struct opengl *context, GLenum internalFormat, opengl_id buffer, u32 width, u32 height)
{
    assert(buffer > OPENGL_NULL_ID && buffer < context->bufferCount);

    i32 texIndex = context->textureCount++;
    if (context->textures)
    {
        context->textures = memory_realloc(context->memoryContext, context->textures, 
            sizeof(struct opengl_texture)*context->textureCount);
    }
    else
    {
        context->textures = memory_alloc(context->memoryContext,
            sizeof(struct opengl_texture)*context->textureCount);
    }

    context->textures[texIndex].buffer = buffer;
    context->textures[texIndex].internalFormat = internalFormat;
    glGenTextures(1, &context->textures[texIndex].textureGLid);
    glGenSamplers(1, &context->textures[texIndex].samplerGLid);

    struct opengl_texture_info info;
    info.buffer = buffer;
    info.width = width;
    info.height = height;
    info.texture = texIndex;

    return info;
}

void
opengl_bind_texture(struct opengl *context, opengl_id texture)
{
    assert(texture > OPENGL_NULL_ID && texture < context->textureCount);

    glBindSampler(0, context->textures[texture].samplerGLid);
    glTexBuffer(GL_TEXTURE_BUFFER, context->textures[texture].internalFormat, context->buffers[context->textures[texture].buffer].glId);
}

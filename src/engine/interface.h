#ifndef _INTERFACE_H
#define _INTERFACE_H

#include "types.h"

struct memory;
struct basic_dict;

enum interface_element_pattern_type
{
    INTERFACE_ELEMENT_PATTERN_GRADIENT,
    INTERFACE_ELEMENT_PATTERN_TEXTURE,
    INTERFACE_ELEMENT_PATTERN_BORDER
};

enum interface_element_pattern_texture_mode
{
    INTERFACE_ELEMENT_PATTERN_TEXTURE_MODE_STRETCH,
    INTERFACE_ELEMENT_PATTERN_TEXTURE_MODE_CENTER,
    INTERFACE_ELEMENT_PATTERN_TEXTURE_MODE_REPEAT
};

struct interface_element_pattern
{
    enum interface_element_pattern_type type;

    union
    {
        struct
        {
            u32 bottomLeftColor;
            u32 bottomRightColor;
            u32 topRightColor;
            u32 topLeftColor;
        } gradient;

        struct
        {
            enum interface_element_pattern_textured_mode mode;
            u8 _padding[4];
            const char *textureName;
        } texture;

        struct
        {
            u32 color;
            u32 pixelThickness;
            u8 _padding[8];
        } border;
    };

    u8 _padding[4];
};

struct interface_element
{
    const char *name;
    real32 x;
    real32 y;
    real32 width;
    real32 height;
    u32 color;
    b32 isDirty;
    struct interface_element_pattern *patterns;
    i32 patternCount;
    i32 patternCapacity;
    struct interface_element *prev;
    struct interface_element *next;
};

struct interface
{
    struct
    {
        b32 isGrid;
        b32 isSnap;
    } flags;

    struct memory *_memoryContext;
    struct interface_element *_elements;
    i32 _elementCount;
    i32 _elementCapacity;
    struct basic_dict *_elementMap;
    struct interface_element *_displayStack;
    struct interface_element *_displayStackTop;
};

struct interface *
interface_init(struct memory *mem);

b32
interface_create_element(struct interface *context, const char *name,
    struct interface_element_pattern *patterns, i32 patternCount,
    real32 x, real32 y, real32 width, real32 height, u32 color);

void
interface_toggle_element(struct interface *context, const char *name, b32 b);

void
interface_target_element(struct interface *context, const char *name);

#endif
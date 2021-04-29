#ifndef __VEC4_H
#define __VEC4_H

#include "types.h"

struct vec4
{
    union
    {
        struct
        {
            real32 x;
            real32 y;
            real32 z;
            real32 w;
        };
        
        struct
        {
            real32 r;
            real32 g;
            real32 b;
            real32 a;
        };

        real32 _[4];
    };
};

#endif
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

void vec4_3_sum(struct vec4 *lhs, const struct vec4 *rhs);
void vec4_3_diff(struct vec4 *lhs, const struct vec4 *rhs);
void vec4_3_scalar(struct vec4 *lhs, real32 s);
real32 vec4_3_mag(const struct vec4 *v);
real32 vec4_3_dot(const struct vec4 *lhs, const struct vec4 *rhs);
struct vec4 vec4_3_normalized(const struct vec4 *v);

#endif

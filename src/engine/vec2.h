#ifndef __VEC2_H
#define __VEC2_H

#include "types.h"

struct vec2
{
    union
    {
        struct
        {
            real32 x;
            real32 y;
        };

        real32 _[2];
    };
};

void vec2_sum(struct vec2 *lhs, const struct vec2 *rhs);
void vec2_diff(struct vec2 *lhs, const struct vec2 *rhs);
void vec2_scalar(struct vec2 *lhs, real32 s);
real32 vec2_mag(const struct vec2 *v);
real32 vec2_dot(const struct vec2 *lhs, const struct vec2 *rhs);
struct vec2 vec2_normalized(const struct vec2 *v);

#endif

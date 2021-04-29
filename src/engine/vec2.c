#include "vec2.h"

#include <math.h>

void vec2_sum(struct vec2 *lhs, const struct vec2 *rhs)
{
    lhs->x += rhs->x;
    lhs->y += rhs->y;
}

void vec2_diff(struct vec2 *lhs, const struct vec2 *rhs)
{
    lhs->x -= rhs->x;
    lhs->y -= rhs->y;
}

void vec2_scalar(struct vec2 *lhs, real32 s)
{
    lhs->x *= s;
    lhs->y *= s;
}

real32 vec2_mag(const struct vec2 *v)
{
    return sqrt(v->x*v->x + v->y*v->y);
}

real32 vec2_dot(const struct vec2 *lhs, const struct vec2 *rhs)
{
    return lhs->x*lhs->y + rhs->x*rhs->y;
}

struct vec2 vec2_normalized(const struct vec2 *v)
{
    real32 mag = vec2_mag(v);

    struct vec2 result = {v->x/mag, v->y/mag};
    return result;
}
#include "vec4.h"

#include <math.h>

void vec4_3_sum(struct vec4 *lhs, const struct vec4 *rhs)
{
    lhs->x += rhs->x;
    lhs->y += rhs->y;
    lhs->z += rhs->z;
    lhs->w += rhs->w;
}

void vec4_3_diff(struct vec4 *lhs, const struct vec4 *rhs)
{
    lhs->x -= rhs->x;
    lhs->y -= rhs->y;
    lhs->z -= rhs->z;
    lhs->w -= rhs->w;
}

void vec4_3_scalar(struct vec4 *lhs, real32 s)
{
    lhs->x *= s;
    lhs->y *= s;
    lhs->z *= s;
    lhs->w *= s;
}

real32 vec4_3_mag(const struct vec4 *v)
{
    return sqrt(v->x*v->x +
				v->y*v->y +
				v->z*v->z +
				v->w*v->w);
}

real32 vec4_3_dot(const struct vec4 *lhs, const struct vec4 *rhs)
{
    return lhs->x*rhs->x +
			lhs->y*rhs->y +
			lhs->z*rhs->z +
			lhs->w*rhs->w;
}

struct vec4 vec4_3_normalized(const struct vec4 *v)
{
    real32 mag = vec4_3_mag(v);

    struct vec4 result = {v->x/mag, v->y/mag, v->z/mag, v->w/mag};
    return result;
}

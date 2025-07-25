#include "mat44_func.h" 
#include "mat44.h" 
#include "vec4.h"

#include <math.h>

struct mat44 mat44_func_ortho_proj(real32 left, real32 right, real32 bottom, 
    real32 top, real32 near, real32 far)
{
    struct mat44 m = {
        2.f/(right-left),                    0.f,                                 0.f,                             0.f,
        0.f,                                 2.f/(top - bottom),                  0.f,                             0.f,
        0.f,                                 0.f,                                 -2.f/(far-near),                 0.f,
        -((right + left)/(right - left)),    -((top + bottom)/(top - bottom)),    -((far + near)/(far - near)),    1.f
    };

    return m;
}

struct mat44 mat44_func_lookat(const struct vec4 *pos, const struct vec4 *dir, 
    const struct vec4 *r, const struct vec4 *u)
{
    struct mat44 m = {
        r->x, u->x, dir->x, 0.f,
        r->y, u->y, dir->y, 0.f,
        r->z, u->y, dir->z, 0.f,
        0.f, 0.f, 0.f, 1.f
    };

    struct mat44 p = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        -pos->x, -pos->y, -pos->z, 1.f
    };

    return mat44_mult(&m, &p);
}

struct mat44 mat44_func_translation(const struct vec4 *v)
{
    struct mat44 m = mat44_identity();

    m._3[0] = v->x;
    m._3[1] = v->y;
    m._3[2] = v->z;

    return m;
}

struct mat44 mat44_func_scale(const struct vec4 *v)
{
    struct mat44 m = {0};

    m._0[0] = v->x;
    m._1[1] = v->y;
    m._2[2] = v->z;
    m._3[3] = 1.f;

    return m;
}

struct mat44 mat44_func_angle_axis(const struct vec4 *axis, real32 theta)
{
    const real32 ct = cos(theta);
    const real32 diffCt = 1.f - ct;
    const real32 st = sin(theta);

    const struct mat44 m = {
        ct + axis->x*axis->x*diffCt, axis->x*axis->y*diffCt - axis->z*st,
            axis->x*axis->z*diffCt + axis->y*st, 0.f,
        axis->y*axis->x*diffCt + axis->z*st, ct + axis->y*axis->y*diffCt, 
            axis->y*axis->z*diffCt - axis->x*st, 0.f,
        axis->z*axis->x*diffCt - axis->y*st, axis->z*axis->y*diffCt + axis->x*st,
            ct + axis->z*axis->z*diffCt, 0.f,
        0.f, 0.f, 0.f, 1.f
    };

    return m;
}

struct mat44 mat44_func_pitch(real32 theta)
{
    const real32 ct = cos(theta);
    const real32 st = sin(theta);

    struct mat44 m = {0};

    m._0[0] = 1.f;
    m._1[1] = ct;
    m._1[2] = -st;
    m._2[1] = st;
    m._2[2] = ct;
    m._3[3] = 1.f;

    return m;
}

struct mat44 mat44_func_yaw(real32 theta)
{
    const real32 ct = cos(theta);
    const real32 st = sin(theta);

    struct mat44 m = {0};

    m._0[0] = ct;
    m._0[2] = st;
    m._1[1] = 1.f;
    m._2[0] = -st;
    m._2[2] = ct;
    m._3[3] = 1.f;

    return m;
}

struct mat44 mat44_func_roll(real32 theta)
{
    const real32 ct = cos(theta);
    const real32 st = sin(theta);

    struct mat44 m = {0};

    m._0[0] = ct;
    m._0[1] = -st;
    m._1[0] = st;
    m._1[1] = ct;
    m._2[2] = 1.f;
    m._3[3] = 1.f;

    return m;
}

struct vec4
mat44_func_multiply_vec4(const struct mat44 *lhsMatrix, const struct vec4 *rhsVector)
{
    struct vec4 result = {(lhsMatrix->_0[0]*rhsVector->x + lhsMatrix->_1[0]*rhsVector->y + 
        lhsMatrix->_2[0]*rhsVector->z + lhsMatrix->_3[0]*rhsVector->w),
        (lhsMatrix->_0[1]*rhsVector->x + lhsMatrix->_1[1]*rhsVector->y +
        lhsMatrix->_2[1]*rhsVector->z + lhsMatrix->_3[1]*rhsVector->w),
        (lhsMatrix->_0[2]*rhsVector->x + lhsMatrix->_1[2]*rhsVector->y +
        lhsMatrix->_2[2]*rhsVector->z + lhsMatrix->_3[2]*rhsVector->w),
        (lhsMatrix->_0[3]*rhsVector->x + lhsMatrix->_1[3]*rhsVector->y +
        lhsMatrix->_2[3]*rhsVector->z + lhsMatrix->_3[3]*rhsVector->w)};

    return result;
}
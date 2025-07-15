#ifndef __MAT44_FUNC_H
#define __MAT44_FUNC_H

#include "types.h"

struct mat44;
struct vec4;

struct mat44 mat44_func_ortho_proj(real32 left, real32 right, real32 bottom, 
    real32 top, real32 near, real32 far);

struct mat44 mat44_func_lookat(const struct vec4 *pos, const struct vec4 *dir, 
    const struct vec4 *r, const struct vec4 *u);

struct mat44 mat44_func_translation(const struct vec4 *v);

struct mat44 mat44_func_scale(const struct vec4 *v);

struct mat44 mat44_func_angle_axis(const struct vec4 *axis, real32 theta);

struct mat44 mat44_func_pitch(real32 theta);

struct mat44 mat44_func_yaw(real32 theta);

struct mat44 mat44_func_roll(real32 theta);

struct vec4
mat44_func_multiply_vec4(const struct mat44 *lhsMatrix, const struct vec4 *rhsVector);

#endif
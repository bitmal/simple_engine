#ifndef __MAT44__H
#define __MAT44__H

#include "types.h"

#define MAT44_LENGTH 16
#define MAT44_V_LENGTH 4

struct mat44
{
    union
    {
        struct
        {
            real32 _0[MAT44_V_LENGTH];
            real32 _1[MAT44_V_LENGTH];
            real32 _2[MAT44_V_LENGTH];
            real32 _3[MAT44_V_LENGTH];
        };

        real32 _[MAT44_LENGTH];
    };
};

struct mat44 mat44_identity();
struct mat44 mat44_scalar(const struct mat44 *lhs, real32 s);
struct mat44 mat44_sum(const struct mat44 *lhs, const struct mat44 *rhs);
struct mat44 mat44_mult(const struct mat44 *lhs, const struct mat44 *rhs);

#endif
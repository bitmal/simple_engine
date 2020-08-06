#include "mat44.h"
#include "types.h"

struct mat44 mat44_identity()
{
    struct mat44 m = {0};
    m._0[0] = 1.f;
    m._1[1] = 1.f;
    m._2[2] = 1.f;
    m._3[3] = 1.f;

    return m;
}

struct mat44 mat44_scalar(const struct mat44 *lhs, real32 s)
{
    struct mat44 m = *lhs;
    
    for (int i = 0; i < MAT44_LENGTH; ++i)
    {
        m._[i] *= s;
    }

    return m;
}

struct mat44 mat44_sum(const struct mat44 *lhs, const struct mat44 *rhs)
{
    struct mat44 m = *lhs;

    for (int i = 0; i < MAT44_LENGTH; ++i)
    {
        m._[i] += rhs->_[i];
    }

    return m;
}

struct mat44 mat44_mult(const struct mat44 *lhs, const struct mat44 *rhs)
{
    struct mat44 m = {
        lhs->_0[0]*rhs->_0[0] + lhs->_0[1]*rhs->_1[0] + lhs->_0[2]*rhs->_2[0] + lhs->_0[3]*rhs->_3[0],
            lhs->_0[0]*rhs->_0[1] + lhs->_0[1]*rhs->_1[1] + lhs->_0[2]*rhs->_2[1] + lhs->_0[3]*rhs->_3[1],
            lhs->_0[0]*rhs->_0[2] + lhs->_0[1]*rhs->_1[2] + lhs->_0[2]*rhs->_2[2] + lhs->_0[3]*rhs->_3[2],
            lhs->_0[0]*rhs->_0[3] + lhs->_0[1]*rhs->_1[3] + lhs->_0[2]*rhs->_2[3] + lhs->_0[3]*rhs->_3[3],
        lhs->_1[0]*rhs->_0[0] + lhs->_1[1]*rhs->_1[0] + lhs->_1[2]*rhs->_2[0] + lhs->_1[3]*rhs->_3[0],
            lhs->_1[0]*rhs->_0[1] + lhs->_1[1]*rhs->_1[1] + lhs->_1[2]*rhs->_2[1] + lhs->_1[3]*rhs->_3[1],
            lhs->_1[0]*rhs->_0[2] + lhs->_1[1]*rhs->_1[2] + lhs->_1[2]*rhs->_2[2] + lhs->_1[3]*rhs->_3[2],
            lhs->_1[0]*rhs->_0[3] + lhs->_1[1]*rhs->_1[3] + lhs->_1[2]*rhs->_2[3] + lhs->_1[3]*rhs->_3[3],
        lhs->_2[0]*rhs->_0[0] + lhs->_2[1]*rhs->_1[0] + lhs->_2[2]*rhs->_2[0] + lhs->_2[3]*rhs->_3[0],
            lhs->_2[0]*rhs->_0[1] + lhs->_2[1]*rhs->_1[1] + lhs->_2[2]*rhs->_2[1] + lhs->_2[3]*rhs->_3[1],
            lhs->_2[0]*rhs->_0[2] + lhs->_2[1]*rhs->_1[2] + lhs->_2[2]*rhs->_2[2] + lhs->_2[3]*rhs->_3[2],
            lhs->_2[0]*rhs->_0[3] + lhs->_2[1]*rhs->_1[3] + lhs->_2[2]*rhs->_2[3] + lhs->_2[3]*rhs->_3[3],
        lhs->_3[0]*rhs->_0[0] + lhs->_3[1]*rhs->_1[0] + lhs->_3[2]*rhs->_2[0] + lhs->_3[3]*rhs->_3[0],
            lhs->_3[0]*rhs->_0[1] + lhs->_3[1]*rhs->_1[1] + lhs->_3[2]*rhs->_2[1] + lhs->_3[3]*rhs->_3[1],
            lhs->_3[0]*rhs->_0[2] + lhs->_3[1]*rhs->_1[2] + lhs->_3[2]*rhs->_2[2] + lhs->_3[3]*rhs->_3[2],
            lhs->_3[0]*rhs->_0[3] + lhs->_3[1]*rhs->_1[3] + lhs->_3[2]*rhs->_2[3] + lhs->_3[3]*rhs->_3[3]
    };

    return m;
}
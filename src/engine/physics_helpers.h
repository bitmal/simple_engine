#ifndef __PHYSICS_HELPERS_H
#define __PHYSICS_HELPERS_H

#include "types.h"

inline void
physics_helpers_calculate_drag(real32 out[3], real32 coeff, real32 airDensity, real32 velocity[3], real32 refArea)
{
    out[0] = coeff*((airDensity*(velocity[0]*velocity[0])/2.f))*refArea;
    out[1] = coeff*((airDensity*(velocity[1]*velocity[1])/2.f))*refArea;
    out[2] = coeff*((airDensity*(velocity[2]*velocity[2])/2.f))*refArea;
}

#endif
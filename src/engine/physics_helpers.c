#include "physics_helpers.h"

#include <math.h>

void
physics_helpers_calculate_drag(real32 out[3], real32 coeff, real32 airDensity, real32 velocity[3], real32 refArea)
{
    out[0] = -((i32)roundf(fabsf(velocity[0])/velocity[0]))*.5f*airDensity*(velocity[0]*velocity[0])*coeff*refArea;
    out[1] = -((i32)roundf(fabsf(velocity[1])/velocity[1]))*.5f*airDensity*(velocity[1]*velocity[1])*coeff*refArea;
    out[2] = -((i32)roundf(fabsf(velocity[2])/velocity[2]))*.5f*airDensity*(velocity[2]*velocity[2])*coeff*refArea;
}
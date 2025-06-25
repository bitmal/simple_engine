#ifndef __PHYSICS_HELPERS_H
#define __PHYSICS_HELPERS_H

#include "types.h"

inline void
physics_helpers_calculate_drag(real32 out[3], real32 coeff, real32 airDensity, real32 velocity[3], real32 refArea);

#endif
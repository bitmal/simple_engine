#include "physics.h"
#include "physics_helpers.h"
#include "memory.h"
#include "basic_dict.h"
#include "types.h"

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <assert.h>

static u64
_physics_collision_hash_func(const struct basic_dict *dict, const void *key)
{
    u64 hash = *(u64 *)key;

    if (((u32 *)&hash)[0] > ((u32 *)&hash)[1])
    {
        u32 temp = ((u32 *)&hash)[0];
        ((u32 *)&hash)[0] = ((u32 *)&hash)[1];
        ((u32 *)&hash)[1] = temp;
    }

    return hash;
}

static u64
_physics_rigidbody_constraint_hash_func(const struct basic_dict *dict, const void *key)
{
    const physics_rigidbody_constraint_t *keyPtr = (const physics_rigidbody_constraint_t *)key;

    return (u64)(*keyPtr);
}

static b32
_physics_rigidbody_collider_is_overlap(struct physics *context, struct physics_rigidbody *lhs, struct physics_rigidbody *rhs)
{
    assert(lhs->collider > PHYSICS_NULL_ID && lhs->collider < context->colliderCount);
    assert(rhs->collider > PHYSICS_NULL_ID && rhs->collider < context->colliderCount);

    struct physics_collider *lhsColl = &context->colliders[lhs->collider];
    struct physics_collider *rhsColl = &context->colliders[rhs->collider];

    return (
        ((lhs->position[0]+lhsColl->bounds.left <= rhs->position[0]+rhsColl->bounds.right &&
         lhs->position[1]+lhsColl->bounds.top >= rhs->position[1]+rhsColl->bounds.bottom) &&
        (lhs->position[0]+lhsColl->bounds.left >= rhs->position[0]+rhsColl->bounds.left &&
         lhs->position[1]+lhsColl->bounds.top <= rhs->position[1]+rhsColl->bounds.top)) ||

        ((lhs->position[0]+lhsColl->bounds.left <= rhs->position[0]+rhsColl->bounds.right &&
         lhs->position[1]+lhsColl->bounds.bottom <= rhs->position[1]+rhsColl->bounds.top) &&
        (lhs->position[0]+lhsColl->bounds.left >= rhs->position[0]+rhsColl->bounds.left &&
         lhs->position[1]+lhsColl->bounds.bottom >= rhs->position[1]+rhsColl->bounds.bottom)) ||

        ((lhs->position[0]+lhsColl->bounds.right >= rhs->position[0]+rhsColl->bounds.left &&
         lhs->position[1]+lhsColl->bounds.bottom <= rhs->position[1]+rhsColl->bounds.top) &&
        (lhs->position[0]+lhsColl->bounds.right <= rhs->position[0]+rhsColl->bounds.right &&
         lhs->position[1]+lhsColl->bounds.bottom >= rhs->position[1]+rhsColl->bounds.bottom)) ||

        ((lhs->position[0]+lhsColl->bounds.right >= rhs->position[0]+rhsColl->bounds.left &&
         lhs->position[1]+lhsColl->bounds.top >= rhs->position[1]+rhsColl->bounds.bottom) &&
        (lhs->position[0]+lhsColl->bounds.right <= rhs->position[0]+rhsColl->bounds.right &&
         lhs->position[1]+lhsColl->bounds.top <= rhs->position[1]+rhsColl->bounds.top)) ||
        
        (lhs->position[0]+lhsColl->bounds.left < rhs->position[0]+rhsColl->bounds.left &&
         lhs->position[1]+lhsColl->bounds.top > rhs->position[1]+rhsColl->bounds.top &&
         lhs->position[0]+lhsColl->bounds.right > rhs->position[0]+rhsColl->bounds.right &&
         lhs->position[1]+lhsColl->bounds.bottom < rhs->position[1]+rhsColl->bounds.bottom));
}

struct physics *
physics_init(struct memory *mem)
{
    assert(mem);

    struct physics *p = memory_alloc(mem, sizeof(struct physics));
    memset(p, 0, sizeof(struct physics));

    p->memoryContext = mem;
    p->collisions = memory_alloc(mem, sizeof(struct physics_collision)*
        (p->collisionCapacity = PHYSICS_COLLISION_ARENA_BASE_SIZE));
    // TODO: dictionaries need to be able to self rezize buckets;
    // active collisions are currently capped because of this
    p->collisionMap = DICTIONARY(p->memoryContext, _physics_collision_hash_func);
    p->airDensity = PHYSICS_DEFAULT_AIR_DENSITY;
    p->gravity[0] = PHYSICS_DEFAULT_GRAVITY_X;
    p->gravity[1] = PHYSICS_DEFAULT_GRAVITY_Y;
    p->gravity[2] = PHYSICS_DEFAULT_GRAVITY_Z;

    return p;
}

physics_id
physics_create_rigidbody(struct physics *context)
{
    physics_id materialId = context->materialCount++;
    if (context->materialCount > 1)
    {
        context->materials = memory_realloc(context->memoryContext, context->materials, 
            sizeof(struct physics_material)*(context->materialCount));
    }
    else
    {
        context->materials = memory_alloc(context->memoryContext, sizeof(struct physics_material)*
            (context->materialCount));
    }

    struct physics_material *materialPtr = &context->materials[materialId];
    materialPtr->id = materialId;
    materialPtr->dragCoefficient = 0.f;
    materialPtr->frictionCoefficient = 0.f;
    
    physics_id colliderId = context->colliderCount++;
    if (context->colliderCount > 1)
    {
        context->colliders = memory_realloc(context->memoryContext, context->colliders, 
            sizeof(struct physics_collider)*(context->colliderCount));
    }
    else
    {
        context->colliders = memory_alloc(context->memoryContext, sizeof(struct physics_collider)*
            (context->colliderCount));
    }

    struct physics_collider *colliderPtr = &context->colliders[colliderId];
    colliderPtr->id = colliderId;
    colliderPtr->bounds.right = 1.f;
    colliderPtr->bounds.top = 1.f;
    colliderPtr->bounds.left = 0.f;
    colliderPtr->bounds.bottom = 0.f;
    colliderPtr->isTrigger = B32_TRUE;
    
    physics_id rbId = context->rigidbodyCount++;
    if (context->rigidbodyCount > 1)
    {
        context->rigidbodies = memory_realloc(context->memoryContext, context->rigidbodies, 
            sizeof(struct physics_rigidbody)*(context->rigidbodyCount));
    }
    else
    {
        context->rigidbodies = memory_alloc(context->memoryContext, sizeof(struct physics_rigidbody)*
            (context->rigidbodyCount));
    }

    struct physics_rigidbody *rbPtr = &context->rigidbodies[rbId];
    rbPtr->id = rbId;
    rbPtr->mass = PHYSICS_DEFAULT_MASS;
    rbPtr->isGravity = B32_FALSE;
    rbPtr->position[0] = 0.f;
    rbPtr->position[1] = 0.f;
    rbPtr->position[2] = 0.f;
    rbPtr->acceleration[0] = 0.f;
    rbPtr->acceleration[1] = 0.f;
    rbPtr->acceleration[2] = 0.f;
    rbPtr->velocity[0] = 0.f;
    rbPtr->velocity[1] = 0.f;
    rbPtr->velocity[2] = 0.f;
    rbPtr->rotation[0] = 0.f;
    rbPtr->rotation[1] = 0.f;
    rbPtr->rotation[2] = 0.f;
    rbPtr->rotationVelocity[0] = 0.f;
    rbPtr->rotationVelocity[1] = 0.f;
    rbPtr->rotationVelocity[2] = 0.f;
    rbPtr->rotationAcceleration[0] = 0.f;
    rbPtr->rotationAcceleration[1] = 0.f;
    rbPtr->rotationAcceleration[2] = 0.f;
    rbPtr->material = materialId;
    rbPtr->collider = colliderId;
    rbPtr->forcesCount = 0;
    rbPtr->forces = NULL;
    rbPtr->activeForces = NULL;
    rbPtr->activeForceCount = rbPtr->activeForceCapacity = 0;
    rbPtr->freeForces = NULL;
    rbPtr->freeForceCount = rbPtr->freeForceCapacity = 0;
    
    rbPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_SPEED].type = PHYSICS_RB_CONSTRAINT_MAX_SPEED;
    rbPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_SPEED].isActive = B32_FALSE;
    rbPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_SPEED].data = memory_alloc(context->memoryContext, sizeof(real32));
    *((real32 *)rbPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_SPEED].data) = 0.f;
    
    rbPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_ROTATION].type = PHYSICS_RB_CONSTRAINT_MAX_ROTATION;
    rbPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_ROTATION].isActive = B32_FALSE;
    rbPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_ROTATION].data = memory_alloc(context->memoryContext, sizeof(real32));
    *((real32 *)rbPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_ROTATION].data) = 0.f;

    return rbId;
}

void
physics_rigidbody_set_constraint(struct physics *context, physics_id rigidbody,
    physics_rigidbody_constraint_t type, const void *data)
{
    assert(data);

    struct physics_rigidbody *rbPtr = &context->rigidbodies[rigidbody];

    switch(type)
    {
        case PHYSICS_RB_CONSTRAINT_MAX_SPEED:
        {
            *((real32 *)rbPtr->constraintArr[type].data) = *((real32 *)data);
        } break;
        
        case PHYSICS_RB_CONSTRAINT_MAX_ROTATION:
        {
            *((real32 *)rbPtr->constraintArr[type].data) = *((real32 *)data);
        } break;

        default: break;
    }
}

void
physics_rigidbody_get_constraint(struct physics *context, physics_id rigidbody,
    physics_rigidbody_constraint_t type, void *outData)
{
    struct physics_rigidbody *rbPtr = &context->rigidbodies[rigidbody];

    switch(type)
    {
        case PHYSICS_RB_CONSTRAINT_MAX_SPEED:
        {
            *((real32 *)outData) = *((real32 *)rbPtr->constraintArr[type].data);
        } break;
        
        case PHYSICS_RB_CONSTRAINT_MAX_ROTATION:
        {
            *((real32 *)outData) = *((real32 *)rbPtr->constraintArr[type].data);
        } break;

        default: break;
    }
}

void
physics_rigidbody_set_constraint_active(struct physics *context, physics_id rigidbody,
    physics_rigidbody_constraint_t type, b32 isActive)
{
    context->rigidbodies[rigidbody].constraintArr[type].isActive = isActive;
}

b32
physics_rigidbody_get_constraint_active(struct physics *context, physics_id rigidbody,
    physics_rigidbody_constraint_t type)
{
    return context->rigidbodies[rigidbody].constraintArr[type].isActive;
}

void
physics_rigidbody_add_force(struct physics *context, physics_id rigidbody, 
                            real32 force[3], real32 duration)
{
    assert(rigidbody > PHYSICS_NULL_ID && rigidbody < context->rigidbodyCount);
    assert(duration >= 0.f);

    struct physics_rigidbody *rbPtr = &context->rigidbodies[rigidbody];

    i32 forceIndex;
    if (rbPtr->freeForceCount > 0)
    {
        forceIndex = rbPtr->freeForces[rbPtr->freeForceCount - 1];
        --rbPtr->freeForceCount;
    }
    else
    {
        forceIndex = rbPtr->forcesCount;

        if (rbPtr->forcesCount > 0)
        {
            rbPtr->forces = memory_realloc(context->memoryContext, rbPtr->forces, 
                sizeof(struct physics_force)*(++rbPtr->forcesCount));
        }
        else
        {
            rbPtr->forces = memory_alloc(context->memoryContext, 
                sizeof(struct physics_force)*(++rbPtr->forcesCount));
        }
    }

    struct physics_force *forcePtr = &rbPtr->forces[forceIndex];
    memcpy(forcePtr->force, force, sizeof(real32)*3);
    forcePtr->timestamp = context->timeSinceStart;
    forcePtr->duration = duration;

    if (rbPtr->activeForceCount >= rbPtr->activeForceCapacity)
    {
        if (rbPtr->activeForceCount > 0)
        {
            rbPtr->activeForces = memory_realloc(context->memoryContext, rbPtr->activeForces, 
                sizeof(i32)*(++rbPtr->activeForceCapacity));
        }
        else
        {
            rbPtr->activeForces = memory_alloc(context->memoryContext,
                sizeof(i32)*(++rbPtr->activeForceCapacity));
        }
    }

    rbPtr->activeForces[rbPtr->activeForceCount++] = forceIndex;
}

void
physics_update(struct physics *context, real32 timestep)
{
    for (i32 rbIndex = 0; rbIndex < context->rigidbodyCount; ++rbIndex)
    {
        struct physics_rigidbody *rbPtr = &context->rigidbodies[rbIndex];
        struct physics_material *materialPtr = &context->materials[rbPtr->material];
        struct physics_collider *colliderPtr = &context->colliders[rbPtr->collider];

        if (rbPtr->isGravity)
        {
            real32 gravityForce[3];
            gravityForce[0] = context->gravity[0]*rbPtr->mass;
            gravityForce[1] = context->gravity[1]*rbPtr->mass;
            gravityForce[2] = context->gravity[2]*rbPtr->mass;

            physics_rigidbody_add_force(context, rbPtr->id, gravityForce, 0.f);
        }
        
        for (i32 rhsIndex = 0; rhsIndex < context->rigidbodyCount; ++rhsIndex)
        {
            if (rbIndex == rhsIndex)
            {
                continue;
            }

            while (context->collisionCount > 0 && !_physics_rigidbody_collider_is_overlap(context, 
                   &context->rigidbodies[context->collisions[context->collisionCount - 1].rbLhs],
                   &context->rigidbodies[context->collisions[context->collisionCount - 1].rbRhs]))
            {
                u64 collHash;
                memcpy(&collHash, &context->collisions[context->collisionCount - 1].rbLhs, 
                    sizeof(physics_id));
                memcpy(&(((physics_id *)&collHash)[1]), &context->collisions[context->collisionCount - 1].rbRhs, 
                    sizeof(physics_id));

                basic_dict_set(context->collisionMap, context->memoryContext, 
                    context->collisions, &collHash, sizeof(collHash), NULL);

                if ((--context->collisionCount) == 0)
                {
                    context->collisions = memory_realloc(context->memoryContext, context->collisions,
                        sizeof(struct physics_collision)*
                        (context->collisionCapacity = PHYSICS_COLLISION_ARENA_BASE_SIZE));
                }
            } 
              
            struct physics_rigidbody *rhsPtr = &context->rigidbodies[rhsIndex];
            struct physics_material *rhsMaterial = &context->materials[rhsPtr->material];
            struct physics_collider *rhsColliderPtr = &context->colliders[rhsPtr->collider];

            u64 collHash;
            memcpy(&collHash, &rbIndex, sizeof(physics_id));
            memcpy(&(((u32 *)&collHash)[1]), &rhsIndex, sizeof(physics_id));
                
            if (!basic_dict_get(context->collisionMap, context->collisions, &collHash))
            {
                if (_physics_rigidbody_collider_is_overlap(context, rbPtr, rhsPtr))
                {
                    real32 rbForce[3] = {
                        rhsPtr->velocity[0] >= 0.f ?
                            ((rhsPtr->velocity[0]*rhsPtr->velocity[0])*rhsPtr->mass*0.5f) :
                            (-(rhsPtr->velocity[0]*rhsPtr->velocity[0])*rhsPtr->mass*0.5f),
                        rhsPtr->velocity[1] >= 0.f ?
                            ((rhsPtr->velocity[1]*rhsPtr->velocity[1])*rhsPtr->mass*0.5f) :
                            (-(rhsPtr->velocity[1]*rhsPtr->velocity[1])*rhsPtr->mass*0.5f),
                        rhsPtr->velocity[2] >= 0.f ?
                            ((rhsPtr->velocity[2]*rhsPtr->velocity[2])*rhsPtr->mass*0.5f) :
                            (-(rhsPtr->velocity[2]*rhsPtr->velocity[2])*rhsPtr->mass*0.5f)};
                    
                    rbForce[0] += rbPtr->velocity[0] >= 0.f ?
                        (-(rbPtr->velocity[0]*rbPtr->velocity[0])*rbPtr->mass*0.5f) :
                        ((rbPtr->velocity[0]*rbPtr->velocity[0])*rbPtr->mass*0.5f);
                    rbForce[1] += rbPtr->velocity[1] >= 0.f ?
                        (-(rbPtr->velocity[1]*rbPtr->velocity[1])*rbPtr->mass*0.5f) :
                        ((rbPtr->velocity[1]*rbPtr->velocity[1])*rbPtr->mass*0.5f);
                    rbForce[2] += rbPtr->velocity[2] >= 0.f ?
                        (-(rbPtr->velocity[2]*rbPtr->velocity[2])*rbPtr->mass*0.5f) :
                        ((rbPtr->velocity[2]*rbPtr->velocity[2])*rbPtr->mass*0.5f);
                    
                    physics_rigidbody_add_force(context, rbIndex, rbForce, 0.f);

                    real32 rhsForce[3] = {
                        rbPtr->velocity[0] >= 0.f ?
                            ((rbPtr->velocity[0]*rbPtr->velocity[0])*rbPtr->mass*0.5f) :
                            (-(rbPtr->velocity[0]*rbPtr->velocity[0])*rbPtr->mass*0.5f),
                        rbPtr->velocity[1] >= 0.f ?
                            ((rbPtr->velocity[1]*rbPtr->velocity[1])*rbPtr->mass*0.5f) :
                            (-(rbPtr->velocity[1]*rbPtr->velocity[1])*rbPtr->mass*0.5f),
                        rbPtr->velocity[2] >= 0.f ?
                            ((rbPtr->velocity[2]*rbPtr->velocity[2])*rbPtr->mass*0.5f) :
                            (-(rbPtr->velocity[2]*rbPtr->velocity[2])*rbPtr->mass*0.5f)};
                    
                    rhsForce[0] += rhsPtr->velocity[0] >= 0.f ?
                        (-(rhsPtr->velocity[0]*rhsPtr->velocity[0])*rhsPtr->mass*0.5f) :
                        ((rhsPtr->velocity[0]*rhsPtr->velocity[0])*rhsPtr->mass*0.5f);
                    rhsForce[1] += rhsPtr->velocity[1] >= 0.f ?
                        (-(rhsPtr->velocity[1]*rhsPtr->velocity[1])*rhsPtr->mass*0.5f) :
                        ((rhsPtr->velocity[1]*rhsPtr->velocity[1])*rhsPtr->mass*0.5f);
                    rhsForce[2] += rhsPtr->velocity[2] >= 0.f ?
                        (-(rhsPtr->velocity[2]*rhsPtr->velocity[2])*rhsPtr->mass*0.5f) :
                        ((rhsPtr->velocity[2]*rhsPtr->velocity[2])*rhsPtr->mass*0.5f);

                    physics_rigidbody_add_force(context, rhsIndex, rhsForce, 0.f);

                    i32 collisionIndex = context->collisionCount;
                    if (++context->collisionCount > context->collisionCapacity)
                    {
                        context->collisions = memory_realloc(context->memoryContext, context->collisions,
                            sizeof(struct physics_collision)*
                            (context->collisionCapacity *= PHYSICS_COLLISION_ARENA_ALLOC_MULTIPLIER));
                    }

                    //fprintf(stderr, "collision count: %d\n", context->collisionCount);
                    //assert(context->collisionCount < 1024);

                    context->collisions[collisionIndex].rbLhs = rbPtr->id;
                    context->collisions[collisionIndex].rbRhs = rhsPtr->id;
                    // context->collisions[collisionIndex].normal[0] = 
                    // context->collisions[collisionIndex].normal[1] = 
                    // context->collisions[collisionIndex].normal[2] = 

                    basic_dict_set(context->collisionMap, context->memoryContext, 
                        context->collisions, &collHash, sizeof(collHash), &context->collisions[collisionIndex]);
                }
            }
        }
        
        //TODO: refactor (at least sort forces whenever one is added, according to completion time)
        for (i32 forceIndex = rbPtr->activeForceCount - 1; forceIndex >= 0; --forceIndex)
        {
            if (rbPtr->activeForces[forceIndex] == PHYSICS_NULL_ID)
            {
                if (forceIndex == (rbPtr->activeForceCount - 1))
                {
                    --rbPtr->activeForceCount;
                    break;
                }
                else
                {
                    continue;
                }
            }

            struct physics_force *forcePtr = &rbPtr->forces[rbPtr->activeForces[forceIndex]];

            real32 timePassed = context->timeSinceStart - forcePtr->timestamp;

            if (timePassed > 0)
            {
                real32 forceMag = sqrt((forcePtr->force[0]*forcePtr->force[0]) +
                    (forcePtr->force[1]*forcePtr->force[1]) +
                    (forcePtr->force[2]*forcePtr->force[2]));
                real32 forceDir[3] = {forcePtr->force[0]/forceMag,
                    forcePtr->force[1]/forceMag,
                    forcePtr->force[2]/forceMag};
                real32 deltaForcePerSec = forceMag/forcePtr->duration;

                if (timePassed < forcePtr->duration)
                {
                    rbPtr->acceleration[0] += (forceDir[0]*deltaForcePerSec)/rbPtr->mass*timestep;
                    rbPtr->acceleration[1] += (forceDir[1]*deltaForcePerSec)/rbPtr->mass*timestep;
                    rbPtr->acceleration[2] += (forceDir[2]*deltaForcePerSec)/rbPtr->mass*timestep;
                }
                else
                {
                    real32 diffTime = forcePtr->duration - (timePassed - timestep);

                    if (diffTime > 0)
                    {
                        rbPtr->acceleration[0] += (forceDir[0]*deltaForcePerSec)/rbPtr->mass*diffTime;
                        rbPtr->acceleration[1] += (forceDir[1]*deltaForcePerSec)/rbPtr->mass*diffTime;
                        rbPtr->acceleration[2] += (forceDir[2]*deltaForcePerSec)/rbPtr->mass*diffTime;
                    }
                    else
                    {
                        rbPtr->acceleration[0] += (forceDir[0]*deltaForcePerSec)/rbPtr->mass;
                        rbPtr->acceleration[1] += (forceDir[1]*deltaForcePerSec)/rbPtr->mass;
                        rbPtr->acceleration[2] += (forceDir[2]*deltaForcePerSec)/rbPtr->mass;
                    }
                    
                    physics_id index = rbPtr->activeForces[forceIndex];
                    if (forceIndex >= (rbPtr->activeForceCount - 1))
                    {
                        --rbPtr->activeForceCount;
                    }
                    else
                    {
                        rbPtr->activeForces[forceIndex] = PHYSICS_NULL_ID;
                    }
                    
                    if (rbPtr->freeForceCount >= rbPtr->freeForceCapacity)
                    {
                        if (rbPtr->freeForceCapacity > 0)
                        {
                            rbPtr->freeForces = memory_realloc(context->memoryContext, rbPtr->freeForces,
                                sizeof(i32)*(++rbPtr->freeForceCapacity));
                        }
                        else
                        {
                            rbPtr->freeForces = memory_alloc(context->memoryContext, sizeof(i32)*(++rbPtr->freeForceCapacity));
                        }
                    }
                    
                    rbPtr->freeForces[rbPtr->freeForceCount++] = index;
                }
            }
            else if (forcePtr->duration == 0)
            {
                rbPtr->acceleration[0] += forcePtr->force[0]/rbPtr->mass;
                rbPtr->acceleration[1] += forcePtr->force[1]/rbPtr->mass;
                rbPtr->acceleration[2] += forcePtr->force[2]/rbPtr->mass;

                physics_id index = rbPtr->activeForces[forceIndex];
                if (forceIndex >= (rbPtr->activeForceCount - 1))
                {
                    --rbPtr->activeForceCount;
                }
                else
                {
                    rbPtr->activeForces[forceIndex] = PHYSICS_NULL_ID;
                }
                
                if (rbPtr->freeForceCount >= rbPtr->freeForceCapacity)
                {
                    if (rbPtr->freeForceCapacity > 0)
                    {
                        rbPtr->freeForces = memory_realloc(context->memoryContext, rbPtr->freeForces,
                            sizeof(i32)*(++rbPtr->freeForceCapacity));
                    }
                    else
                    {
                        rbPtr->freeForces = memory_alloc(context->memoryContext, sizeof(i32)*(++rbPtr->freeForceCapacity));
                    }
                }
                
                rbPtr->freeForces[rbPtr->freeForceCount++] = index;
            }
        }
        
        real32 drag[3];
        physics_helpers_calculate_drag(drag, materialPtr->dragCoefficient, context->airDensity, rbPtr->velocity, 
            ((colliderPtr->bounds.right - colliderPtr->bounds.left)*
            (colliderPtr->bounds.top - colliderPtr->bounds.bottom)));

        real32 magAccelSqr = rbPtr->acceleration[0]*rbPtr->acceleration[0] + 
            rbPtr->acceleration[1]*rbPtr->acceleration[1] +
            rbPtr->acceleration[2]*rbPtr->acceleration[2];
        real32 magDragSqr = drag[0]*drag[0] + drag[1]*drag[1] + drag[2]*drag[2];

        if ((magAccelSqr - magDragSqr*timestep) > 0)
        {
            rbPtr->acceleration[0] -= drag[0]*timestep;
            rbPtr->acceleration[1] -= drag[1]*timestep;
            rbPtr->acceleration[2] -= drag[2]*timestep;
        }
        else
        {
            rbPtr->acceleration[0] = 0.f;
            rbPtr->acceleration[1] = 0.f;
            rbPtr->acceleration[2] = 0.f;
        }

        rbPtr->velocity[0] += rbPtr->acceleration[0]*timestep;
        rbPtr->velocity[1] += rbPtr->acceleration[1]*timestep;
        rbPtr->velocity[2] += rbPtr->acceleration[2]*timestep;
        
        real32 velMag = sqrt(
            (rbPtr->velocity[0]*rbPtr->velocity[0]) +
            (rbPtr->velocity[1]*rbPtr->velocity[1]) +
            (rbPtr->velocity[2]*rbPtr->velocity[2]));

        if (velMag > 0.f)
        {
            real32 velDir[3] = {
                rbPtr->velocity[0]/velMag,
                rbPtr->velocity[1]/velMag,
                rbPtr->velocity[2]/velMag
            };
        
            real32 frictionForce[3] = {
                -materialPtr->frictionCoefficient*velDir[0],
                -materialPtr->frictionCoefficient*velDir[1],
                -materialPtr->frictionCoefficient*velDir[2]
            };

            if ((velMag - (materialPtr->frictionCoefficient*timestep)) > 0.f)
            {
                rbPtr->velocity[0] += frictionForce[0]*timestep;
                rbPtr->velocity[1] += frictionForce[1]*timestep;
                rbPtr->velocity[2] += frictionForce[2]*timestep;

                real32 accelMag = sqrt((rbPtr->acceleration[0]*rbPtr->acceleration[0]) +
                    (rbPtr->acceleration[1]*rbPtr->acceleration[1] +
                    (rbPtr->acceleration[2]*rbPtr->acceleration[2])));
                real32 accelNormalized[3] = {
                    rbPtr->acceleration[0]/accelMag,
                    rbPtr->acceleration[1]/accelMag,
                    rbPtr->acceleration[2]/accelMag
                };

                real32 velDot = (velDir[0]*accelNormalized[0]) + (velDir[1]*accelNormalized[1]) +
                    (velDir[2]*accelNormalized[2]);

                if (velDot >= 0.f)
                {
                    rbPtr->acceleration[0] += frictionForce[0]*timestep;
                    rbPtr->acceleration[1] += frictionForce[1]*timestep;
                    rbPtr->acceleration[2] += frictionForce[2]*timestep;
                }
                else
                {
                    rbPtr->acceleration[0] -= frictionForce[0]*timestep;
                    rbPtr->acceleration[1] -= frictionForce[1]*timestep;
                    rbPtr->acceleration[2] -= frictionForce[2]*timestep;
                }
            }
            else
            {
                rbPtr->velocity[0] = 0.f;
                rbPtr->velocity[1] = 0.f;
                rbPtr->velocity[2] = 0.f;
                
                rbPtr->acceleration[0] = 0.f;
                rbPtr->acceleration[1] = 0.f;
                rbPtr->acceleration[2] = 0.f;
            }
        }

        real32 magnitude = sqrtf(rbPtr->velocity[0]*rbPtr->velocity[0] + 
            rbPtr->velocity[1]*rbPtr->velocity[1] + 
            rbPtr->velocity[2]*rbPtr->velocity[2]);

        if (rbPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_SPEED].isActive)
        {
            if (magnitude > *((real32 *)rbPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_SPEED].data))
            {
                rbPtr->velocity[0] = (rbPtr->velocity[0]/magnitude)*
                    (*((real32 *)rbPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_SPEED].data));
                
               rbPtr->velocity[1] = (rbPtr->velocity[1]/magnitude)*
                    (*((real32 *)rbPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_SPEED].data));
               
               rbPtr->velocity[2] = (rbPtr->velocity[2]/magnitude)*
                    (*((real32 *)rbPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_SPEED].data));
            }
        }

        rbPtr->position[0] += rbPtr->velocity[0]*timestep;
        rbPtr->position[1] += rbPtr->velocity[1]*timestep;
        rbPtr->position[2] += rbPtr->velocity[2]*timestep;
    }
    
    context->timeSinceStart += timestep;
}

struct physics_rigidbody *
physics_get_rigidbody(struct physics *context, physics_id rb)
{
    assert(rb >= 0 && rb < context->rigidbodyCount);

    return &context->rigidbodies[rb];
}

struct physics_material *
physics_get_material(struct physics *context, physics_id material)
{
    assert(material >= 0 && material < context->materialCount);

    return &context->materials[material];
}

struct physics_collider *
physics_get_collider(struct physics *context, physics_id collider)
{
    assert(collider >= 0 && collider < context->colliderCount);

    return &context->colliders[collider];
}
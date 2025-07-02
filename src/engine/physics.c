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
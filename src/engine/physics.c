#include "physics.h"
#include "physics_helpers.h"
#include "memory.h"
#include "basic_dict.h"
#include "types.h"
#include "vec4.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
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

    printf("(colliderLhs: [%d] :: rbLhs [%d]), colliderRhs: [%d] :: rbRhs [%d])\n", lhsColl->id, lhs->id, rhsColl->id, rhs->id);
    assert(B32_FALSE);

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

static physics_id
_physics_rigidbody_alloc_force(struct physics *context, physics_id rigidbodyId)
{
    struct physics_rigidbody *rbPtr = &context->rigidbodies[rigidbodyId];

    i32 forceIndex = rbPtr->forceCapacity;

    if (rbPtr->forceCapacity > 0)
    {
        rbPtr->forceArr = (struct physics_force *)memory_realloc(context->memoryContext, rbPtr->forceArr, sizeof(struct physics_force)*
            ++rbPtr->forceCapacity);
    }
    else 
    {
        rbPtr->forceArr = (struct physics_force *)memory_alloc(context->memoryContext, sizeof(struct physics_force)*
            ++rbPtr->forceCapacity);
    }

    printf("force attempted to allocate: %d\n", rigidbodyId);

    if (!rbPtr->forceArr)
    {
        fprintf(stderr, "force failed to allocate: %d\n", rigidbodyId);

        return PHYSICS_NULL_ID;
    }

    struct physics_force *forcePtr = &rbPtr->forceArr[forceIndex];
    memset(forcePtr, (char)0, sizeof(struct physics_force));

    forcePtr->id = (physics_id)forceIndex;
    forcePtr->allocationTimestamp = context->timeSinceStart;

    return forcePtr->id;
}

static void
_physics_rigidbody_free_force_id(struct physics *context, physics_id rigidbodyId, struct physics_force_id *forceIdPtr)
{
    assert(rigidbodyId != PHYSICS_NULL_ID);
    assert(forceIdPtr);

    if (!forceIdPtr->isActive)
    {
        char messageBuffer[512];
        sprintf(messageBuffer, "_physics_rigidbody_free_force_id: Error freeing force id '%d'. Id is already inactive.", (i32)forceIdPtr->id);
        
        physics_log_queue_message(context, messageBuffer, 1);

        return;
    }

    struct physics_rigidbody *rbPtr = &context->rigidbodies[(i32)rigidbodyId];
    
    forceIdPtr->isActive = B32_FALSE;

    forceIdPtr->prev->next = forceIdPtr->next;
    forceIdPtr->next->prev = forceIdPtr->prev;

    if (forceIdPtr == rbPtr->activeForceIds)
    {
        if (rbPtr->activeForceIdCount > 1)
        {
            rbPtr->activeForceIds = rbPtr->activeForceIds->next;
        }
        else 
        {
            rbPtr->activeForceIds = NULL;
        }
    }

    --rbPtr->activeForceIdCount;

    if (rbPtr->freeForceIdCount > 0)
    {
        rbPtr->freeForceIds->prev->next = forceIdPtr;

        forceIdPtr->prev = rbPtr->freeForceIds->prev;
        forceIdPtr->next = rbPtr->freeForceIds;
    }
    else 
    {
        forceIdPtr->prev = forceIdPtr->next = forceIdPtr;
    }

    rbPtr->freeForceIds = forceIdPtr;
    ++rbPtr->freeForceIdCount;
}

static struct physics_force_id *
_physics_rigidbody_alloc_force_id(struct physics *context, physics_id rigidbodyId)
{
    assert(rigidbodyId != PHYSICS_NULL_ID);
    
    struct physics_rigidbody *rbPtr = &context->rigidbodies[rigidbodyId];

    struct physics_force_id *forceIdPtr;

    if (rbPtr->freeForceIdCount > 0)
    {
        forceIdPtr = rbPtr->freeForceIds;

        assert(rbPtr->freeForceIds);

        if (rbPtr->freeForceIds->next != rbPtr->freeForceIds)
        {
            rbPtr->freeForceIds->next->prev = rbPtr->freeForceIds->prev;
            rbPtr->freeForceIds->prev->next = rbPtr->freeForceIds->next;
            rbPtr->freeForceIds = rbPtr->freeForceIds->next;
        }

        forceIdPtr->isActive = B32_TRUE;

        --rbPtr->freeForceIdCount;
    }
    else 
    {
        forceIdPtr = (struct physics_force_id *)memory_alloc(context->memoryContext, sizeof(struct physics_force_id));

        assert(forceIdPtr);

        if (!forceIdPtr)
        {
            return NULL;
        }

        forceIdPtr->id = (physics_id)_physics_rigidbody_alloc_force(context, rigidbodyId);

        assert(forceIdPtr->id != PHYSICS_NULL_ID);
        
        forceIdPtr->isActive = B32_TRUE;
    }

    if (rbPtr->activeForceIdCount > 0)
    {
        assert(rbPtr->activeForceIds);

        rbPtr->activeForceIds->prev->next = forceIdPtr;

        forceIdPtr->prev = rbPtr->activeForceIds->prev;
        forceIdPtr->next = rbPtr->activeForceIds;
    }
    else 
    {
        forceIdPtr->prev = forceIdPtr->next = forceIdPtr;
    }
    
    rbPtr->activeForceIds = forceIdPtr;
    ++rbPtr->activeForceIdCount;

    return forceIdPtr;
}

static void
_physics_log_message_default_callback(struct physics *context, const struct physics_log_message *messagePtr)
{
    static FILE *logFile = NULL;

    if (!logFile)
    {
        if (!remove(context->log->outputFilename))
        {
            fprintf(stderr, "_physics_log_message_default_callback: Failure to remove logfile!");
        }
    }
    
    logFile = fopen(context->log->outputFilename, "a+");

    if (!logFile)
    {
        fprintf(stderr, "_physics_log_message_default_callback: Failure to open log file '%s' to append message contents.\n", 
            context->log->outputFilename);

        return;
    }

    fprintf(logFile, "%.2f: %s\n\n", messagePtr->startTimestamp, messagePtr->message);

    fclose(logFile);
}

struct physics *
physics_init(struct memory *mem)
{
    assert(mem);

    struct physics *p = memory_alloc(mem, sizeof(struct physics));
    memset(p, 0, sizeof(struct physics));

    p->memoryContext = mem;
    //p->collisions = memory_alloc(mem, sizeof(struct physics_collision)*
    //    (p->collisionCapacity = PHYSICS_COLLISION_ARENA_BASE_SIZE));
    // TODO: dictionaries need to be able to self rezize buckets;
    // active collisions are currently capped because of this
    p->collisionMap = DICTIONARY(p->memoryContext, _physics_collision_hash_func);
    p->airDensity = PHYSICS_DEFAULT_AIR_DENSITY;
    p->gravity[0] = PHYSICS_DEFAULT_GRAVITY_X;
    p->gravity[1] = PHYSICS_DEFAULT_GRAVITY_Y;
    p->gravity[2] = PHYSICS_DEFAULT_GRAVITY_Z;

    p->log = (struct physics_log *)memory_alloc(p->memoryContext, sizeof(struct physics_log));
    memset(p->log, 0, sizeof(struct physics_log));

    p->log->outputFilename = (char *)memory_alloc(p->memoryContext, sizeof("physics.log") + 1);
    
    sprintf(p->log->outputFilename, "physics.log");

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
    rbPtr->velocity[0] = 0.f;
    rbPtr->velocity[1] = 0.f;
    rbPtr->velocity[2] = 0.f;
    rbPtr->rotation[0] = 0.f;
    rbPtr->rotation[1] = 0.f;
    rbPtr->rotation[2] = 0.f;
    rbPtr->rotationVelocity[0] = 0.f;
    rbPtr->rotationVelocity[1] = 0.f;
    rbPtr->rotationVelocity[2] = 0.f;
    rbPtr->material = materialId;
    rbPtr->collider = colliderId;
    rbPtr->forceArr = NULL;
    rbPtr->forceCapacity = 0;
    rbPtr->forceIdArr = NULL;
    rbPtr->forceIdCapacity = 0;
    rbPtr->activeForceIdCount = 0;
    rbPtr->freeForceIdCount = 0;
    rbPtr->activeForceIds = NULL;
    rbPtr->freeForceIds = NULL;
    
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

const struct physics_force_id *
physics_rigidbody_add_force(struct physics *context, physics_id rigidbodyId, 
                            real32 direction[3], real32 magnitude, 
                            real32 duration)
{
    struct physics_rigidbody *rbPtr = &context->rigidbodies[rigidbodyId];

    struct physics_force_id *forceIdPtr = _physics_rigidbody_alloc_force_id(context, rigidbodyId);

    assert(forceIdPtr);

    struct physics_force *forcePtr = &rbPtr->forceArr[forceIdPtr->id];
    
    forcePtr->isActive = B32_TRUE;
    forcePtr->startTimestamp = forcePtr->currentTimestamp = context->timeSinceStart;
    forcePtr->direction[0] = direction[0];
    forcePtr->direction[1] = direction[1];
    forcePtr->direction[2] = direction[2];
    forcePtr->magnitude = fabsf(magnitude);
    forcePtr->duration = fabsf(duration);

    forceIdPtr->isActive = B32_TRUE;

    return forceIdPtr;
}

void
physics_rigidbody_force_set_active(struct physics *context, physics_id rigidbodyId, struct physics_force_id *forceIdPtr, b32 isActive)
{
    struct physics_rigidbody *rbPtr = &context->rigidbodies[(i32)rigidbodyId];
    struct physics_force *forcePtr = &rbPtr->forceArr[(i32)forceIdPtr->id];

    forcePtr->isActive = isActive;
}

const struct physics_force *
physics_rigidbody_get_force(struct physics *context, physics_id rigidbodyId, struct physics_force_id *forceIdPtr)
{
    struct physics_rigidbody *rbPtr = &context->rigidbodies[(i32)rigidbodyId];

    return &rbPtr->forceArr[(i32)forceIdPtr->id];
}

void
physics_update(struct physics *context, real32 timestep)
{
    for (i32 i = context->log->activeMessageCount; i > 0; ++i)
    {
        const struct physics_log_message_id *messageIdPtr;

        physics_log_pop_message(context, &messageIdPtr);
    }

    for (i32 rbIndex = 0; rbIndex < context->rigidbodyCount; ++rbIndex)
    {
        real32 accelerationDirection[] = {0.f, 0.f, 0.f};
        real32 accumulatedAccelerationMagnitude = 0.f;

        struct physics_rigidbody *rbPtr = &context->rigidbodies[rbIndex];
        //struct physics_force_id *forceIdPtr = rbPtr->activeForceIds;

        if (rbPtr->activeForceIdCount > 0) 
        {
            for (i32 counter = rbPtr->activeForceIdCount; counter > 0; --counter)
            {
                struct physics_force_id *forceIdPtr = rbPtr->activeForceIds;

                assert(forceIdPtr);
                assert(forceIdPtr->isActive);
                
                struct physics_force *forcePtr = &rbPtr->forceArr[(i32)forceIdPtr->id];

                real32 force[] = {
                    forcePtr->direction[0]*forcePtr->magnitude,
                    forcePtr->direction[1]*forcePtr->magnitude,
                    forcePtr->direction[2]*forcePtr->magnitude
                };

                assert(rbPtr->mass > 0.f);

                real32 acceleration[] = {force[0]/rbPtr->mass,
                    force[1]/rbPtr->mass,
                    force[2]/rbPtr->mass};

                real32 magnitude = sqrtf(force[0]*force[0] +
                        force[1]*force[1] +
                        force[2]*force[2]);
                
                accumulatedAccelerationMagnitude += (forcePtr->duration > 0.f) ? (magnitude*timestep) : magnitude;

                accelerationDirection[0] += acceleration[0]/magnitude;
                accelerationDirection[1] += acceleration[1]/magnitude;
                accelerationDirection[2] += acceleration[2]/magnitude;

                real32 magnitude1 = sqrtf(accelerationDirection[0]*accelerationDirection[0] +
                    accelerationDirection[1]*accelerationDirection[1] +
                    accelerationDirection[2]*accelerationDirection[2]);

                accelerationDirection[0] /= magnitude1;
                accelerationDirection[1] /= magnitude1;
                accelerationDirection[2] /= magnitude1;

                real32 deltaTime = (forcePtr->currentTimestamp + timestep) - forcePtr->startTimestamp;
                
                //real32 normalizedTime = (forcePtr->currentTimestamp - forcePtr->startTimestamp)/(forcePtr->duration > 0.f ? forcePtr->duration : 0.01f);
                
                if ((deltaTime + forcePtr->currentTimestamp - forcePtr->startTimestamp) >= forcePtr->duration)
                {
                    _physics_rigidbody_free_force_id(context, (physics_id)rbIndex, forceIdPtr);
                }
                
                forcePtr->currentTimestamp = context->timeSinceStart;
            }

            real32 resultVec[3];

            resultVec[0] = accelerationDirection[0]*accumulatedAccelerationMagnitude;
            resultVec[1] = accelerationDirection[1]*accumulatedAccelerationMagnitude;
            resultVec[2] = accelerationDirection[2]*accumulatedAccelerationMagnitude;

            rbPtr->velocity[0] += resultVec[0]*timestep;
            rbPtr->velocity[1] += resultVec[1]*timestep;
            rbPtr->velocity[2] += resultVec[2]*timestep;
        }
        
        struct physics_material *materialPtr = physics_get_material(context, rbPtr->material);
        struct physics_collider *colliderPtr = physics_get_collider(context, rbPtr->collider);

        //real32 velocityMagnitude = sqrtf(rbPtr->velocity[0]*rbPtr->velocity[0] +
        //    rbPtr->velocity[1]*rbPtr->velocity[1] +
        //    rbPtr->velocity[2]*rbPtr->velocity[2]);

        // apply drag
        real32 dragForce[3];
        physics_helpers_calculate_drag(dragForce, materialPtr->dragCoefficient, context->airDensity, rbPtr->velocity, 
            (colliderPtr->bounds.right - colliderPtr->bounds.left)*(colliderPtr->bounds.top - colliderPtr->bounds.bottom));

        real32 dragAcceleration[] = {dragForce[0]/rbPtr->mass, dragForce[1]/rbPtr->mass, dragForce[2]/rbPtr->mass};
        //real32 dragAccelerationMagnitude = sqrtf(dragAcceleration[0]*dragAcceleration[0] + dragAcceleration[1]*dragAcceleration[1] +
        //    dragAcceleration[2]*dragAcceleration[2]);
        
        rbPtr->velocity[0] += dragAcceleration[0]*timestep;
        rbPtr->velocity[1] += dragAcceleration[1]*timestep;
        rbPtr->velocity[2] += dragAcceleration[2]*timestep;

        // apply friction
        // TODO:

        rbPtr->rotation[0] += rbPtr->rotationVelocity[0]*timestep;
        rbPtr->rotation[1] += rbPtr->rotationVelocity[1]*timestep;
        rbPtr->rotation[2] += rbPtr->rotationVelocity[2]*timestep;

        const real64 phi = M_PI*2.0;

        if (rbPtr->rotation[0] > phi)
        {
            rbPtr->rotation[0] = 0.f;
        }
        else if (rbPtr->rotation[0] < 0.)
        {
            rbPtr->rotation[0] = phi;
        }
        if (rbPtr->rotation[1] > phi)
        {
            rbPtr->rotation[1] = 0.f;
        }
        else if (rbPtr->rotation[1] < 0.)
        {
            rbPtr->rotation[1] = phi;
        }
        if (rbPtr->rotation[2] > phi)
        {
            rbPtr->rotation[2] = 0.f;
        }
        else if (rbPtr->rotation[2] < 0.)
        {
            rbPtr->rotation[2] = phi;
        }

        rbPtr->position[0] += rbPtr->velocity[0]*timestep;
        rbPtr->position[1] += rbPtr->velocity[1]*timestep;
        rbPtr->position[2] += rbPtr->velocity[2]*timestep;

        for (i32 rbRhsIndex = 0; rbRhsIndex < context->rigidbodyCount; ++rbRhsIndex)
        {
            if (rbRhsIndex == rbIndex)
            {
                continue;
            }

            struct physics_rigidbody *rbRhsPtr = &context->rigidbodies[rbRhsIndex];

            if (_physics_rigidbody_collider_is_overlap(context, rbPtr, rbRhsPtr))
            {
                i32 collisionIndex = context->collisionCount;

                if (context->collisionCount == 0)
                {
                    context->collisions = memory_alloc(context->memoryContext, sizeof(struct physics_collision)*
                        (++context->collisionCount));
                }
                else if (context->collisionCount >= context->collisionCapacity)
                {
                    context->collisions = memory_realloc(context->memoryContext, context->collisions, 
                        sizeof(struct physics_collision)*(++context->collisionCount));
                }
                else 
                {
                    ++context->collisionCount;
                }

                //u64 hashKey = (((u64)rbPtr->id) << (sizeof(u32)*8)) | ((u64)rbRhsPtr->id);
                struct physics_collision *collisionPtr = &context->collisions[collisionIndex];

                collisionPtr->rbLhs = rbPtr->id;
                collisionPtr->rbRhs = rbRhsPtr->id;

                real32 rbLhsCollisionMagnitude = sqrtf(rbPtr->velocity[0]*rbPtr->velocity[0]*rbRhsPtr->mass +
                    rbPtr->velocity[1]*rbPtr->velocity[1]*rbRhsPtr->mass +
                    rbPtr->velocity[2]*rbPtr->velocity[2]*rbRhsPtr->mass);

                collisionPtr->normal[0] = rbPtr->velocity[0]/rbLhsCollisionMagnitude;
                collisionPtr->normal[1] = rbPtr->velocity[1]/rbLhsCollisionMagnitude;
                collisionPtr->normal[2] = rbPtr->velocity[2]/rbLhsCollisionMagnitude;

                //basic_dict_set(context->collisionMap, context->memoryContext, context->collisions, 
                //    &hashKey, sizeof(hashKey), collisionPtr);
            }
        }
    }
    
    for (i32 collisionIndex = 0; collisionIndex < context->collisionCount; ++collisionIndex)
    {
        struct physics_collision *collisionPtr = &context->collisions[collisionIndex];
        struct physics_rigidbody *rbLhsPtr = &context->rigidbodies[collisionPtr->rbLhs];
        struct physics_rigidbody *rbRhsPtr = &context->rigidbodies[collisionPtr->rbRhs];

        real32 rbLhsCollisionDirection[] = {-collisionPtr->normal[0], -collisionPtr->normal[1], -collisionPtr->normal[2]};

        real32 rbLhsMomentumMagnitude = sqrtf(rbLhsPtr->velocity[0]*rbLhsPtr->velocity[0]*rbLhsPtr->mass +
           rbLhsPtr->velocity[1]*rbLhsPtr->velocity[1]*rbLhsPtr->mass +
           rbLhsPtr->velocity[2]*rbLhsPtr->velocity[2]*rbLhsPtr->mass);

        physics_rigidbody_add_force(context, collisionPtr->rbLhs, rbLhsCollisionDirection, rbLhsMomentumMagnitude, 0.f);
    }
    
    for (struct physics_log_message_id *messageIdPtr = context->log->activeMessageIdQueue;
        messageIdPtr;)
    {
        //struct physics_log_message *messagePtr = &context->log->messageArr[(i32)messageIdPtr->id];

        //messageIdPtr = messageIdPtr->next != context->log->activeMessageIdQueue ? messageIdPtr->next : NULL;
        physics_log_pop_message(context, NULL);
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

struct physics_log_message_id *
physics_log_queue_message(struct physics *context, const char *message, u64 cycleCount)
{
    struct physics_log_message_id *messageIdPtr;

    if (context->log->freeMessageCount > 0)
    {
        messageIdPtr = context->log->freeMessageIds;
        --context->log->freeMessageCount;

        if (context->log->freeMessageCount > 0)
        {
            messageIdPtr->prev->next = messageIdPtr->next;
            messageIdPtr->next->prev = messageIdPtr->prev;

            context->log->freeMessageIds = messageIdPtr->next;
        }
        else 
        {
            context->log->freeMessageIds = NULL;
        }

        if (context->log->activeMessageCount > 0)
        {
            messageIdPtr->prev = context->log->activeMessageIdQueue;
            messageIdPtr->next = context->log->activeMessageIdQueue->next;
        }
        else 
        {
            messageIdPtr->prev = messageIdPtr->next = messageIdPtr;
        }

        context->log->activeMessageIdQueue->next->prev = messageIdPtr;
        context->log->activeMessageIdQueue->next = messageIdPtr;
        ++context->log->activeMessageCount;

        context->log->messageArr[(i32)messageIdPtr->id].startTimestamp = context->timeSinceStart;
        context->log->messageArr[(i32)messageIdPtr->id].outputCallback = _physics_log_message_default_callback;
        context->log->messageArr[(i32)messageIdPtr->id].message = (char *)memory_realloc(context->memoryContext, 
            context->log->messageArr[(i32)messageIdPtr->id].message, strlen(message) + 1);
        
        sprintf(context->log->messageArr[(i32)messageIdPtr->id].message, "%s\n", message);
    }
    else 
    {
        messageIdPtr = (struct physics_log_message_id *)memory_alloc(context->memoryContext, 
            sizeof(struct physics_log_message_id));

        i32 messageIndex = context->log->messageCapacity++;

        if (context->log->messageCapacity > 1)
        {
            context->log->messageArr = (struct physics_log_message *)memory_realloc(context->memoryContext, context->log->messageArr, 
                sizeof(struct physics_log_message)*context->log->messageCapacity);
        }
        else 
        {
            context->log->messageArr = (struct physics_log_message *)memory_alloc(context->memoryContext, 
                sizeof(struct physics_log_message)*context->log->messageCapacity);
        }

        if (context->log->activeMessageCount > 0)
        {
            messageIdPtr->prev = context->log->activeMessageIdQueue->prev;
            messageIdPtr->next = context->log->activeMessageIdQueue;
        }
        else 
        {
            messageIdPtr->prev = messageIdPtr->next = messageIdPtr;
        }
       
        context->log->activeMessageIdQueue = messageIdPtr;
        ++context->log->activeMessageCount;

        messageIdPtr->id = (physics_id)messageIndex;
        messageIdPtr->allocationTimestamp = context->timeSinceStart;
        messageIdPtr->startTimestamp = context->timeSinceStart;

        context->log->messageArr[messageIndex].id = (physics_id)messageIndex;
        context->log->messageArr[messageIndex].allocationTimestamp = context->timeSinceStart;
        context->log->messageArr[messageIndex].startTimestamp = context->timeSinceStart;
        context->log->messageArr[messageIndex].message = (char *)memory_alloc(context->memoryContext, strlen(message) + 1);
        context->log->messageArr[messageIndex].outputCallback = _physics_log_message_default_callback;

        sprintf(context->log->messageArr[messageIndex].message, "%s\n", message);
    }
    
    messageIdPtr->currentTimestamp = context->timeSinceStart;
    messageIdPtr->isActive = B32_TRUE;
    messageIdPtr->cycleCount = 0;
    messageIdPtr->maxCycles = cycleCount;

    if (!context->log->activeMessagePeekPtr)
    {
        context->log->activeMessagePeekPtr = context->log->activeMessageIdQueue;
    }

    return messageIdPtr;
}

void
physics_log_overwrite_message(struct physics *context, 
    struct physics_log_message_id *messageIdPtr, const char *message)
{
    assert(messageIdPtr->isActive);

    struct physics_log_message *messagePtr = &context->log->messageArr[(i32)messageIdPtr->id];

    messagePtr->message = (char *)memory_realloc(context->memoryContext, messagePtr->message, strlen(message) + 1);
    sprintf(messagePtr->message, "%s", message);
}

const struct physics_log_message *
physics_log_get_message(struct physics *context, struct physics_log_message_id *messageIdPtr)
{
    assert(messageIdPtr);

    return &context->log->messageArr[(i32)messageIdPtr->id];
}

struct physics_log_message_id *
physics_log_peek_message(struct physics *context)
{
    if (context->log->activeMessageCount > 0)
    {
        struct physics_log_message_id *messageIdPtr = context->log->activeMessageIdQueue;

        return messageIdPtr;
    }

    return NULL;
}

b32
physics_log_pop_message(struct physics *context, const struct physics_log_message_id **outputMessageIdPtr)
{
    if (context->log->activeMessageCount > 0)
    {
        struct physics_log_message_id *messageIdPtr = context->log->activeMessageIdQueue;
        struct physics_log_message *messagePtr = &context->log->messageArr[(i32)messageIdPtr->id];

        messagePtr->outputCallback(context, messagePtr);

        if (messageIdPtr->maxCycles > 0 && (++messageIdPtr->cycleCount >= messageIdPtr->maxCycles))
        {
            struct physics_log_message_id *messageIdPtr1 = messageIdPtr->next;

            if (messageIdPtr1 != context->log->activeMessageIdQueue)
            {
                messageIdPtr1->prev = messageIdPtr->prev;
            }
            
            messageIdPtr->prev->next = messageIdPtr1;

            if (context->log->freeMessageCount > 0)
            {
                messageIdPtr->prev = context->log->freeMessageIds->prev;
                messageIdPtr->next = context->log->freeMessageIds->next;
                context->log->freeMessageIds->prev->next = messageIdPtr;
                context->log->freeMessageIds->prev = messageIdPtr;
            }
            else 
            {
                messageIdPtr->prev = messageIdPtr->next = messageIdPtr;
            }

            context->log->freeMessageIds = messageIdPtr;

            --context->log->activeMessageCount;
            ++context->log->freeMessageCount;

            messageIdPtr->isActive = B32_FALSE;
        }

        if (outputMessageIdPtr)
        {
            *outputMessageIdPtr = messageIdPtr;
        }

        return B32_TRUE;
    }

    if (outputMessageIdPtr)
    {
        *outputMessageIdPtr = NULL;
    }

    return B32_FALSE;
}

b32
physics_log_free_message(struct physics *context, struct physics_log_message_id *messageIdPtr)
{
    // TODO: implementation
    
    return B32_FALSE;
}

struct physics_log_message_id *
physics_log_message_pointer_increment(struct physics *context)
{
    return (context->log->activeMessageCount > 0) ? (context->log->activeMessagePeekPtr = context->log->activeMessagePeekPtr->next) : NULL;
}

struct physics_log_message_id *
physics_log_message_pointer_decrement(struct physics *context)
{
    return (context->log->activeMessageCount > 0) ? (context->log->activeMessagePeekPtr = context->log->activeMessagePeekPtr->prev) : NULL;
}

struct physics_log_message_id *
physics_log_message_pointer_reset_position(struct physics *context)
{
    return (context->log->activeMessagePeekPtr = context->log->activeMessageIdQueue);
}

struct physics_log_message_id *
physics_log_message_pointer_set_position(struct physics *context, struct physics_log_message_id *rhsPtr)
{
    assert(rhsPtr->isActive);

    return (context->log->activeMessagePeekPtr = rhsPtr);
}
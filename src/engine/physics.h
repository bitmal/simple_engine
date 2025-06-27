#ifndef __PHYSICS_H
#define __PHYSICS_H

#include "types.h"

typedef i32 physics_id;
#define PHYSICS_NULL_ID -1

struct memory;
struct basic_dict;

#define PHYSICS_DEFAULT_MASS 1.f
#define PHYSICS_DEFAULT_AIR_DENSITY 0.2f
#define PHYSICS_DEFAULT_GRAVITY_X 0.f
#define PHYSICS_DEFAULT_GRAVITY_Y -9.72f
#define PHYSICS_DEFAULT_GRAVITY_Z 0.f

#define PHYSICS_COLLISION_ARENA_BASE_SIZE 5
#define PHYSICS_COLLISION_ARENA_ALLOC_MULTIPLIER 5 

struct physics_collider_bounds
{
    real32 right;
    real32 top;
    real32 left;
    real32 bottom;
};

struct physics_collider
{
    physics_id id;
    struct physics_collider_bounds bounds;
    b32 isTrigger;
};

struct physics_collision
{
    physics_id rbLhs;
    physics_id rbRhs;
    real32 normal[3];
};

struct physics_material
{
    physics_id id;
    real32 dragCoefficient;
    real32 frictionCoefficient;
};

struct physics_force
{
    real32 force[3];
    real32 timestamp;
    real32 duration;
};

typedef enum physics_rigidbody_constraint_type
{
    PHYSICS_RB_CONSTRAINT_MAX_SPEED,
    PHYSICS_RB_CONSTRAINT_MAX_ROTATION,
    PHYSICS_RB_CONSTRAINT_TYPE_COUNT
} physics_rigidbody_constraint_t;

struct physics_rigidbody_constraint
{
    physics_rigidbody_constraint_t type;
    void *data;
    b32 isActive;
};

struct physics_rigidbody
{
    physics_id id;
    physics_id material;
    physics_id collider;
    real32 position[3];
    real32 velocity[3];
    real32 acceleration[3];
    real32 rotation[3];
    real32 rotationVelocity[3];
    real32 rotationAcceleration[3];
    real32 mass;
    b32 isGravity;
    i32 forcesCount;
    struct physics_force *forces;
    i32 activeForceCount;
    i32 activeForceCapacity;
    i32 *activeForces;
    i32 freeForceCount;
    i32 freeForceCapacity;
    i32 *freeForces;
    struct physics_rigidbody_constraint constraintArr[PHYSICS_RB_CONSTRAINT_TYPE_COUNT];
};

struct physics
{
    struct memory *memoryContext;
    real32 timeSinceStart;
    real32 airDensity;
    real32 gravity[3];
    b32 isGravity;
    i32 materialCount;
    struct physics_material *materials;
    i32 colliderCount;
    struct physics_collider *colliders;
    i32 rigidbodyCount;
    struct physics_rigidbody *rigidbodies;
    i32 collisionCapacity;
    i32 collisionCount;
    struct physics_collision *collisions;
    struct basic_dict *collisionMap;
};

struct physics *
physics_init(struct memory *mem);

physics_id
physics_create_rigidbody(struct physics *context);

void
physics_rigidbody_set_constraint(struct physics *context, physics_id rigidbody,
    physics_rigidbody_constraint_t type, const void *data);

void
physics_rigidbody_get_constraint(struct physics *context, physics_id rigidbody,
    physics_rigidbody_constraint_t type, void *outData);

void
physics_rigidbody_set_constraint_active(struct physics *context, physics_id rigidbody,
    physics_rigidbody_constraint_t type, b32 isActive);

b32
physics_rigidbody_get_constraint_active(struct physics *context, physics_id rigidbody,
    physics_rigidbody_constraint_t type);

void
physics_rigidbody_add_force(struct physics *context, physics_id rigidbody, 
                            real32 force[3], real32 duration);

void
physics_update(struct physics *context, real32 timestep);

struct physics_rigidbody *
physics_get_rigidbody(struct physics *context, physics_id rb);

struct physics_material *
physics_get_material(struct physics *context, physics_id material);

struct physics_collider *
physics_get_collider(struct physics *context, physics_id collider);

#endif
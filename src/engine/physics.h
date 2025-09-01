#ifndef __PHYSICS_H
#define __PHYSICS_H

#include "types.h"
#include "memory.h"
#include "basic_dict.h"

typedef i32 physics_id;
typedef i64 physics_wide_id;
#define PHYSICS_NULL_ID -1

#define PHYSICS_DEFAULT_MASS 1.f
#define PHYSICS_DEFAULT_AIR_DENSITY 0.02f
#define PHYSICS_DEFAULT_GRAVITY_X 0.f
#define PHYSICS_DEFAULT_GRAVITY_Y -9.72f
#define PHYSICS_DEFAULT_GRAVITY_Z 0.f

#define PHYSICS_COLLISION_ARENA_BASE_LENGTH 0
#define PHYSICS_COLLISION_ARENA_LENGTH_REALLOC_MULTIPLIER 2

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
    //b32 isTrigger; // TODO: implement triggers vs solid objects rigidbody collisions
};

struct physics_collision
{
    physics_wide_id wideId;
    physics_id rbLhsId;
    physics_id rbRhsId;
    b32 isActive;
};

struct physics_material
{
    physics_id id;
    real32 dragCoefficient;
    real32 frictionCoefficient;
};

struct physics_force
{
    physics_wide_id allocationId;
    physics_id id;
    real32 direction[3];
    real32 magnitude;
    real32 allocationTimestamp;
    real32 startTimestamp;
    real32 currentTimestamp;
    real32 duration;
    b32 isActive;
};

struct physics_force_id
{
    physics_wide_id allocationId;
    physics_id id;
    b32 isActive;
    struct physics_force_id *prev;
    struct physics_force_id *next;
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
    real32 rotation[3];
    real32 rotationVelocity[3];
    real32 mass;
    b32 isGravity;
    b32 isKinematic;
    struct physics_force *forceArr;
    i32 forceCapacity;
    struct physics_force_id *forceIdArr;
    i32 forceIdCapacity;
    struct physics_force_id *activeForceIds;
    i32 activeForceIdCount;
    struct physics_force_id *freeForceIds;
    i32 freeForceIdCount;
    struct physics_rigidbody_constraint constraintArr[PHYSICS_RB_CONSTRAINT_TYPE_COUNT];
};

struct physics;
struct physics_log_message;
struct physics_log_message_id;

typedef void(*physics_log_message_callback)(struct physics *context, 
    const struct physics_log_message *messagePtr);
typedef physics_log_message_callback physics_log_message_cb_t;

struct physics_log_message
{
    physics_id id;
    char *message;
    void *data;
    real32 allocationTimestamp;
    real32 startTimestamp;
    physics_log_message_cb_t outputCallback;
};

struct physics_log_message_id
{
    physics_id id;
    b32 isActive;
    real32 allocationTimestamp;
    real32 startTimestamp;
    real32 currentTimestamp;
    u64 cycleCount;
    u64 maxCycles;
    struct physics_log_message_id *prev;
    struct physics_log_message_id *next;
};

struct physics_log
{
    char *outputFilename;
    struct physics_log_message *messageArr;
    i32 messageCapacity;
    struct physics_log_message_id *activeMessageIdQueue;
    i32 activeMessageCount;
    struct physics_log_message_id *freeMessageIds;
    i32 freeMessageCount;
    struct physics_log_message_id *activeMessagePeekPtr;
};

b32
physics_init(const struct memory_context_key *memoryKeyPtr, 
    const struct memory_allocation_key *outPhysicsKeyPtr);

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

const struct physics_force_id *
physics_rigidbody_add_force(struct physics *context, physics_id rigidbody, 
                            real32 direction[3], real32 magnitude, 
                            real32 duration);

void
physics_rigidbody_force_set_active(struct physics *context, physics_id rigidbodyId, 
    struct physics_force_id *forceIdPtr, b32 isActive);

const struct physics_force *
physics_rigidbody_get_force(struct physics *context, physics_id rigidbodyId, 
    struct physics_force_id *forceIdPtr);

void
physics_update(struct physics *context, real32 timestep);

struct physics_rigidbody *
physics_get_rigidbody(struct physics *context, physics_id rb);

struct physics_material *
physics_get_material(struct physics *context, physics_id material);

struct physics_collider *
physics_get_collider(struct physics *context, physics_id collider);

struct physics_log_message_id *
physics_log_queue_message(struct physics *context, const char *message, u64 cycleCount);

void
physics_log_overwrite_message(struct physics *context, 
    struct physics_log_message_id *messageIdPtr, const char *message);

const struct physics_log_message *
physics_log_get_message(struct physics *context, struct physics_log_message_id *messageIdPtr);

struct physics_log_message_id *
physics_log_peek_message(struct physics *context);

b32
physics_log_pop_message(struct physics *context, const struct physics_log_message_id **outputMessageIdPtr);

b32
physics_log_free_message(struct physics *context, struct physics_log_message_id *messageIdPtr);

struct physics_log_message_id *
physics_log_message_pointer_increment(struct physics *context);

struct physics_log_message_id *
physics_log_message_pointer_decrement(struct physics *context);

struct physics_log_message_id *
physics_log_message_pointer_reset_position(struct physics *context);

struct physics_log_message_id *
physics_log_message_pointer_set_position(struct physics *context, struct physics_log_message_id *rhsPtr);

#endif
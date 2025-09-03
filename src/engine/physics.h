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

#define PHYSICS_MESSAGE_ARENA_BASE_CAPACITY 100
#define PHYSICS_COLLISION_ARENA_BASE_CAPACITY 512

#define PHYSICS_RB_MAX_ACTIVE_FORCES 200

struct physics_collider_bounds
{
    real32 right;
    real32 top;
    real32 left;
    real32 bottom;
};

typedef enum physics_rigidbody_constraint_type
{
    PHYSICS_RB_CONSTRAINT_MAX_SPEED,
    PHYSICS_RB_CONSTRAINT_MAX_ROTATION,
    PHYSICS_RB_CONSTRAINT_TYPE_COUNT
} physics_rigidbody_constraint_t;

struct physics_log_message;

typedef void(*physics_log_message_callback)(struct physics *context, 
    const struct physics_log_message *messagePtr);
typedef physics_log_message_callback physics_log_message_cb_t;

struct physics_log_message
{
    physics_id id;
    const struct memory_raw_allocation_key rawMessageKey;
    const struct memory_allocation_key dataKey;
    real32 allocationTimestamp;
    physics_log_message_cb_t outputCallback;
};

b32
physics_init(const struct memory_context_key *memoryKeyPtr, 
    const struct memory_allocation_key *outPhysicsKeyPtr);

b32
physics_create_rigidbody(const struct memory_allocation_key *physicsKeyPtr, physics_id *outRbId);

b32
physics_rigidbody_set_constraint(const struct memory_allocation_key *physicsKeyPtr, physics_id rigidbodyId,
    physics_rigidbody_constraint_t type, void *dataPtr);

b32
physics_rigidbody_get_constraint(const struct memory_allocation_key *physicsKeyPtr, physics_id rigidbodyId,
    physics_rigidbody_constraint_t type, void *outDataPtr);

b32
physics_rigidbody_set_constraint_active(const struct memory_allocation_key physicsKeyPtr, 
    physics_id rigidbodyId, physics_rigidbody_constraint_t type, b32 isActive);

b32
physics_rigidbody_get_constraint_active(const struct memory_allocation_key *physicsKeyPtr, 
    physics_id rigidbodyId, physics_rigidbody_constraint_t type, b32 *outIsActive);

// TODO: customizable force functions with a default applied
b32
physics_rigidbody_apply_force(const struct memory_allocation_key *physicsKeyPtr, 
    physics_id rigidbodyId, real32 direction[3], real32 magnitude, real32 duration);

b32
physics_rigidbody_toggle_force(const struct memory_allocation_key *physicsKeyPtr, 
    physics_id rigidbodyId, physics_id forceId, b32 isMuted);

b32
physics_update(const struct memory_allocation_key *physicsKeyPtr, real32 timestep);

b32
physics_log_write_message(const struct memory_allocation_key *physicsKeyPtr, 
    const char *messageFmt, void *dataPtr, p64 dataByteOffset, u64 cycleCount, 
    physics_id *outMessageId);

void
physics_log_overwrite_message(const struct memory_allocation_key *physicsKeyPtr, 
    physics_id messageId, const char *messageFmt, void *dataPtr, p64 dataByteOffset);

b32
physics_log_map_message(const struct memory_allocation_key *physicsKeyPtr,
    physics_id messageId, const struct physics_log_message **outMessagePtr);

b32
physics_log_unmap_message(const struct memory_allocation_key *physicsKeyPtr,
    const struct physics_log_message **messagePtr);

b32
physics_log_read_message(const struct memory_allocation_key *physicsKeyPtr,
    struct physics_log_message *messagePtr);

#endif
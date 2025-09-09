#include "physics.h"
#include "constants.h"
#include "physics_helpers.h"
#include "memory.h"
#include "basic_dict.h"
#include "types.h"
#include "utils.h"
#include "basic_list.h"
#include "vec4.h"
#include "circular_buffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

struct physics_collider
{
    physics_id id;
    struct physics_collider_bounds bounds;
    u32 prevColliderIndex;
    u32 nextColliderIndex;
    b32 isActive;
    //b32 isTrigger; // TODO: implement triggers vs solid objects rigidbody collisions
};

struct physics_collision
{
    physics_id id;
    physics_id rbLhsId;
    physics_id rbRhsId;
};

struct physics_material
{
    physics_id id;
    real32 dragCoefficient;
    real32 frictionCoefficient;
    u32 prevMaterialIndex;
    u32 nextMaterialIndex;
    b32 isActive;
};

struct physics_force
{
    physics_id id;
    real32 direction[3];
    real32 magnitude;
    real32 startTimestamp;
    real32 duration;
    b32 isMuted;
};

struct physics_force_object
{
    physics_id rigidbodyId;
    struct physics_force forceData;
};

struct physics_rigidbody_constraint
{
    physics_rigidbody_constraint_t type;
    const struct memory_raw_allocation_key rawDataKey;
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
    const struct memory_allocation_key forcesArenaKey;
    struct physics_rigidbody_constraint constraintArr[PHYSICS_RB_CONSTRAINT_TYPE_COUNT];
    u64 prevRigidbodyIndex;
    u64 nextRigidbodyIndex;
    b32 isActive;
};

struct physics_log
{
    const struct memory_raw_allocation_key rawOutputFilenameKey;
    const struct memory_allocation_key messageArenaKey;
};

struct physics
{
    const struct memory_context_key memoryContextKey;
    const struct memory_page_key memoryContextPageKey;
    const struct memory_page_key memoryHeapPageKey;
    real32 startTimestamp;
    real32 airDensity;
    real32 gravity[3];
    b32 isGravity;
    u32 materialCapacity;
    const struct memory_allocation_key materialArrKey;
    u32 freeMaterialCount;
    u32 freeMaterialHeadIndex;
    u32 activeMaterialCount;
    u32 activeMaterialHeadIndex;
    u32 colliderCapacity;
    const struct memory_allocation_key colliderArrKey;
    u32 freeColliderCount;
    u32 freeColliderHeadIndex;
    u32 activeColliderCount;
    u32 activeColliderHeadIndex;
    const struct memory_allocation_key collisionArenaKey;
    const struct memory_allocation_key collisionDictKey;
    u32 rigidbodyCapacity;
    const struct memory_allocation_key rigidbodyArrKey;
    u32 freeRigidbodyCount;
    u32 freeRigidbodyHeadIndex;
    u32 activeRigidbodyCount;
    u32 activeRigidbodyHeadIndex;
    const struct memory_allocation_key activeForceListKey;
    const struct memory_allocation_key freeForceListKey;
    u32 activeForceCount;
    u32 freeForceCount;
    const struct memory_allocation_key forceDictKey;
    const struct memory_allocation_key physicsLogKey;
};

static i64 g_COLLISION_ID_COUNTER = 0;
static i64 g_FORCE_ALLOCATION_ID_COUNTER = 0;
static u32 g_MATERIAL_ALLOCATION_ID_COUNTER = 0;
static u32 g_COLLIDER_ALLOCATION_ID_COUNTER = 0;
static u32 g_RIGIDBODY_ALLOCATION_ID_COUNTER = 0;

b32
_physics_collision_hash_func(struct basic_dict *dictPtr, void *key,
    utils_hash *outHashPtr)
{
    const u64 *keyPtr = key;

    physics_id lhsId = *((physics_id *)keyPtr);
    physics_id rhsId = *((physics_id *)keyPtr + 1);

    assert(lhsId != rhsId);

    u64 resultHash;
    ((physics_id *)&resultHash)[0] = (lhsId > rhsId) ? lhsId : rhsId;
    ((physics_id *)&resultHash)[1] = (lhsId > rhsId) ? rhsId : lhsId;

    *outHashPtr = (utils_hash)resultHash;

    return B32_TRUE;
}

b32
_physics_create_key_copy_func(struct basic_dict *dictPtr, void *keyPtr, 
    const struct memory_raw_allocation_key *outRawKeyKeyPtr)
{
    const struct memory_raw_allocation_key rawKeyKey;
    u64 *dataPtr;
    {
        memory_error_code resultCode = memory_raw_alloc(&rawKeyKey, sizeof(u64));

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        resultCode = memory_map_raw_allocation(&rawKeyKey, (void **)&dataPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_raw_free(&rawKeyKey);
        }
    }

    memcpy(dataPtr, keyPtr, sizeof(u64));

    memory_unmap_raw_allocation(&rawKeyKey, (void **)&dataPtr);

    memcpy((void *)outRawKeyKeyPtr, &rawKeyKey, sizeof(struct memory_raw_allocation_key));

    return B32_TRUE;
}

b32
_physics_force_hash_func(struct basic_dict *dictPtr, void *keyPtr, utils_hash *outHashPtr)
{
    if (!dictPtr)
    {
        if (outHashPtr)
        {
            *outHashPtr = 0;
        }

        return B32_FALSE;
    }

    if (!keyPtr)
    {
        if (outHashPtr)
        {
            *outHashPtr = 0;
        }

        return B32_FALSE;
    }

    if (!outHashPtr)
    {
        return B32_FALSE;
    }

    i32 key = *(i32 *)keyPtr;

    key = ~key + (key << 15); // key = (key << 15) - key - 1;
    key = key ^ (key >> 12);
    key = key + (key << 2);
    key = key ^ (key >> 4);
    key = key * 2057; // key = (key + (key << 3)) + (key << 11);
    key = key ^ (key >> 16);

    *outHashPtr = (u64)(((real64)key/INT32_MAX)*(real64)(UINT64_MAX - 1)) + 1;

    return B32_TRUE;
}

b32
_physics_force_key_copy_func(struct basic_dict *dictPtr, void *keyPtr, 
    const struct memory_raw_allocation_key *outRawKeyKeyPtr)
{
    const struct memory_raw_allocation_key rawKeyKey;
    u64 *dataPtr;
    {
        memory_error_code resultCode = memory_raw_alloc(&rawKeyKey, 
        sizeof(physics_id));

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        resultCode = memory_map_raw_allocation(&rawKeyKey, (void **)&dataPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_raw_free(&rawKeyKey);
        }
    }

    memcpy(dataPtr, keyPtr, sizeof(physics_id));

    memory_unmap_raw_allocation(&rawKeyKey, (void **)&dataPtr);

    memcpy((void *)outRawKeyKeyPtr, &rawKeyKey, sizeof(struct memory_raw_allocation_key));

    return B32_TRUE;
}

static b32
_physics_free_material(const struct memory_allocation_key *physicsKeyPtr, physics_id materialId)
{
    if ((MEMORY_IS_ALLOCATION_NULL(physicsKeyPtr)))
    {
        return B32_FALSE;
    }

    if (materialId < 1)
    {
        return B32_FALSE;
    }

    struct physics *physicsPtr;
    struct physics_material *materialArrPtr;
    {
        memory_error_code resultCode = memory_map_alloc(physicsKeyPtr, (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        if (materialId >= physicsPtr->materialCapacity)
        {
            return B32_FALSE;
        }

        resultCode = memory_map_alloc(&physicsPtr->materialArrKey, (void **)&materialArrPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    struct physics_material *materialPtr = &materialArrPtr[materialId - 1];

    if (!materialPtr->isActive)
    {
        memory_unmap_alloc((void **)&materialArrPtr);
        memory_unmap_alloc((void **)&physicsPtr);

        return B32_TRUE;
    }

    materialPtr->isActive = B32_FALSE;

    if (physicsPtr->activeMaterialCount > 1)
    {
        materialArrPtr[materialPtr->prevMaterialIndex].nextMaterialIndex = materialPtr->nextMaterialIndex;
        materialArrPtr[materialPtr->nextMaterialIndex].prevMaterialIndex = materialPtr->prevMaterialIndex;

        if (materialId == (physicsPtr->activeMaterialHeadIndex + 1))
        {
            physicsPtr->activeMaterialHeadIndex = materialPtr->nextMaterialIndex;
        }
    }

    --physicsPtr->activeMaterialCount;

    if (physicsPtr->freeMaterialCount > 0)
    {
        materialPtr->prevMaterialIndex = materialArrPtr[physicsPtr->freeMaterialHeadIndex].prevMaterialIndex;
        materialPtr->nextMaterialIndex = materialArrPtr[physicsPtr->freeMaterialHeadIndex].nextMaterialIndex;

        materialArrPtr[materialArrPtr[physicsPtr->freeMaterialHeadIndex].prevMaterialIndex].nextMaterialIndex = materialId - 1;
        materialArrPtr[physicsPtr->freeMaterialHeadIndex].prevMaterialIndex = materialId - 1;
    }
    else 
    {
        materialPtr->prevMaterialIndex = materialPtr->nextMaterialIndex = materialId - 1;
    }

    physicsPtr->freeMaterialHeadIndex = materialId - 1;

    ++physicsPtr->freeMaterialCount;

    memory_unmap_alloc((void **)&materialArrPtr);
    memory_unmap_alloc((void **)&physicsPtr);

    return B32_TRUE;
}

static b32
_physics_activate_material(const struct memory_allocation_key *physicsKeyPtr, physics_id materialId)
{
    if ((MEMORY_IS_ALLOCATION_NULL(physicsKeyPtr)))
    {
        return B32_FALSE;
    }

    if (materialId < 1)
    {
        return B32_FALSE;
    }

    struct physics *physicsPtr;
    struct physics_material *materialArrPtr;
    {
        memory_error_code resultCode = memory_map_alloc(physicsKeyPtr, (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        if (materialId > physicsPtr->materialCapacity)
        {
            memory_unmap_alloc((void **)&physicsPtr);

            return B32_FALSE;
        }

        resultCode = memory_map_alloc(&physicsPtr->materialArrKey, (void **)&materialArrPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&physicsPtr);

            return B32_FALSE;
        }
    }

    struct physics_material *materialPtr = &materialArrPtr[materialId - 1];

    if (materialPtr->isActive)
    {
        memory_unmap_alloc((void **)&materialArrPtr);
        memory_unmap_alloc((void **)&physicsPtr);

        return B32_TRUE;
    }

    materialPtr->isActive = B32_TRUE;

    if (physicsPtr->freeMaterialCount > 1)
    {
        materialArrPtr[materialPtr->prevMaterialIndex].nextMaterialIndex = materialPtr->nextMaterialIndex;
        materialArrPtr[materialPtr->nextMaterialIndex].prevMaterialIndex = materialPtr->prevMaterialIndex;

        if (materialId == (physicsPtr->freeMaterialHeadIndex + 1))
        {
            physicsPtr->freeMaterialHeadIndex = materialPtr->nextMaterialIndex;
        }
    }

    --physicsPtr->freeMaterialCount;

    if (physicsPtr->activeMaterialCount > 0)
    {
        materialPtr->prevMaterialIndex = materialArrPtr[physicsPtr->activeMaterialHeadIndex].prevMaterialIndex;
        materialPtr->nextMaterialIndex = materialArrPtr[physicsPtr->activeMaterialHeadIndex].nextMaterialIndex;

        materialArrPtr[materialArrPtr[physicsPtr->activeMaterialHeadIndex].prevMaterialIndex].nextMaterialIndex = materialId - 1;
        materialArrPtr[physicsPtr->activeMaterialHeadIndex].prevMaterialIndex = materialId - 1;
    }
    else 
    {
        materialPtr->prevMaterialIndex = materialPtr->nextMaterialIndex = materialId - 1;
    }

    physicsPtr->activeMaterialHeadIndex = materialId - 1;

    ++physicsPtr->activeMaterialCount;

    memory_unmap_alloc((void **)&materialArrPtr);
    memory_unmap_alloc((void **)&physicsPtr);

    return B32_TRUE;
}

static b32
_physics_get_free_material_index(const struct memory_allocation_key *physicsKeyPtr, u32 *outFreeMaterialIndex)
{
    if ((MEMORY_IS_ALLOCATION_NULL(physicsKeyPtr)))
    {
        return B32_FALSE;
    }

    struct physics *physicsPtr;
    {
        memory_error_code resultCode = memory_map_alloc(physicsKeyPtr, (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    if (physicsPtr->freeMaterialCount < 1)
    {
        return B32_FALSE;
    }

    *outFreeMaterialIndex = physicsPtr->freeMaterialHeadIndex;
    
    memory_unmap_alloc((void **)&physicsPtr);

    return B32_TRUE;
}

static b32
_physics_free_collider(const struct memory_allocation_key *physicsKeyPtr, physics_id colliderId)
{
    if ((MEMORY_IS_ALLOCATION_NULL(physicsKeyPtr)))
    {
        return B32_FALSE;
    }

    if (colliderId < 1)
    {
        return B32_FALSE;
    }

    struct physics *physicsPtr;
    struct physics_collider *colliderArrPtr;
    {
        memory_error_code resultCode = memory_map_alloc(physicsKeyPtr, (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        if (colliderId > physicsPtr->colliderCapacity)
        {
            memory_unmap_alloc((void **)&physicsPtr);

            return B32_FALSE;
        }

        resultCode = memory_map_alloc(&physicsPtr->colliderArrKey, (void **)&colliderArrPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&physicsPtr);

            return B32_FALSE;
        }
    }

    struct physics_collider *colliderPtr = &colliderArrPtr[colliderId - 1];

    if (!colliderPtr->isActive)
    {
        memory_unmap_alloc((void **)&colliderArrPtr);
        memory_unmap_alloc((void **)&physicsPtr);

        return B32_TRUE;
    }

    colliderPtr->isActive = B32_FALSE;

    if (physicsPtr->activeMaterialCount > 1)
    {
        colliderArrPtr[colliderPtr->prevColliderIndex].nextColliderIndex = colliderPtr->nextColliderIndex;
        colliderArrPtr[colliderPtr->nextColliderIndex].prevColliderIndex = colliderPtr->prevColliderIndex;

        if (colliderId == (physicsPtr->activeColliderHeadIndex + 1))
        {
            physicsPtr->activeColliderHeadIndex = colliderPtr->nextColliderIndex;
        }
    }

    --physicsPtr->activeColliderCount;

    if (physicsPtr->freeColliderCount > 0)
    {
        colliderPtr->prevColliderIndex = colliderArrPtr[physicsPtr->freeColliderHeadIndex].prevColliderIndex;
        colliderPtr->nextColliderIndex = colliderArrPtr[physicsPtr->freeColliderHeadIndex].nextColliderIndex;

        colliderArrPtr[colliderArrPtr[physicsPtr->freeColliderHeadIndex].prevColliderIndex].nextColliderIndex = colliderId - 1;
        colliderArrPtr[physicsPtr->freeColliderHeadIndex].prevColliderIndex = colliderId - 1;
    }
    else 
    {
        colliderPtr->prevColliderIndex = colliderPtr->nextColliderIndex = colliderId - 1;
    }

    physicsPtr->freeColliderHeadIndex = colliderId - 1;

    ++physicsPtr->freeColliderCount;

    memory_unmap_alloc((void **)&colliderArrPtr);
    memory_unmap_alloc((void **)&physicsPtr);

    return B32_TRUE;
}

static b32
_physics_activate_collider(const struct memory_allocation_key *physicsKeyPtr, physics_id colliderId)
{
    if ((MEMORY_IS_ALLOCATION_NULL(physicsKeyPtr)))
    {
        return B32_FALSE;
    }

    if (colliderId < 1)
    {
        return B32_FALSE;
    }

    struct physics *physicsPtr;
    struct physics_collider *colliderArrPtr;
    {
        memory_error_code resultCode = memory_map_alloc(physicsKeyPtr, (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        if (colliderId > physicsPtr->colliderCapacity)
        {
            memory_unmap_alloc((void **)&physicsPtr);

            return B32_FALSE;
        }

        resultCode = memory_map_alloc(&physicsPtr->colliderArrKey, (void **)&colliderArrPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&physicsPtr);

            return B32_FALSE;
        }
    }

    struct physics_collider *colliderPtr = &colliderArrPtr[colliderId - 1];

    if (colliderPtr->isActive)
    {
        memory_unmap_alloc((void **)&colliderArrPtr);
        memory_unmap_alloc((void **)&physicsPtr);

        return B32_TRUE;
    }

    colliderPtr->isActive = B32_TRUE;

    if (physicsPtr->freeColliderCount > 1)
    {
        colliderArrPtr[colliderPtr->prevColliderIndex].nextColliderIndex = colliderPtr->nextColliderIndex;
        colliderArrPtr[colliderPtr->nextColliderIndex].prevColliderIndex = colliderPtr->prevColliderIndex;

        if (colliderId == (physicsPtr->freeColliderHeadIndex + 1))
        {
            physicsPtr->freeColliderHeadIndex = colliderPtr->nextColliderIndex;
        }
    }

    --physicsPtr->freeColliderCount;

    if (physicsPtr->activeColliderCount > 0)
    {
        colliderPtr->prevColliderIndex = colliderArrPtr[physicsPtr->activeColliderHeadIndex].prevColliderIndex;
        colliderPtr->nextColliderIndex = colliderArrPtr[physicsPtr->activeColliderHeadIndex].nextColliderIndex;

        colliderArrPtr[colliderArrPtr[physicsPtr->activeColliderHeadIndex].prevColliderIndex].nextColliderIndex = colliderId - 1;
        colliderArrPtr[physicsPtr->activeColliderHeadIndex].prevColliderIndex = colliderId - 1;
    }
    else 
    {
        colliderPtr->prevColliderIndex = colliderPtr->nextColliderIndex = colliderId - 1;
    }

    physicsPtr->activeColliderHeadIndex = colliderId - 1;

    ++physicsPtr->activeColliderCount;

    memory_unmap_alloc((void **)&colliderArrPtr);
    memory_unmap_alloc((void **)&physicsPtr);

    return B32_TRUE;
}

static b32
_physics_get_free_collider_index(const struct memory_allocation_key *physicsKeyPtr, u32 *outFreeColliderIndex)
{
    if ((MEMORY_IS_ALLOCATION_NULL(physicsKeyPtr)))
    {
        return B32_FALSE;
    }

    struct physics *physicsPtr;
    {
        memory_error_code resultCode = memory_map_alloc(physicsKeyPtr, (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    if (physicsPtr->freeColliderCount < 1)
    {
        return B32_FALSE;
    }

    *outFreeColliderIndex = physicsPtr->freeColliderHeadIndex;

    memory_unmap_alloc((void **)&physicsPtr);

    return B32_TRUE;
}

static b32
_physics_free_rigidbody(const struct memory_allocation_key *physicsKeyPtr, physics_id rigidbodyId)
{
    if ((MEMORY_IS_ALLOCATION_NULL(physicsKeyPtr)))
    {
        return B32_FALSE;
    }

    if (rigidbodyId < 1)
    {
        return B32_FALSE;
    }

    struct physics *physicsPtr;
    struct physics_rigidbody *rigidbodyArrPtr;
    {
        memory_error_code resultCode = memory_map_alloc(physicsKeyPtr, (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        if (rigidbodyId > physicsPtr->rigidbodyCapacity)
        {
            return B32_FALSE;
        }

        resultCode = memory_map_alloc(&physicsPtr->rigidbodyArrKey, (void **)&rigidbodyArrPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    struct physics_rigidbody *rigidbodyPtr = &rigidbodyArrPtr[rigidbodyId - 1];

    if (!rigidbodyPtr->isActive)
    {
        memory_unmap_alloc((void **)&rigidbodyArrPtr);
        memory_unmap_alloc((void **)&physicsPtr);

        return B32_TRUE;
    }

    rigidbodyPtr->isActive = B32_FALSE;

    if (physicsPtr->activeRigidbodyCount > 1)
    {
        rigidbodyArrPtr[rigidbodyPtr->prevRigidbodyIndex].nextRigidbodyIndex = rigidbodyPtr->nextRigidbodyIndex;
        rigidbodyArrPtr[rigidbodyPtr->nextRigidbodyIndex].prevRigidbodyIndex = rigidbodyPtr->prevRigidbodyIndex;

        if (rigidbodyId == (physicsPtr->activeRigidbodyHeadIndex + 1))
        {
            physicsPtr->activeRigidbodyHeadIndex = rigidbodyPtr->nextRigidbodyIndex;
        }
    }

    --physicsPtr->activeRigidbodyCount;

    if (physicsPtr->freeRigidbodyCount > 0)
    {
        rigidbodyPtr->prevRigidbodyIndex = rigidbodyArrPtr[physicsPtr->freeRigidbodyHeadIndex].prevRigidbodyIndex;
        rigidbodyPtr->nextRigidbodyIndex = rigidbodyArrPtr[physicsPtr->freeRigidbodyHeadIndex].nextRigidbodyIndex;

        rigidbodyArrPtr[rigidbodyArrPtr[physicsPtr->freeRigidbodyHeadIndex].prevRigidbodyIndex].nextRigidbodyIndex = rigidbodyId - 1;
        rigidbodyArrPtr[physicsPtr->freeRigidbodyHeadIndex].prevRigidbodyIndex = rigidbodyId - 1;
    }
    else 
    {
        rigidbodyPtr->prevRigidbodyIndex = rigidbodyPtr->nextRigidbodyIndex = rigidbodyId - 1;
    }

    physicsPtr->freeRigidbodyHeadIndex = rigidbodyId - 1;

    ++physicsPtr->freeRigidbodyCount;

    memory_unmap_alloc((void **)&rigidbodyArrPtr);
    memory_unmap_alloc((void **)&physicsPtr);

    return B32_TRUE;
}

static b32
_physics_activate_rigidbody(const struct memory_allocation_key *physicsKeyPtr, physics_id rigidbodyId)
{
    if ((MEMORY_IS_ALLOCATION_NULL(physicsKeyPtr)))
    {
        return B32_FALSE;
    }

    if (rigidbodyId < 1)
    {
        return B32_FALSE;
    }

    struct physics *physicsPtr;
    struct physics_rigidbody *rigidbodyArrPtr;
    {
        memory_error_code resultCode = memory_map_alloc(physicsKeyPtr, (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        if (rigidbodyId > physicsPtr->rigidbodyCapacity)
        {
            memory_unmap_alloc((void **)&physicsPtr);

            return B32_FALSE;
        }

        resultCode = memory_map_alloc(&physicsPtr->rigidbodyArrKey, (void **)&rigidbodyArrPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&physicsPtr);

            return B32_FALSE;
        }
    }

    struct physics_rigidbody *rigidbodyPtr = &rigidbodyArrPtr[rigidbodyId - 1];

    if (rigidbodyPtr->isActive)
    {
        memory_unmap_alloc((void **)&rigidbodyArrPtr);
        memory_unmap_alloc((void **)&physicsPtr);

        return B32_TRUE;
    }

    rigidbodyPtr->isActive = B32_TRUE;

    if (physicsPtr->freeRigidbodyCount > 1)
    {
        rigidbodyArrPtr[rigidbodyPtr->prevRigidbodyIndex].nextRigidbodyIndex = rigidbodyPtr->nextRigidbodyIndex;
        rigidbodyArrPtr[rigidbodyPtr->nextRigidbodyIndex].prevRigidbodyIndex = rigidbodyPtr->prevRigidbodyIndex;

        if (rigidbodyId == (physicsPtr->freeRigidbodyHeadIndex + 1))
        {
            physicsPtr->freeRigidbodyHeadIndex = rigidbodyPtr->nextRigidbodyIndex;
        }
    }

    --physicsPtr->freeColliderCount;

    if (physicsPtr->activeColliderCount > 0)
    {
        rigidbodyPtr->prevRigidbodyIndex = rigidbodyArrPtr[physicsPtr->activeRigidbodyHeadIndex].prevRigidbodyIndex;
        rigidbodyPtr->nextRigidbodyIndex = rigidbodyArrPtr[physicsPtr->activeRigidbodyHeadIndex].nextRigidbodyIndex;

        rigidbodyArrPtr[rigidbodyArrPtr[physicsPtr->activeRigidbodyHeadIndex].prevRigidbodyIndex].nextRigidbodyIndex = rigidbodyId - 1;
        rigidbodyArrPtr[physicsPtr->activeRigidbodyHeadIndex].prevRigidbodyIndex = rigidbodyId - 1;
    }
    else 
    {
        rigidbodyPtr->prevRigidbodyIndex = rigidbodyPtr->nextRigidbodyIndex = rigidbodyId - 1;
    }

    physicsPtr->activeRigidbodyHeadIndex = rigidbodyId - 1;

    ++physicsPtr->activeRigidbodyCount;

    memory_unmap_alloc((void **)&rigidbodyArrPtr);
    memory_unmap_alloc((void **)&physicsPtr);

    return B32_TRUE;
}

static b32
_physics_get_free_rigidbody_index(const struct memory_allocation_key *physicsKeyPtr, u32 *outFreeRigidbodyIndex)
{
    if ((MEMORY_IS_ALLOCATION_NULL(physicsKeyPtr)))
    {
        return B32_FALSE;
    }

    struct physics *physicsPtr;
    {
        memory_error_code resultCode = memory_map_alloc(physicsKeyPtr, (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    if (physicsPtr->freeRigidbodyCount < 1)
    {
        return B32_FALSE;
    }

    *outFreeRigidbodyIndex = physicsPtr->freeRigidbodyHeadIndex;

    memory_unmap_alloc((void **)&physicsPtr);

    return B32_TRUE;
}

static u64
_physics_rigidbody_constraint_hash_func(const struct basic_dict *dict, const void *key)
{
    const physics_rigidbody_constraint_t *keyPtr = (const physics_rigidbody_constraint_t *)key;

    return (u64)(*keyPtr);
}

static b32
_physics_rigidbody_collider_is_overlap(const struct memory_allocation_key *physicsKeyPtr, 
    physics_id lhsRigidbodyId, physics_id rhsRigidbodyId, b32 *outIsOverlap)
{
    if ((MEMORY_IS_ALLOCATION_NULL(physicsKeyPtr)))
    {
        return B32_FALSE;
    }

    if (!outIsOverlap)
    {
        return B32_FALSE;
    }

    struct physics *physicsPtr;
    struct physics_rigidbody *rigidbodyArrPtr;
    struct physics_collider *colliderArrPtr;
    {
        memory_error_code resultCode = memory_map_alloc(physicsKeyPtr, 
        (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        resultCode = memory_map_alloc(&physicsPtr->rigidbodyArrKey, 
        (void **)&rigidbodyArrPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&physicsPtr);

            return B32_FALSE;
        }
        
        resultCode = memory_map_alloc(&physicsPtr->colliderArrKey, 
        (void **)&colliderArrPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&rigidbodyArrPtr);
            memory_unmap_alloc((void **)&physicsPtr);

            return B32_FALSE;
        }
    }

    struct physics_rigidbody *lhsRigidbodyPtr = &rigidbodyArrPtr[lhsRigidbodyId - 1];
    struct physics_rigidbody *rhsRigidbodyPtr = &rigidbodyArrPtr[rhsRigidbodyId - 1];

    struct physics_collider *lhsColliderPtr = &colliderArrPtr[lhsRigidbodyPtr->collider - 1];
    struct physics_collider *rhsColliderPtr = &colliderArrPtr[rhsRigidbodyPtr->collider - 1];

    *outIsOverlap = (
        ((lhsRigidbodyPtr->position[0]+lhsColliderPtr->bounds.left <= rhsRigidbodyPtr->position[0]+
        rhsColliderPtr->bounds.right && lhsRigidbodyPtr->position[1]+
        lhsColliderPtr->bounds.top >= rhsRigidbodyPtr->position[1]+rhsColliderPtr->bounds.bottom) &&
        (lhsRigidbodyPtr->position[0]+lhsColliderPtr->bounds.left >= rhsRigidbodyPtr->position[0]+
        rhsColliderPtr->bounds.left && lhsRigidbodyPtr->position[1]+lhsColliderPtr->bounds.top <= 
        rhsRigidbodyPtr->position[1]+rhsColliderPtr->bounds.top)) ||

        ((lhsRigidbodyPtr->position[0]+lhsColliderPtr->bounds.left <= rhsRigidbodyPtr->position[0]+
        rhsColliderPtr->bounds.right && lhsRigidbodyPtr->position[1]+lhsColliderPtr->bounds.bottom <= 
        rhsRigidbodyPtr->position[1]+ rhsColliderPtr->bounds.top) && (lhsRigidbodyPtr->position[0]+
        lhsColliderPtr->bounds.left >= rhsRigidbodyPtr->position[0]+rhsColliderPtr->bounds.left &&
        lhsRigidbodyPtr->position[1]+lhsColliderPtr->bounds.bottom >= rhsRigidbodyPtr->position[1]+
        rhsColliderPtr->bounds.bottom)) ||

        ((lhsRigidbodyPtr->position[0]+lhsColliderPtr->bounds.right >= rhsRigidbodyPtr->position[0]+
        rhsColliderPtr->bounds.left &&
         lhsRigidbodyPtr->position[1]+lhsColliderPtr->bounds.bottom <= rhsRigidbodyPtr->position[1]+
        rhsColliderPtr->bounds.top) &&
        (lhsRigidbodyPtr->position[0]+lhsColliderPtr->bounds.right <= rhsRigidbodyPtr->position[0]+
        rhsColliderPtr->bounds.right && lhsRigidbodyPtr->position[1]+lhsColliderPtr->bounds.bottom >= 
        rhsRigidbodyPtr->position[1]+ rhsColliderPtr->bounds.bottom)) ||

        ((lhsRigidbodyPtr->position[0]+lhsColliderPtr->bounds.right >= rhsRigidbodyPtr->position[0]+
        rhsColliderPtr->bounds.left && lhsRigidbodyPtr->position[1]+lhsColliderPtr->bounds.top >= 
        rhsRigidbodyPtr->position[1]+rhsColliderPtr->bounds.bottom) && (lhsRigidbodyPtr->position[0]+
        lhsColliderPtr->bounds.right <= rhsRigidbodyPtr->position[0]+rhsColliderPtr->bounds.right &&
         lhsRigidbodyPtr->position[1]+lhsColliderPtr->bounds.top <= rhsRigidbodyPtr->position[1]+
        rhsColliderPtr->bounds.top)) ||
        
        (lhsRigidbodyPtr->position[0]+lhsColliderPtr->bounds.left < rhsRigidbodyPtr->position[0]+
        rhsColliderPtr->bounds.left && lhsRigidbodyPtr->position[1]+lhsColliderPtr->bounds.top > 
        rhsRigidbodyPtr->position[1]+rhsColliderPtr->bounds.top && lhsRigidbodyPtr->position[0]+
        lhsColliderPtr->bounds.right > rhsRigidbodyPtr->position[0]+rhsColliderPtr->bounds.right && 
        lhsRigidbodyPtr->position[1]+lhsColliderPtr->bounds.bottom < rhsRigidbodyPtr->position[1]+
        rhsColliderPtr->bounds.bottom));

    memory_unmap_alloc((void **)&colliderArrPtr);
    memory_unmap_alloc((void **)&rigidbodyArrPtr);
    memory_unmap_alloc((void **)&physicsPtr);

    return B32_TRUE;
}

static b32
_physics_rigidbody_alloc_force(const struct memory_allocation_key *physicsKeyPtr, 
    physics_id rigidbodyId, real32 direction[3], real32 magnitude, real32 duration)
{
    if ((MEMORY_IS_ALLOCATION_NULL(physicsKeyPtr)))
    {
        return B32_FALSE;
    }

    struct physics *physicsPtr;
    struct physics_rigidbody *rigidbodyArrPtr;
    {
        memory_error_code resultCode = memory_map_alloc(physicsKeyPtr, 
        (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        resultCode = memory_map_alloc(&physicsPtr->rigidbodyArrKey, 
        (void **)&rigidbodyArrPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&physicsPtr);

            return B32_FALSE;
        }
    }

    struct physics_rigidbody *rigidbodyPtr = &rigidbodyArrPtr[rigidbodyId - 1];

    {
        b32 isForcesArenaFull;

        if (!(circular_buffer_is_full(&rigidbodyPtr->forcesArenaKey, 
        &isForcesArenaFull)))
        {
            memory_unmap_alloc((void **)&rigidbodyArrPtr);
            memory_unmap_alloc((void **)&physicsPtr);

            return B32_FALSE;
        }

        if (isForcesArenaFull)
        {
            memory_unmap_alloc((void **)&rigidbodyArrPtr);
            memory_unmap_alloc((void **)&physicsPtr);

            return B32_FALSE;
        }
    }

    struct physics_force force;

    force.direction[0] = direction[0];
    force.direction[1] = direction[1];
    force.direction[2] = direction[2];

    force.magnitude = magnitude;
    force.duration = duration;
    force.id = ++g_FORCE_ALLOCATION_ID_COUNTER;
    force.startTimestamp = (real32)utils_get_elapsed_ms()/1000.f;

    circular_buffer_write_bytes(&rigidbodyPtr->forcesArenaKey, 
    sizeof(struct physics_force), (void *)&force);

    memory_unmap_alloc((void **)rigidbodyArrPtr);
    memory_unmap_alloc((void **)physicsPtr);

    return B32_TRUE;
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

b32
physics_init(const struct memory_context_key *memoryKeyPtr, 
    const struct memory_allocation_key *outPhysicsKeyPtr)
{
    const struct memory_page_key contextPage;
    const struct memory_page_key heapPage;
    {
        memory_error_code resultCode = memory_alloc_page(memoryKeyPtr, 
        sizeof(struct physics) + (memory_get_size_of_allocator()), &contextPage);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        resultCode = memory_alloc_page(memoryKeyPtr, 
            PHYSICS_MEMORY_SIZE - sizeof(struct physics) - (memory_get_size_of_allocator())
            - (memory_get_size_of_page_header())*2, &heapPage);

        if (resultCode != MEMORY_OK)
        {
            memory_free_page(&contextPage);

            return B32_FALSE;
        }
    }

    const struct memory_allocation_key physicsKey;
    {
        memory_error_code resultCode = memory_alloc(&contextPage, 
        sizeof(struct physics), NULL, &physicsKey);

        if (resultCode != MEMORY_OK)
        {
            memory_free_page(&heapPage);
            memory_free_page(&contextPage);

            return B32_FALSE;
        }
    }

    memory_set_alloc_offset_width(&physicsKey, 0, 
    sizeof(struct physics), '\0');

    struct physics *physicsPtr;
    {
        memory_error_code resultCode = memory_map_alloc(&physicsKey, 
        (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_free_page(&heapPage);
            memory_free_page(&contextPage);

            return B32_FALSE;
        }
    }

    memcpy((void *)&physicsPtr->memoryContextPageKey, &contextPage, sizeof(struct memory_page_key));
    memcpy((void *)&physicsPtr->memoryHeapPageKey, &heapPage, sizeof(struct memory_page_key));
    memcpy((void *)&physicsPtr->memoryContextKey, memoryKeyPtr, sizeof(struct memory_context_key));

    physicsPtr->airDensity = PHYSICS_DEFAULT_AIR_DENSITY;

    u64 keySize = sizeof(u64);

    if (!(basic_dict_create(&heapPage, &_physics_collision_hash_func, 
        &_physics_create_key_copy_func, utils_generate_next_prime_number(100), 
        &keySize, &physicsKey, &physicsPtr->collisionDictKey)))
    {
        memory_unmap_alloc((void **)&physicsPtr);

        memory_free_page(&heapPage);
        memory_free_page(&contextPage);
    }

    physicsPtr->gravity[0] = PHYSICS_DEFAULT_GRAVITY_X;
    physicsPtr->gravity[1] = PHYSICS_DEFAULT_GRAVITY_Y;
    physicsPtr->gravity[2] = PHYSICS_DEFAULT_GRAVITY_Z;
    physicsPtr->isGravity = B32_TRUE;

    keySize = sizeof(physics_id);

    if (!(basic_dict_create(&heapPage, &_physics_force_hash_func, 
        &_physics_force_key_copy_func, (
            utils_generate_next_prime_number(100)), &keySize, 
        &physicsKey, &physicsPtr->forceDictKey)))
    {
        memory_unmap_alloc((void **)&physicsPtr);
        
        memory_free_page(&heapPage);
        memory_free_page(&contextPage);

        return B32_FALSE;
    }

    if (!(basic_list_create(&heapPage, &physicsPtr->activeForceListKey)))
    {
        memory_unmap_alloc((void **)&physicsPtr);
        
        memory_free_page(&heapPage);
        memory_free_page(&contextPage);

        return B32_FALSE;
    }
    
    if (!(basic_list_create(&heapPage, &physicsPtr->freeForceListKey)))
    {
        memory_unmap_alloc((void **)&physicsPtr);
        
        memory_free_page(&heapPage);
        memory_free_page(&contextPage);

        return B32_FALSE;
    }

    if (!(circular_buffer_create(&heapPage, sizeof(struct physics_collision)*
        PHYSICS_COLLISION_ARENA_BASE_LENGTH, &physicsPtr->collisionArenaKey)))
    {
        memory_unmap_alloc((void **)&physicsPtr);

        memory_free_page(&heapPage);
        memory_free_page(&contextPage);

        return B32_FALSE;
    }

    {
        memory_error_code resultCode = memory_alloc(&heapPage, sizeof(struct physics_log), NULL,
            &physicsPtr->physicsLogKey);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&physicsPtr);

            memory_free_page(&heapPage);
            memory_free_page(&contextPage);

            return B32_FALSE;
        }

        memory_set_alloc_offset_width(&physicsPtr->physicsLogKey, 0, 
        sizeof(struct physics_log), '\0');

        struct physics_log *logPtr;

        resultCode = memory_map_alloc(&physicsPtr->physicsLogKey, (void **)&logPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&physicsPtr);

            memory_free_page(&heapPage);
            memory_free_page(&contextPage);

            return B32_FALSE;
        }

        if (!(circular_buffer_create(&heapPage, sizeof(struct physics_log_message)*
            PHYSICS_MESSAGE_ARENA_BASE_CAPACITY, &logPtr->messageArenaKey)))
        {
            memory_unmap_alloc((void **)&logPtr);
            memory_unmap_alloc((void **)&physicsPtr);

            memory_free_page(&heapPage);
            memory_free_page(&contextPage);

            return B32_FALSE;
        }

        resultCode = memory_raw_alloc(&logPtr->rawOutputFilenameKey, sizeof("physics.log"));

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&logPtr);
            memory_unmap_alloc((void **)&physicsPtr);

            memory_free_page(&heapPage);
            memory_free_page(&contextPage);

            return B32_FALSE;
        }

        char *logPath;

        resultCode = memory_map_raw_allocation(&logPtr->rawOutputFilenameKey, (void **)&logPath);

        if (resultCode != MEMORY_OK)
        {
            memory_raw_free(&logPtr->rawOutputFilenameKey);
            
            memory_unmap_alloc((void **)&logPtr);
            memory_unmap_alloc((void **)&physicsPtr);

            memory_free_page(&heapPage);
            memory_free_page(&contextPage);
        }

        strcpy(logPath, "physics.log");

        memory_unmap_raw_allocation(&logPtr->rawOutputFilenameKey, (void **)&logPath);
        memory_unmap_alloc((void **)&logPtr);
    }

    return B32_TRUE;
}

b32
physics_create_rigidbody(const struct memory_allocation_key *physicsKeyPtr, physics_id *outRbId)
{
    if ((MEMORY_IS_ALLOCATION_NULL(physicsKeyPtr)))
    {
        if (outRbId)
        {
            *outRbId = PHYSICS_NULL_ID;
        }

        return B32_FALSE;
    }

    struct physics *physicsPtr;

    physics_id materialId;
    u32 materialIndex;

    if (_physics_get_free_material_index(physicsKeyPtr, &materialIndex))
    {
        materialId = materialIndex + 1;

        if (!_physics_activate_material(physicsKeyPtr, materialId))
        {
            return B32_FALSE;
        }
    }
    else
    {
        memory_error_code resultCode = memory_map_alloc(physicsKeyPtr, (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        if (physicsPtr->materialCapacity > 0)
        {
            const struct memory_allocation_key tempKey;

            memory_error_code resultCode = memory_realloc(&physicsPtr->materialArrKey, sizeof(struct physics_material)*
                (physicsPtr->materialCapacity * 2), &tempKey);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&physicsPtr);

                return B32_FALSE;
            }
            
            u32 allocatedCount = physicsPtr->materialCapacity;

            memory_set_alloc_offset_width(&physicsPtr->materialArrKey, 
            physicsPtr->materialCapacity*sizeof(struct physics_material), sizeof(struct physics_material)*
                allocatedCount, '\0');

            memcpy((void *)&physicsPtr->materialArrKey, &tempKey, sizeof(struct memory_allocation_key));

            struct physics_material *materialArrPtr;

            resultCode = memory_map_alloc(&tempKey, (void **)&materialArrPtr);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&physicsPtr);

                return B32_FALSE;
            }

            for (u32 i = 0; i < allocatedCount; ++i)
            {
                u32 materialIndex = physicsPtr->materialCapacity + i;

                materialArrPtr[materialIndex].id = ++g_MATERIAL_ALLOCATION_ID_COUNTER;
                materialArrPtr[materialIndex].isActive = B32_FALSE;

                if (i > 0)
                {
                    materialArrPtr[materialIndex].prevMaterialIndex = materialIndex - 1;
                }

                if (i < (allocatedCount - 1))
                {
                    materialArrPtr[materialIndex].nextMaterialIndex = materialIndex + 1;
                }
            }

            if (physicsPtr->freeMaterialCount > 0)
            {
                materialArrPtr[physicsPtr->materialCapacity].prevMaterialIndex = materialArrPtr[physicsPtr->freeMaterialHeadIndex].prevMaterialIndex;
                materialArrPtr[physicsPtr->materialCapacity + allocatedCount - 1].nextMaterialIndex = physicsPtr->freeMaterialHeadIndex;

                materialArrPtr[materialArrPtr[physicsPtr->freeMaterialHeadIndex].prevMaterialIndex].nextMaterialIndex = physicsPtr->materialCapacity;
                materialArrPtr[physicsPtr->freeMaterialHeadIndex].prevMaterialIndex = physicsPtr->materialCapacity;
            }
            else 
            {
                materialArrPtr[physicsPtr->materialCapacity].prevMaterialIndex = physicsPtr->materialCapacity + allocatedCount - 1;
                materialArrPtr[physicsPtr->materialCapacity].nextMaterialIndex = physicsPtr->materialCapacity;
            }

            memory_unmap_alloc((void **)&materialArrPtr);

            physicsPtr->freeMaterialHeadIndex = physicsPtr->materialCapacity;

            materialId = physicsPtr->materialCapacity + 1;
            materialIndex = materialId - 1;

            physicsPtr->freeMaterialCount += allocatedCount - 1;
            physicsPtr->materialCapacity *= 2;

            memory_unmap_alloc((void **)&physicsPtr);
            
            if (!(_physics_activate_material(physicsKeyPtr, materialId)))
            {
                return B32_FALSE;
            }
        }
        else 
        {
            memory_error_code resultCode = memory_alloc(&physicsPtr->memoryHeapPageKey, 
                sizeof(struct physics_material)*(physicsPtr->materialCapacity + 1), NULL, 
                &physicsPtr->materialArrKey);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&physicsPtr);

                return B32_FALSE;
            }

            memory_set_alloc_offset_width(&physicsPtr->materialArrKey, 0, 
            sizeof(struct physics_material), '\0');

            struct physics_material *materialPtr;

            resultCode = memory_map_alloc(&physicsPtr->materialArrKey, (void **)&materialPtr);

            if (resultCode != MEMORY_OK)
            {
                memory_free(&physicsPtr->materialArrKey);

                memory_unmap_alloc((void **)&physicsPtr);

                return B32_FALSE;
            }

            materialPtr->id = ++g_MATERIAL_ALLOCATION_ID_COUNTER;
            
            materialId = materialPtr->id;
            materialIndex = materialPtr->id - 1;

            memory_unmap_alloc((void **)&materialPtr);

            ++physicsPtr->materialCapacity;

            memory_unmap_alloc((void **)&physicsPtr);
            
            if (!(_physics_activate_material(physicsKeyPtr, materialId)))
            {
                return B32_FALSE;
            }
        }
    }

    {
        memory_error_code resultCode = memory_map_alloc(physicsKeyPtr, (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            _physics_free_material(physicsKeyPtr, materialId);

            return B32_FALSE;
        }

        struct physics_material *materialArrPtr;

        resultCode = memory_map_alloc(&physicsPtr->materialArrKey, (void **)&materialArrPtr);

        materialArrPtr[materialIndex].dragCoefficient = 0.2f;
        materialArrPtr[materialIndex].frictionCoefficient = 0.2f;

        memory_unmap_alloc((void **)&materialArrPtr);
        memory_unmap_alloc((void **)&physicsPtr);
    }

    struct physics_collider *colliderArrPtr;
    physics_id colliderId;
    u32 colliderIndex;

    if ((_physics_get_free_collider_index(physicsKeyPtr, &colliderIndex)))
    {
        colliderId = colliderIndex + 1;

        if (!(_physics_activate_collider(physicsKeyPtr, colliderId)))
        {
            _physics_free_material(physicsKeyPtr, materialId);

            return B32_FALSE;
        }
    }
    else 
    {
        struct physics *physicsPtr;

        memory_error_code resultCode = memory_map_alloc(physicsKeyPtr, (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            _physics_free_material(physicsKeyPtr, materialId);

            return B32_FALSE;
        }

        if (physicsPtr->colliderCapacity > 0)
        {
            const struct memory_allocation_key tempKey;

            memory_error_code resultCode = memory_realloc(&physicsPtr->colliderArrKey, sizeof(struct physics_collider)*
                physicsPtr->colliderCapacity*2, &tempKey);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&physicsPtr);

                _physics_free_material(physicsKeyPtr, materialId);

                return B32_FALSE;
            }
            
            u32 allocatedCount = physicsPtr->colliderCapacity;
            
            memory_set_alloc_offset_width(&tempKey, 
            sizeof(struct physics_collider)*physicsPtr->colliderCapacity, 
            sizeof(struct physics_collider)*allocatedCount, '\0');

            resultCode = memory_map_alloc(&tempKey, 
            (void **)&colliderArrPtr);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&physicsPtr);

                _physics_free_material(physicsKeyPtr, materialId);

                return B32_FALSE;
            }

            for (u32 i = 0; i < allocatedCount; ++i)
            {
                u32 colliderIndex = physicsPtr->colliderCapacity + i;

                colliderArrPtr[colliderIndex].id = ++g_COLLIDER_ALLOCATION_ID_COUNTER;
                colliderArrPtr[colliderIndex].isActive = B32_FALSE;

                if (i > 0)
                {
                    colliderArrPtr[colliderIndex].prevColliderIndex = colliderIndex - 1;
                }

                if (i < (allocatedCount - 1))
                {
                    colliderArrPtr[colliderIndex].nextColliderIndex = colliderIndex + 1;
                }
            }

            if (physicsPtr->freeColliderCount > 0)
            {
                colliderArrPtr[physicsPtr->colliderCapacity].prevColliderIndex = colliderArrPtr[physicsPtr->freeColliderHeadIndex].prevColliderIndex;
                colliderArrPtr[physicsPtr->colliderCapacity + allocatedCount - 1].nextColliderIndex = physicsPtr->freeColliderHeadIndex;

                colliderArrPtr[colliderArrPtr[physicsPtr->freeColliderHeadIndex].prevColliderIndex].nextColliderIndex = physicsPtr->colliderCapacity;
                colliderArrPtr[physicsPtr->freeColliderHeadIndex].prevColliderIndex = physicsPtr->colliderCapacity;
            }
            else 
            {
                colliderArrPtr[physicsPtr->colliderCapacity].prevColliderIndex = physicsPtr->colliderCapacity + allocatedCount - 1;
                colliderArrPtr[physicsPtr->colliderCapacity].nextColliderIndex = physicsPtr->colliderCapacity;
            }

            memory_unmap_alloc((void **)&colliderArrPtr);

            physicsPtr->freeColliderHeadIndex = physicsPtr->colliderCapacity;

            colliderId = physicsPtr->colliderCapacity + 1;
            colliderIndex = colliderId - 1;

            physicsPtr->freeMaterialCount += allocatedCount - 1;
            physicsPtr->materialCapacity *= 2;

            memory_unmap_alloc((void **)&physicsPtr);
            
            if (!(_physics_activate_collider(physicsKeyPtr, materialId)))
            {
                _physics_free_material(physicsKeyPtr, materialId);

                return B32_FALSE;
            }

            physicsPtr->colliderCapacity *= 2;

            memcpy((void *)&physicsPtr->colliderArrKey, &tempKey, sizeof(struct memory_allocation_key));

            memory_unmap_alloc((void **)&physicsPtr);
        }
        else 
        {
            memory_error_code resultCode = memory_alloc(&physicsPtr->memoryHeapPageKey, 
            sizeof(struct physics_collider), NULL, &physicsPtr->colliderArrKey);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&physicsPtr);
                
                _physics_free_material(physicsKeyPtr, materialId);

                return B32_FALSE;
            }

            memory_set_alloc_offset_width(&physicsPtr->colliderArrKey, 0, 
            sizeof(struct physics_collider), '\0');
            
            ++physicsPtr->colliderCapacity;

            memory_unmap_alloc((void **)&physicsPtr);
            
            if (!(_physics_activate_collider(physicsKeyPtr, materialId)))
            {
                --physicsPtr->colliderCapacity;

                memory_free(&physicsPtr->colliderArrKey);

                _physics_free_material(physicsKeyPtr, materialId);

                return B32_FALSE;
            }
        }

        colliderId = ++g_COLLIDER_ALLOCATION_ID_COUNTER;
        colliderIndex = colliderId - 1;
    }

    {
        memory_error_code resultCode = memory_map_alloc(physicsKeyPtr, (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            _physics_free_collider(physicsKeyPtr, colliderId);
            _physics_free_material(physicsKeyPtr, materialId);

            return B32_FALSE;
        }
    }

    {
        memory_error_code resultCode = memory_map_alloc(&physicsPtr->colliderArrKey, 
        (void **)&colliderArrPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&physicsPtr);
            
            _physics_free_collider(physicsKeyPtr, colliderId);
            _physics_free_material(physicsKeyPtr, materialId);

            return B32_FALSE;
        }
    }

    struct physics_collider *colliderPtr = &colliderArrPtr[colliderId - 1];
    colliderPtr->bounds.right = 1.f;
    colliderPtr->bounds.top = 1.f;
    colliderPtr->bounds.left = 0.f;
    colliderPtr->bounds.bottom = 0.f;
    //colliderPtr->isTrigger = B32_TRUE;

    memory_unmap_alloc((void **)&colliderArrPtr);

    physics_id rigidbodyId;
    u32 rigidbodyIndex;
    
    if ((_physics_get_free_rigidbody_index(physicsKeyPtr, &rigidbodyIndex)))
    {
        rigidbodyId = rigidbodyIndex + 1;

        if (!_physics_activate_rigidbody(physicsKeyPtr, rigidbodyId))
        {
            _physics_free_collider(physicsKeyPtr, colliderId);
            _physics_free_material(physicsKeyPtr, materialId);

            return B32_FALSE;
        }
    }
    else
    {
        memory_error_code resultCode = memory_map_alloc(physicsKeyPtr, (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }

        if (physicsPtr->rigidbodyCapacity > 0)
        {
            const struct memory_allocation_key tempKey;

            memory_error_code resultCode = memory_realloc(&physicsPtr->rigidbodyArrKey, sizeof(struct physics_rigidbody)*
                (physicsPtr->rigidbodyCapacity * 2), &tempKey);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&physicsPtr);

                return B32_FALSE;
            }
            
            u32 allocatedCount = physicsPtr->rigidbodyCapacity;
            
            memory_set_alloc_offset_width(&tempKey, physicsPtr->rigidbodyCapacity*
                sizeof(struct physics_rigidbody), sizeof(struct physics_rigidbody)*allocatedCount, '\0');

            memcpy((void *)&physicsPtr->rigidbodyArrKey, &tempKey, sizeof(struct memory_allocation_key));

            struct physics_rigidbody *rigidbodyArrPtr;

            resultCode = memory_map_alloc(&tempKey, (void **)&rigidbodyArrPtr);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&physicsPtr);
                
                _physics_free_collider(physicsKeyPtr, colliderId);
                _physics_free_material(physicsKeyPtr, materialId);

                return B32_FALSE;
            }
            
            for (u32 i = 0; i < allocatedCount; ++i)
            {
                u32 rigidbodyIndex = physicsPtr->rigidbodyCapacity + i;

                rigidbodyArrPtr[rigidbodyIndex].id = ++g_RIGIDBODY_ALLOCATION_ID_COUNTER;
                rigidbodyArrPtr[rigidbodyIndex].isActive = B32_FALSE;

                if (i > 0)
                {
                    rigidbodyArrPtr[rigidbodyIndex].prevRigidbodyIndex = rigidbodyIndex - 1;
                }

                if (i < (allocatedCount - 1))
                {
                    rigidbodyArrPtr[rigidbodyIndex].nextRigidbodyIndex = rigidbodyIndex + 1;
                }
            }

            if (physicsPtr->freeRigidbodyCount > 0)
            {
                rigidbodyArrPtr[physicsPtr->rigidbodyCapacity].prevRigidbodyIndex = rigidbodyArrPtr[physicsPtr->freeRigidbodyHeadIndex].prevRigidbodyIndex;
                rigidbodyArrPtr[physicsPtr->rigidbodyCapacity + allocatedCount - 1].nextRigidbodyIndex = physicsPtr->freeRigidbodyHeadIndex;

                rigidbodyArrPtr[rigidbodyArrPtr[physicsPtr->freeRigidbodyHeadIndex].prevRigidbodyIndex].nextRigidbodyIndex = physicsPtr->rigidbodyCapacity;
                rigidbodyArrPtr[physicsPtr->freeRigidbodyHeadIndex].prevRigidbodyIndex = physicsPtr->rigidbodyCapacity;
            }
            else 
            {
                rigidbodyArrPtr[physicsPtr->rigidbodyCapacity].prevRigidbodyIndex = physicsPtr->rigidbodyCapacity + allocatedCount - 1;
                rigidbodyArrPtr[physicsPtr->rigidbodyCapacity].nextRigidbodyIndex = physicsPtr->rigidbodyCapacity;
            }

            memory_unmap_alloc((void **)&rigidbodyArrPtr);

            physicsPtr->freeRigidbodyHeadIndex = physicsPtr->rigidbodyCapacity;

            rigidbodyId = physicsPtr->rigidbodyCapacity + 1;
            rigidbodyIndex = rigidbodyId - 1;

            physicsPtr->freeMaterialCount += allocatedCount - 1;
            physicsPtr->materialCapacity *= 2;

            if (!(circular_buffer_create(&physicsPtr->memoryHeapPageKey, 
                sizeof(struct physics_force)*PHYSICS_RB_MAX_ACTIVE_FORCES, &rbPtr->forcesArenaKey)))
            {
                memory_unmap_alloc((void **)&physicsPtr);

                _physics_free_collider(physicsKeyPtr, colliderId);
                _physics_free_material(physicsKeyPtr, materialId);

                return B32_FALSE;
            }

            memory_unmap_alloc((void **)&physicsPtr);
            
            if (!(_physics_activate_rigidbody(physicsKeyPtr, rigidbodyId)))
            {
                _physics_free_collider(physicsKeyPtr, colliderId);
                _physics_free_material(physicsKeyPtr, materialId);

                return B32_FALSE;
            }
        }
        else 
        {
            memory_error_code resultCode = memory_alloc(&physicsPtr->memoryHeapPageKey, 
            sizeof(struct physics_rigidbody)*(physicsPtr->rigidbodyCapacity + 1), NULL, 
            &physicsPtr->rigidbodyArrKey);

            if (resultCode != MEMORY_OK)
            {
                memory_unmap_alloc((void **)&physicsPtr);
                
                _physics_free_collider(physicsKeyPtr, colliderId);
                _physics_free_material(physicsKeyPtr, materialId);

                return B32_FALSE;
            }

            memory_set_alloc_offset_width(&physicsPtr->rigidbodyArrKey, 0, 
            sizeof(struct physics_rigidbody), '\0');

            struct physics_rigidbody *rigidbodyPtr;

            resultCode = memory_map_alloc(&physicsPtr->rigidbodyArrKey, (void **)&rigidbodyPtr);

            if (resultCode != MEMORY_OK)
            {
                memory_free(&physicsPtr->rigidbodyArrKey);

                memory_unmap_alloc((void **)&physicsPtr);

                _physics_free_collider(physicsKeyPtr, colliderId);
                _physics_free_material(physicsKeyPtr, materialId);

                return B32_FALSE;
            }

            rigidbodyPtr->id = ++g_MATERIAL_ALLOCATION_ID_COUNTER;
            
            rigidbodyId = rigidbodyPtr->id;
            rigidbodyIndex = rigidbodyPtr->id - 1;
            
            if (!(circular_buffer_create(&physicsPtr->memoryHeapPageKey, 
                sizeof(struct physics_force)*PHYSICS_RB_MAX_ACTIVE_FORCES, &rbPtr->forcesArenaKey)))
            {
                memory_unmap_alloc((void **)&physicsPtr);

                _physics_free_collider(physicsKeyPtr, colliderId);
                _physics_free_material(physicsKeyPtr, materialId);

                return B32_FALSE;
            }
            
            memory_unmap_alloc((void **)&rigidbodyPtr);
            
            ++physicsPtr->materialCapacity;

            memory_unmap_alloc((void **)&physicsPtr);
            
            if (!(_physics_activate_material(physicsKeyPtr, materialId)))
            {
                return B32_FALSE;
            }
        }
    }

    {
        memory_error_code resultCode = memory_map_alloc(physicsKeyPtr, (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            _physics_free_rigidbody(physicsKeyPtr, rigidbodyId);
            _physics_free_collider(physicsKeyPtr, colliderId);
            _physics_free_material(physicsKeyPtr, materialId);

            return B32_FALSE;
        }
    }

    struct physics_rigidbody *rbArrPtr;
    {
        memory_error_code resultCode = memory_map_alloc(&physicsPtr->rigidbodyArrKey, (void **)&rbArrPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&physicsPtr);

            _physics_free_rigidbody(physicsKeyPtr, rigidbodyId);
            _physics_free_collider(physicsKeyPtr, colliderId);
            _physics_free_material(physicsKeyPtr, materialId);

            return B32_FALSE;
        }
    }

    struct physics_rigidbody *rigidbodyPtr = &rbArrPtr[rigidbodyId - 1];

    rigidbodyPtr->mass = PHYSICS_DEFAULT_MASS;
    rigidbodyPtr->isGravity = B32_FALSE;
    rigidbodyPtr->collider = colliderId;
    rigidbodyPtr->material = materialId;
    rigidbodyPtr->isKinematic = B32_FALSE;
    rigidbodyPtr->position[0] = 0.f;
    rigidbodyPtr->position[1] = 0.f;
    rigidbodyPtr->position[2] = 0.f;
    rigidbodyPtr->rotation[0] = 0.f;
    rigidbodyPtr->rotation[1] = 0.f;
    rigidbodyPtr->rotation[2] = 0.f;
    rigidbodyPtr->rotationVelocity[0] = 0.f;
    rigidbodyPtr->rotationVelocity[1] = 0.f;
    rigidbodyPtr->rotationVelocity[2] = 0.f;

    circular_buffer_reset(&rigidbodyPtr->forcesArenaKey);

    rigidbodyPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_SPEED].type = PHYSICS_RB_CONSTRAINT_MAX_SPEED;
    rigidbodyPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_SPEED].isActive = B32_FALSE;

    real32 *maxSpeedConstraintPtr;
    {
        memory_error_code resultCode = memory_raw_alloc(&rigidbodyPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_SPEED].rawDataKey, 
        sizeof(real32));

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&rbArrPtr);
            memory_unmap_alloc((void **)&physicsPtr);

            _physics_free_rigidbody(physicsKeyPtr, rigidbodyId);
            _physics_free_collider(physicsKeyPtr, colliderId);
            _physics_free_material(physicsKeyPtr, materialId);

            return B32_FALSE;
        }

        resultCode = memory_map_raw_allocation(&rigidbodyPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_SPEED].rawDataKey, 
        (void **)&maxSpeedConstraintPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_raw_free(&rigidbodyPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_SPEED].rawDataKey);

            memory_unmap_alloc((void **)&rbArrPtr);
            memory_unmap_alloc((void **)&physicsPtr);
            
            _physics_free_rigidbody(physicsKeyPtr, rigidbodyId);
            _physics_free_collider(physicsKeyPtr, colliderId);
            _physics_free_material(physicsKeyPtr, materialId);

            return B32_FALSE;
        }
    }

    *maxSpeedConstraintPtr = 0.f;

    memory_unmap_raw_allocation(&rigidbodyPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_SPEED].rawDataKey, 
    (void **)&maxSpeedConstraintPtr);
    
    rigidbodyPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_ROTATION].type = PHYSICS_RB_CONSTRAINT_MAX_ROTATION;
    rigidbodyPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_ROTATION].isActive = B32_FALSE;

    real32 *maxRotationConstraintPtr;
    {
        memory_error_code resultCode = memory_raw_alloc(&rigidbodyPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_ROTATION].rawDataKey, 
        sizeof(real32));

        if (resultCode != MEMORY_OK)
        {
            memory_raw_free(&rigidbodyPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_SPEED].rawDataKey);

            memory_unmap_alloc((void **)&rbArrPtr);
            memory_unmap_alloc((void **)&physicsPtr);
            
            _physics_free_rigidbody(physicsKeyPtr, rigidbodyId);
            _physics_free_collider(physicsKeyPtr, colliderId);
            _physics_free_material(physicsKeyPtr, materialId);

            return B32_FALSE;
        }

        resultCode = memory_map_raw_allocation(&rigidbodyPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_ROTATION].rawDataKey, 
        (void **)&maxRotationConstraintPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_raw_free(&rigidbodyPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_SPEED].rawDataKey);
            memory_raw_free(&rigidbodyPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_ROTATION].rawDataKey);

            memory_unmap_alloc((void **)&rbArrPtr);
            memory_unmap_alloc((void **)&physicsPtr);

            return B32_FALSE;
        }
    }

    *maxRotationConstraintPtr = 0.f;
    
    memory_unmap_raw_allocation(&rigidbodyPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_ROTATION].rawDataKey, 
    (void **)&maxRotationConstraintPtr);

    memory_unmap_alloc((void **)&rbArrPtr);
    memory_unmap_alloc((void **)&physicsPtr);

    return B32_TRUE;
}

b32
physics_rigidbody_set_constraint(const struct memory_allocation_key *physicsKeyPtr, physics_id rigidbodyId,
    physics_rigidbody_constraint_t type, void *dataPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(physicsKeyPtr)))
    {
        return B32_FALSE;
    }

    if (rigidbodyId < 1)
    {
        return B32_FALSE;
    }

    if (!dataPtr)
    {
        return B32_FALSE;
    }

    struct physics *physicsPtr;
    struct physics_rigidbody *rigidbodyArrPtr;
    {
        memory_error_code resultCode = memory_map_alloc(physicsKeyPtr, 
        (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE:
        }

        resultCode = memory_map_alloc(&physicsPtr->rigidbodyArrKey, 
        (void **)&rigidbodyArrPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&physicsPtr);

            return B32_FALSE;
        }
    }

    if (rigidbodyId > physicsPtr->rigidbodyCapacity)
    {
        memory_unmap_alloc((void **)&rigidbodyArrPtr);
        memory_unmap_alloc((void **)&physicsPtr);

        return B32_FALSE;
    }

    struct physics_rigidbody *rigidbodyPtr = &rigidbodyArrPtr[rigidbodyId - 1];

    void *constraintPtr;
    {
        memory_error_code resultCode = memory_map_raw_allocation(&rigidbodyPtr->
            constraintArr[type].rawDataKey, &constraintPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    switch(type)
    {
        case PHYSICS_RB_CONSTRAINT_MAX_SPEED:
        {
            memcpy(constraintPtr, dataPtr, sizeof(real32));
        } break;
        
        case PHYSICS_RB_CONSTRAINT_MAX_ROTATION:
        {
            memcpy(constraintPtr, dataPtr, sizeof(real32));
        } break;

        default: 
        {
            memory_unmap_alloc((void **)&rigidbodyArrPtr);
            memory_unmap_alloc((void **)&physicsPtr);

            return B32_FALSE;
        } break;
    }

    memory_unmap_alloc((void **)&rigidbodyArrPtr);
    memory_unmap_alloc((void **)&physicsPtr);

    return B32_TRUE;
}

b32
physics_rigidbody_get_constraint(const struct memory_allocation_key *physicsKeyPtr, physics_id rigidbodyId,
    physics_rigidbody_constraint_t type, void *outDataPtr)
{
    if ((MEMORY_IS_ALLOCATION_NULL(physicsKeyPtr)))
    {
        return B32_FALSE;
    }

    if (rigidbodyId < 1)
    {
        return B32_FALSE;
    }

    if (!outDataPtr)
    {
        return B32_FALSE;
    }

    struct physics *physicsPtr;
    struct physics_rigidbody *rigidbodyArrPtr;
    {
        memory_error_code resultCode = memory_map_alloc(physicsKeyPtr, 
        (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE:
        }

        resultCode = memory_map_alloc(&physicsPtr->rigidbodyArrKey, 
        (void **)&rigidbodyArrPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&physicsPtr);

            return B32_FALSE;
        }
    }

    if (rigidbodyId > physicsPtr->rigidbodyCapacity)
    {
        memory_unmap_alloc((void **)&rigidbodyArrPtr);
        memory_unmap_alloc((void **)&physicsPtr);

        return B32_FALSE;
    }

    struct physics_rigidbody *rigidbodyPtr = &rigidbodyArrPtr[rigidbodyId - 1];

    void *constraintPtr;
    {
        memory_error_code resultCode = memory_map_raw_allocation(&rigidbodyPtr->
            constraintArr[type].rawDataKey, &constraintPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    switch(type)
    {
        case PHYSICS_RB_CONSTRAINT_MAX_SPEED:
        {
            memcpy(outDataPtr, constraintPtr, sizeof(real32));
        } break;
        
        case PHYSICS_RB_CONSTRAINT_MAX_ROTATION:
        {
            memcpy(outDataPtr, constraintPtr, sizeof(real32));
        } break;

        default: 
        {
            memory_unmap_alloc((void **)&rigidbodyArrPtr);
            memory_unmap_alloc((void **)&physicsPtr);

            return B32_FALSE;
        } break;
    }

    memory_unmap_alloc((void **)&rigidbodyArrPtr);
    memory_unmap_alloc((void **)&physicsPtr);

    return B32_TRUE;
}

b32
physics_rigidbody_set_constraint_active(const struct memory_allocation_key physicsKeyPtr, 
    physics_id rigidbodyId, physics_rigidbody_constraint_t type, b32 isActive)
{
    if ((MEMORY_IS_ALLOCATION_NULL(physicsKeyPtr)))
    {
        return B32_FALSE;
    }

    if (rigidbodyId < 1)
    {
        return B32_FALSE;
    }
    
    struct physics *physicsPtr;
    struct physics_rigidbody *rigidbodyArrPtr;
    {
        memory_error_code resultCode = memory_map_alloc(physicsKeyPtr, 
        (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE:
        }

        resultCode = memory_map_alloc(&physicsPtr->rigidbodyArrKey, 
        (void **)&rigidbodyArrPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&physicsPtr);

            return B32_FALSE;
        }
    }

    if (rigidbodyId > physicsPtr->rigidbodyCapacity)
    {
        memory_unmap_alloc((void **)&rigidbodyArrPtr);
        memory_unmap_alloc((void **)&physicsPtr);

        return B32_FALSE;
    }

    struct physics_rigidbody *rigidbodyPtr = &rigidbodyArrPtr[rigidbodyId - 1];

    rigidbodyPtr->constraintArr[type].isActive = isActive;

    memory_unmap_alloc((void **)&rigidbodyArrPtr);
    memory_unmap_alloc((void **)&physicsPtr);

    return B32_TRUE;
}

b32
physics_rigidbody_get_constraint_active(const struct memory_allocation_key *physicsKeyPtr, 
    physics_id rigidbodyId, physics_rigidbody_constraint_t type, b32 *outIsActive)
{
    if ((MEMORY_IS_ALLOCATION_NULL(physicsKeyPtr)))
    {
        return B32_FALSE;
    }

    if (rigidbodyId < 1)
    {
        return B32_FALSE;
    }

    if (!outIsActive)
    {
        return B32_FALSE:
    }
    
    struct physics *physicsPtr;
    struct physics_rigidbody *rigidbodyArrPtr;
    {
        memory_error_code resultCode = memory_map_alloc(physicsKeyPtr, 
        (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE:
        }

        resultCode = memory_map_alloc(&physicsPtr->rigidbodyArrKey, 
        (void **)&rigidbodyArrPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&physicsPtr);

            return B32_FALSE;
        }
    }

    if (rigidbodyId > physicsPtr->rigidbodyCapacity)
    {
        memory_unmap_alloc((void **)&rigidbodyArrPtr);
        memory_unmap_alloc((void **)&physicsPtr);

        return B32_FALSE;
    }

    struct physics_rigidbody *rigidbodyPtr = &rigidbodyArrPtr[rigidbodyId - 1];

    *outIsActive = rigidbodyPtr->constraintArr[type].isActive;

    memory_unmap_alloc((void **)&rigidbodyArrPtr);
    memory_unmap_alloc((void **)&physicsPtr);

    return B32_TRUE;
}

b32
physics_rigidbody_apply_force(const struct memory_allocation_key *physicsKeyPtr, 
    physics_id rigidbodyId, real32 direction[3], real32 magnitude, real32 duration)
{
    if ((MEMORY_IS_ALLOCATION_NULL(physicsKeyPtr)))
    {
        return B32_FALSE;
    }

    if (rigidbodyId < 1)
    {
        return B32_FALSE;
    }

    b32 isResult = _physics_rigidbody_alloc_force(physicsKeyPtr, rigidbodyId, direction, 
        magnitude, duration);

    return isResult;
}

b32
physics_rigidbody_mute_force(const struct memory_allocation_key *physicsKeyPtr, 
    physics_id forceId, b32 isMuted)
{
    if ((MEMORY_IS_ALLOCATION_NULL(physicsKeyPtr)))
    {
        return B32_FALSE;
    }

    struct physics *physicsPtr;
    {
        memory_error_code resultCode = memory_map_alloc(physicsKeyPtr, 
        (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    struct physics_force_object *forceObjPtr;

    if (!(basic_dict_map_data(&physicsPtr->forceDictKey, (void *)&forceId, 
        (void **)&forceObjPtr)))
    {
        memory_unmap_alloc((void **)&physicsPtr);

        return B32_FALSE;
    }

    forceObjPtr->forceData.isMuted = isMuted;

    basic_dict_unmap_data(&physicsPtr->forceDictKey, &forceId, 
    (void **)&forceObjPtr);

    return B32_TRUE;
}

b32
physics_update(const struct memory_allocation_key *physicsKeyPtr, real32 timestep)
{
    if ((MEMORY_IS_ALLOCATION_NULL(physicsKeyPtr)))
    {
        return B32_FALSE;
    }

    struct physics *physicsPtr;
    {
        memory_error_code resultCode = memory_map_alloc(physicsKeyPtr, 
        (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
    }

    // no active rigidbodies; operation is already complete
    if (physicsPtr->activeRigidbodyCount < 1)
    {
        memory_unmap_alloc((void **)&physicsPtr);

        return B32_TRUE;
    }

    u32 activeHeadRigidbodyIndex = physicsPtr->activeRigidbodyHeadIndex;

    memory_unmap_alloc((void **)&physicsPtr);

    for (u32 activeLhsRigidbodyIndex = activeHeadRigidbodyIndex;;)
    {
        memory_error_code resultCode = memory_map_alloc(physicsKeyPtr, 
        (void **)&physicsPtr);

        if (resultCode != MEMORY_OK)
        {
            return B32_FALSE;
        }
        
        struct physics_rigidbody *rigidbodyArrPtr;

        resultCode = memory_map_alloc(&physicsPtr->rigidbodyArrKey, 
        (void **)&rigidbodyArrPtr);

        if (resultCode != MEMORY_OK)
        {
            memory_unmap_alloc((void **)&physicsPtr);

            return B32_FALSE;
        }

        struct physics_rigidbody *rigidbodyPtr = &rigidbodyArrPtr[activeLhsRigidbodyIndex];
        physics_id rigidbodyId = rigidbodyPtr->id;

        memory_unmap_alloc((void **)&rigidbodyArrPtr);
        memory_unmap_alloc((void **)&physicsPtr);

        if (physicsPtr->activeRigidbodyCount > 1)
        {
            for (u32 activeRhsRigidbodyIndex = activeHeadRigidbodyIndex;;)
            {
                resultCode = memory_map_alloc(physicsKeyPtr, 
                (void **)&physicsPtr);

                if (resultCode != MEMORY_OK)
                {
                    return B32_FALSE;
                }
                
                resultCode = memory_map_alloc(&physicsPtr->rigidbodyArrKey, 
                (void **)&rigidbodyArrPtr);

                if (resultCode != MEMORY_OK)
                {
                    memory_unmap_alloc((void **)&physicsPtr);

                    return B32_FALSE;
                }

                if (activeRhsRigidbodyIndex == activeLhsRigidbodyIndex)
                {
                    activeRhsRigidbodyIndex = rigidbodyPtr->nextRigidbodyIndex;

                    memory_unmap_alloc((void **)&rigidbodyArrPtr);
                    memory_unmap_alloc((void **)&physicsPtr);

                    continue;
                }

                struct physics_rigidbody *rhsRigidbodyPtr = &rigidbodyArrPtr[activeRhsRigidbodyIndex];
                physics_id rhsRigidbodyId = rhsRigidbodyPtr->id;

                memory_unmap_alloc((void **)&rigidbodyArrPtr);
                memory_unmap_alloc((void **)&physicsPtr);

                b32 isOverlap;
                
                _physics_rigidbody_collider_is_overlap(physicsKeyPtr, rigidbodyId, rhsRigidbodyId, 
                &isOverlap);

                if (isOverlap)
                {
                    resultCode = memory_map_alloc(physicsKeyPtr, 
                    (void **)&physicsPtr);

                    if (resultCode != MEMORY_OK)
                    {
                        return B32_FALSE;
                    }

                    u64 key;

                    ((physics_id *)&key)[0] = rigidbodyId;
                    ((physics_id *)&key)[1] = rhsRigidbodyId;

                    if (!(basic_dict_get_is_found(&physicsPtr->collisionDictKey, &key)))
                    {
                        basic_dict_push_data(&physicsPtr->collisionDictKey, &key, NULL, 
                        NULL, NULL);
                    }

                    struct physics_collision collData;

                    collData.id = ++g_COLLISION_ID_COUNTER;
                    collData.rbLhsId = rigidbodyId;
                    collData.rbRhsId = rhsRigidbodyId;

                    circular_buffer_write_bytes(&physicsPtr->collisionArenaKey, sizeof(struct physics_collision), 
                    (void *)&collData);

                    memory_unmap_alloc((void **)&physicsPtr);
                }

                if (rhsRigidbodyPtr->nextRigidbodyIndex == physicsPtr->activeRigidbodyHeadIndex)
                {
                    break;
                }
            }
        }

        u64 bytesRead;
        struct physics_force force;

        // add force to physics list
        while (!(circular_buffer_read_bytes(&rigidbodyPtr->forcesArenaKey, 
            sizeof(struct physics_force), (void *)&force, 
            &bytesRead)))
        {
            if (bytesRead == sizeof(struct physics_force))
            {
                const struct memory_allocation_key forceObjNodeKey;
                struct physics_force_object *forceObjPtr;

                if (physicsPtr->freeForceCount > 0)
                {
                    if (!(basic_list_get_next_node(&physicsPtr->freeForceListKey, NULL, 
                        &forceObjNodeKey)))
                    {
                        continue;
                    }

                    if (!(basic_list_move_node(&physicsPtr->freeForceListKey, 
                        &physicsPtr->activeForceListKey, &forceObjNodeKey)))
                    {
                        continue;
                    }

                    if (!(basic_list_map_data(&forceObjNodeKey, (void **)&forceObjPtr)))
                    {
                        continue;
                    }
                }
                else 
                {
                    if (!(basic_list_insert_front(&physicsPtr->activeForceListKey, 0, 
                        sizeof(struct physics_force_object), physicsKeyPtr, 
                        &forceObjNodeKey)))
                    {
                        continue;
                    }
                    
                    if (!(basic_list_map_data(&forceObjNodeKey, (void **)&forceObjPtr)))
                    {
                        continue;
                    }
                }

                forceObjPtr->rigidbodyId = rigidbodyPtr->id;

                memcpy(&forceObjPtr->forceData, &force, sizeof(struct physics_force));

                basic_list_unmap_data(&physicsPtr->activeForceListKey, &forceObjNodeKey, 
                (void **)&forceObjPtr);
            }
        }

        circular_buffer_reset(&rigidbodyPtr->forcesArenaKey);

        u32 nextRigidbodyIndex = rigidbodyPtr->nextRigidbodyIndex;

        memory_unmap_alloc((void **)&rigidbodyArrPtr);
        memory_unmap_alloc((void **)&physicsPtr);

        if (rigidbodyPtr->nextRigidbodyIndex != activeHeadRigidbodyIndex)
        {
            activeLhsRigidbodyIndex = nextRigidbodyIndex;
        }
    }

    // TODO: handle collisions

    // TODO: apply forces

    // TODO: apply friction and drag
    memory_unmap_alloc((void **)&physicsPtr);

    return B32_TRUE;
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
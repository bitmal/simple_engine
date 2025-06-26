#ifndef __GAME_H
#define __GAME_H

#include "renderer.h"
#include "types.h"
#include "physics.h"
#include "vec2.h"
#include <SDL3/SDL_gamepad.h>

struct context;
struct interface;
struct game;

typedef i32 game_id;
#define GAME_NULL_ID -1

typedef void (*game_timer_callback)
    (struct game *g, real32 elapsedTime, void *dataPtr);

typedef void (*game_physics_collider_trigger_callback)
    (struct game *g, game_id lhsEntity, game_id rhsEntity);

typedef void (*game_physics_collider_collision_callback)
    (struct game *g, game_id lhsEntity, game_id rhsEntity);

enum game_component_type
{
    GAME_TRANSFORM, 
    GAME_RENDER_COMPONENT,
    GAME_PHYSICS_COMPONENT
};

struct game_layer
{
    u32 layer;
    const char *name;
};

struct game_component_index
{
    i32 index;
    enum game_component_type type;
};

struct game_transform
{
    game_id id;
    struct vec2 position;
    struct vec2 pivot;
    struct vec2 scale;
    real32 rotation;
    b32 isDirty;
};

struct game_render_component
{
    game_id id;
    i32 rendererModelId;
};

struct game_physics_component
{
    game_id id;
    physics_id rigidbody;
    b32 isActive;
};

struct game_entity
{
    game_id id;
    game_id transform;
    game_id renderComponent;
    game_id physicsComponent;
    const char *layer;
    b32 isActive;
};

struct game_timer
{
    u32 startTime;
    u32 duration;
    game_timer_callback callback;
    void *dataPtr;
    b32 isActive;
};

struct player
{
    game_id entityId;
    real32 movementAxisNormalized[2];
    struct game_timer *animTimer;
};

struct game
{
    struct context *app;
    struct game_entity *entities;
    struct game_component_index *components;
    struct game_transform *transforms;
    struct game_render_component *renderComponents;
    struct game_physics_component *physicsComponents;
    struct interface *gameInterface;
    renderer_id *interfaceElementRenders;
    struct basic_dict *interfaceElementRenderMap;
    struct game_timer *timers;
    struct game_layer *layers;
    struct basic_dict *layerDict;
    struct player *player0;
    i32 entityCount;
    i32 componentCount;
    i32 transformCount;
    i32 renderComponentCount;
    i32 physicsComponentCount;
    i32 timerCount;
    i32 activeTimerCount;
    i32 layerCount;
};

struct game *
game_init(struct context *app);

void
game_cycle(struct game *g, real32 dt);

void
game_render(struct game *g);

game_id
game_create_entity(struct game *g, const char *layer);

game_id
game_create_component(struct game *g, enum game_component_type type);

b32
game_attach_component(struct game *g, game_id entity, game_id component);

void *
game_get_component(struct game *g, game_id entity, enum game_component_type type);

b32
game_instantiate_interface_element(struct game *g, const char *name, 
    struct interface_element_pattern *patterns, i32 patternCount, real32 x, real32 y, 
    real32 width, real32 height, u32 color);

void
game_start_timer(struct game *g, real32 seconds, game_timer_callback callback, void *dataPtr);

void
game_init_layer(struct game *g, const char *name, u32 layer);

b32
game_load_texture(struct game *g, const char *name);

#endif

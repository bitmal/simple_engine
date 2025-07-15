#include "game.h"
#include "config.h"
#include "engine.h"
#include "renderer.h"
#include "vec2.h"
#include "vec4.h"
#include "mat44.h"
#include "mat44_func.h"
#include "basic_dict.h"
#include "input.h"
#include "types.h"
#include "memory.h"
#include "physics.h"
#include "interface.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <math.h>

static void 
_game_input_callback(const char *key, const char *action, void *dataPtr)
{
    struct game *g = (struct game *)dataPtr;
    const b32 isKeyDown = !strcmp(action, "KEYDOWN");

    if (isKeyDown)
    {
        if (!strcmp(key, "escape"))
        {
            g->app->isRunning = B32_FALSE;
        }
    }
}

void
_init_player_0(struct game *g)
{
    g->player0 = memory_alloc(g->app->memoryContext, sizeof(struct player));
    
    g->player0->entityId = game_create_entity(g, "player");
    
    game_id transfId = game_create_component(g, GAME_TRANSFORM);
    game_attach_component(g, g->player0->entityId, transfId);
    struct game_transform *transf = game_get_component(g, g->player0->entityId, GAME_TRANSFORM);
    transf->position.x = *(real32 *)config_get_var(g->app->config, "GAME_grid_width", 0)/2.f;
    transf->position.y = *(real32 *)config_get_var(g->app->config, "GAME_grid_height", 0)/2.f;
    transf->rotation = 0.f;

    game_id renderId = game_create_component(g, GAME_RENDER_COMPONENT);
    game_attach_component(g, g->player0->entityId, renderId);
    struct game_render_component *renderComp = game_get_component(g, g->player0->entityId, GAME_RENDER_COMPONENT);
    renderer_id materialId = renderer_model_get_material(g->app->renderContext, renderComp->rendererModelId);

    real32 color[] = {1.f, 1.f, 1.f, 1.f};
    renderer_material_update_property(g->app->renderContext, materialId, "u_Color", color);
    u32 width = 25;
    renderer_material_update_property(g->app->renderContext, materialId, "u_TexWidth", &width);
    u32 height = 25;
    renderer_material_update_property(g->app->renderContext, materialId, "u_TexHeight", &height);
    
    game_id physicsId = game_create_component(g, GAME_PHYSICS_COMPONENT);
    game_attach_component(g, g->player0->entityId, physicsId);
    struct game_physics_component *physicsComp = &g->physicsComponents[physicsId];
    
    struct physics_rigidbody *rbPtr = physics_get_rigidbody(g->app->physics, physicsComp->rigidbody);
    //rbPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_SPEED].isActive = B32_TRUE;
    //*((real32 *)rbPtr->constraintArr[PHYSICS_RB_CONSTRAINT_MAX_SPEED].data) = 15.f;
    rbPtr->isGravity = B32_FALSE;
    rbPtr->position[0] = transf->position.x;
    rbPtr->position[1] = transf->position.y;
    rbPtr->position[2] = 1.f;
    rbPtr->rotation[0] = 0.f;
    rbPtr->rotation[1] = 0.f;
    rbPtr->rotation[2] = transf->rotation;

    struct physics_material *physicsMaterial = physics_get_material(g->app->physics, rbPtr->material);
    physicsMaterial->dragCoefficient = 0.01f;

    struct physics_collider *collider = physics_get_collider(g->app->physics, rbPtr->collider);
    collider->bounds.bottom = 0.f;
    collider->bounds.top = 10.f;
    collider->bounds.left = 0.f;
    collider->bounds.right = 10.f;

    g->player0->animTimer = NULL;
}

struct game *
game_init(struct context *app)
{
    struct game *g = memory_alloc(app->memoryContext, sizeof(struct game));
    memset(g, 0, sizeof(struct game));
    g->app = app;

    g->timerCount = 10;
    g->timers = memory_alloc(app->memoryContext, sizeof(struct game_timer)*g->timerCount);
    
    g->layerDict = DICTIONARY(app->memoryContext, NULL);

    game_init_layer(g, "default", 0);
    game_init_layer(g, "ui", 1);
    game_init_layer(g, "player", 2);
    game_init_layer(g, "foreground", 3);
    game_init_layer(g, "background", 10);

    renderer_init_layer(g->app->renderContext, "default", 0);
    renderer_init_layer(g->app->renderContext, "ui", 1);
    renderer_init_layer(g->app->renderContext, "player", 2);
    renderer_init_layer(g->app->renderContext, "foreground", 3);
    renderer_init_layer(g->app->renderContext, "background", 4);
    
    const char *keys[] = {
        "escape",
        "space",
        "a",
        "s",
        "d",
        "w"
    };

    input_add_keys(app->inputContext, g->app->memoryContext, keys, sizeof(keys)/sizeof(keys[0]));

    input_bind_key(app->inputContext, g->app->memoryContext, "escape", _game_input_callback);
    input_bind_key(app->inputContext, g->app->memoryContext, "space", _game_input_callback);
    input_bind_key(app->inputContext, g->app->memoryContext, "w", _game_input_callback);
    input_bind_key(app->inputContext, g->app->memoryContext, "a", _game_input_callback);
    input_bind_key(app->inputContext, g->app->memoryContext, "s", _game_input_callback);
    input_bind_key(app->inputContext, g->app->memoryContext, "d", _game_input_callback);

    struct vec4 bgColor = {0.f, 0.f, 0.1f, 1.f};
    renderer_set_background_color(app->renderContext, &bgColor);
    
    renderer_load_program(app->renderContext, "unlit");
    renderer_load_program(app->renderContext, "unlit_frame");
    renderer_load_program(app->renderContext, "unlit_texture");
    renderer_load_program(app->renderContext, "interface");
    
    real32 textureQuadVertices[] = {
        0.f, 0.f, 0.f, 1.f,  0.f, 0.f,
        1.f, 0.f, 0.f, 1.f,  1.f, 0.f,
        1.f, 1.f, 0.f, 1.f,  1.f, 1.f,
        0.f, 1.f, 0.f, 1.f,  0.f, 1.f
    };

    u16 textureQuadIndices[] = {
        0, 1, 3,
        1, 2, 3
    };

    renderer_create_mesh(app->renderContext, "texture_quad", textureQuadVertices, sizeof(real32)*6, 
        4, textureQuadIndices, 6);
    
    real32 frameQuadVertices[] = {
        0.f, 0.f, 0.f, 1.f,  1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 0.f,
        25.f, 0.f, 0.f, 1.f,  1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 0.f,
        12.f, 25.f, 0.f, 1.f,  1.f, 1.f, 1.f, 1.f, 1.f, 1.f, 0.f
    };

    u16 frameQuadIndices[] = {
        0, 1, 2
    };

    renderer_create_mesh(app->renderContext, "frame_triangle", frameQuadVertices, sizeof(real32)*11, 
        3, frameQuadIndices, 3);

    renderer_load_texture(g->app->renderContext, "test1", "test1", B32_FALSE);

    g->gameInterface = interface_init(g->app->memoryContext);
    g->interfaceElementRenderMap = DICTIONARY(g->app->memoryContext, NULL);

    _init_player_0(g);

    game_id wallId = game_create_entity(g, "foreground");
    
    game_id wallTransfId = game_create_component(g, GAME_TRANSFORM);
    game_attach_component(g, wallId, wallTransfId);

    struct game_transform *wallTransf = game_get_component(g, wallId, GAME_TRANSFORM);
    wallTransf->position.x = 0.f;
    wallTransf->position.y = 0.f;

    game_id wallRenderId = game_create_component(g, GAME_RENDER_COMPONENT);
    game_attach_component(g, wallId, wallRenderId);
    
    struct game_render_component *wallRender = game_get_component(g, wallId, GAME_RENDER_COMPONENT);

    renderer_id wallRenderMaterialId = renderer_model_get_material(g->app->renderContext, wallRender->rendererModelId);
    renderer_model_set_mesh(g->app->renderContext, wallRender->rendererModelId, "frame_triangle");
    renderer_material_set_program(g->app->renderContext, wallRenderMaterialId, "unlit_frame");

    real32 frameColor[] = {1.f, 0.f, 0.f, 1.f};
    renderer_material_update_property(g->app->renderContext, wallRenderMaterialId, "u_FrameColor", frameColor);
    real32 frameThickness = 0.1f;
    renderer_material_update_property(g->app->renderContext, wallRenderMaterialId, "u_FrameThickness", &frameThickness);

    physics_id wallPhysicsId = game_create_component(g, GAME_PHYSICS_COMPONENT);
    game_attach_component(g, wallId, wallPhysicsId);

    struct game_physics_component *wallPhysics = game_get_component(g, wallId, GAME_PHYSICS_COMPONENT);
    struct physics_rigidbody *wallPhysicsRb = physics_get_rigidbody(g->app->physics, wallPhysics->rigidbody);
    struct physics_material *wallPhysicsMaterial = physics_get_material(g->app->physics, wallPhysicsRb->material);

    //wallPhysicsRb->isGravity = B32_TRUE;
    
    struct physics_collider *collider = physics_get_collider(g->app->physics, wallPhysicsRb->collider);
    collider->bounds.bottom = 0.f;
    collider->bounds.top = 10.f;
    collider->bounds.left = 0.f;
    collider->bounds.right = 25.f;

    wallPhysicsRb->position[0] = wallTransf->position.x;
    wallPhysicsRb->position[1] = wallTransf->position.y;
    wallPhysicsRb->position[2] = 10.f;

    return g;
}

void
game_cycle(struct game *g, real32 dt)
{
    // INPUT

    // TIMERS
    for (i32 timerIndex = 0; timerIndex < g->activeTimerCount; ++timerIndex)
    {
        if (g->timers[timerIndex].isActive)
        {
            const u32 time = g->app->msSinceStart - g->timers[timerIndex].startTime;

            if (time >= g->timers[timerIndex].duration)
            {
                if (timerIndex == (g->activeTimerCount - 1))
                {
                    --g->activeTimerCount;
                }

                g->timers[timerIndex].isActive = B32_FALSE;
                g->timers[timerIndex].callback((void *)g, (real32)time/1000.f, g->timers[timerIndex].dataPtr);
            }
        }
        else if (timerIndex == (g->activeTimerCount - 1))
        {
            --g->activeTimerCount;
        }
    }

    // ENTITIES
    for (i32 entityIndex = 0; 
         entityIndex < g->entityCount;
         ++entityIndex)
    {
        struct game_entity *entity = &g->entities[entityIndex];
		{
			struct game_transform *transform;
			{
                if (entity->transform > GAME_NULL_ID)
        		{
        		    transform = &g->transforms[g->components[entity->transform].index];
        		}
                else
        		{
        		    transform = NULL;
        		}
			}
        	
        	struct game_physics_component *physicsComp;
			{
                if (entity->physicsComponent > GAME_NULL_ID)
        	    {
        	        physicsComp = &g->physicsComponents[g->components[entity->physicsComponent].index];
        	    }
                else
        	    {
        	        physicsComp = NULL;
        	    }
        	
				// PHYSICS
        		if (physicsComp && physicsComp->isActive && transform)
        		{
                    #define ACCEL 100.f

                    struct physics_rigidbody *rbPtr = physics_get_rigidbody(g->app->physics, physicsComp->rigidbody);

                    if (entity->id == g->player0->entityId)
                    {
                        real32 normalizedInputAxis[2];
                        normalizedInputAxis[0] = normalizedInputAxis[1] = 0.f;

                        if (input_get_key_down(g->app->inputContext, "a"))
                        {
                            normalizedInputAxis[0] = -1.f;
                        }
                        else if (input_get_key_down(g->app->inputContext, "d"))
                        {
                            normalizedInputAxis[0] = 1.f;
                        }
                        if (input_get_key_down(g->app->inputContext, "s"))
                        {
                            normalizedInputAxis[1] = -1.f;
                        }
                        else if (input_get_key_down(g->app->inputContext, "w"))
                        {
                            normalizedInputAxis[1] = 1.f;
                        }
                        
                        real32 rotationAxisMagnitude = fabsf(normalizedInputAxis[0]);
                        i32 rotationAxisSign = (rotationAxisMagnitude > 0.f) ? ((i32)ceilf(normalizedInputAxis[0]/rotationAxisMagnitude)) : 0;
                        real32 movementAxisMagnitude = fabsf(normalizedInputAxis[1]);
                        i32 movementAxisSign = (movementAxisMagnitude > 0.f) ? ((i32)ceilf(normalizedInputAxis[1]/movementAxisMagnitude)) : 0;

                        #define ROTATION_ACCEL (M_PI*2.f)

                        if (rotationAxisMagnitude > 0.f)
                        {
                            rbPtr->rotationVelocity[2] += normalizedInputAxis[0]*ROTATION_ACCEL*dt;
                        }
                        else
                        {
                            rbPtr->rotationVelocity[2] = 0.f;
                        }
                        
                        if (movementAxisMagnitude > 0.f)
                        {
                            struct mat44 rotationMatrix = mat44_func_roll(rbPtr->rotation[2]);
                            struct vec4 inputAxisDirection = {0.f, normalizedInputAxis[1], 0.f, 0.f};

                            struct vec4 movementDirection = mat44_func_multiply_vec4(&rotationMatrix, &inputAxisDirection);

                            physics_rigidbody_add_force(g->app->physics, physicsComp->rigidbody, &movementDirection._[0], rbPtr->mass*(movementAxisMagnitude*ACCEL), 
                                0.f);
                        }
                    }
                    
        		    transform->position.x = rbPtr->position[0];
        		    transform->position.y = rbPtr->position[1];
        		    transform->rotation = rbPtr->rotation[2];
        		}
			}

            //if (entity->id == g->player0->entityId)
            {
                const real32 gameGridWidth = *((real32 *)config_get_var(g->app->config, "GAME_grid_width", 0));
                const real32 gameGridHeight = *((real32 *)config_get_var(g->app->config, "GAME_grid_height", 0));

                struct physics_rigidbody *rbPtr = physicsComp ? physics_get_rigidbody(g->app->physics, physicsComp->rigidbody) : NULL;

                const real32 offsetPositionBufferSize = 5.f;

                if (transform->position.x >= (gameGridWidth + offsetPositionBufferSize))
                {
                    transform->position.x = -offsetPositionBufferSize;

                    if (rbPtr)
                    {
                        rbPtr->position[0] = transform->position.x;
                    }
                }
                else if (transform->position.x <= -offsetPositionBufferSize)
                {
                    transform->position.x = gameGridWidth + offsetPositionBufferSize - 0.1f;

                    if (rbPtr)
                    {
                        rbPtr->position[0] = transform->position.x;
                    }
                }
                
                if (transform->position.y >= (gameGridHeight + offsetPositionBufferSize))
                {
                    transform->position.y = -offsetPositionBufferSize;

                    if (rbPtr)
                    {
                        rbPtr->position[1] = transform->position.y;
                    }
                }
                else if (transform->position.y <= -offsetPositionBufferSize)
                {
                    transform->position.y = gameGridHeight + offsetPositionBufferSize;

                    if (rbPtr)
                    {
                        rbPtr->position[1] = transform->position.y;
                    }
                }
            }
        }
    }
}

void
game_render(struct game *g)
{
    struct mat44 proj = mat44_func_ortho_proj(0.f, (real32)GAME_GRID_WIDTH, 0.f, (real32)GAME_GRID_HEIGHT, 0.f, 100.f);

    struct vec4 camPos = {0.f, 0.f, 1.f, 1.f};
    struct vec4 camDir = {0.f, 0.f, -1.f, 1.f};
    struct vec4 camRight = {1.f, 0.f, 0.f, 1.f};
    struct vec4 camUp = {0.f, 1.f, 0.f, 1.f};

    struct mat44 view = mat44_func_lookat(&camPos, &camDir, &camRight, &camUp);

    // TODO: couple this with game_cycle loop
    for (i32 entityIndex = 0; entityIndex < g->entityCount; ++entityIndex)
    {
        struct game_entity *entity = &g->entities[entityIndex];
        struct game_layer *layerPtr = basic_dict_get(g->layerDict, g->layers, entity->layer);

        if (entity->isActive)
        {
            struct game_transform *transform = game_get_component(g, entity->id, GAME_TRANSFORM);
            struct game_render_component *renderComp = game_get_component(g, entity->id, GAME_RENDER_COMPONENT);

            if (transform && renderComp)
            {
                renderer_id meshId = renderer_model_get_mesh(g->app->renderContext, renderComp->rendererModelId);

                struct vec4 position = {transform->position.x, transform->position.y, 0.f, 1.f};
                struct vec4 pivot = {transform->pivot.x, transform->pivot.y, 0.f, 1.f};
                struct vec4 scalar = {transform->scale.x, transform->scale.y, 1.f, 1.f};

                //real32 boundsWidth = meshBounds->right - meshBounds->left;
                //real32 boundsHeight = meshBounds->top - meshBounds->bottom;
                //real32 boundsDepth = meshBounds->front - meshBounds->back;

                //struct vec4 offset = {
                //    (-meshBounds->left*boundsWidth*scalar.x - pivot.x*boundsWidth*scalar.x)/scalar.x, (-meshBounds->bottom*boundsHeight*scalar.y - pivot.y*boundsHeight*scalar.y)/scalar.y, 
                //    0.f, 1.f
                //};

                struct mat44 translation = mat44_func_translation(&position);
                struct mat44 scale = mat44_func_scale(&scalar);
                struct mat44 rotation = mat44_func_roll(transform->rotation);
                struct mat44 model = mat44_mult(&scale, &translation);
                model = mat44_mult(&rotation, &model);
                //struct mat44 offsetTranslation = mat44_func_translation(&offset);
                //model = mat44_mult(&offsetTranslation, &model);
                struct mat44 modelView = mat44_mult(&view, &model);
                
                renderer_id materialId = renderer_model_get_material(g->app->renderContext, renderComp->rendererModelId);

                renderer_material_update_property(g->app->renderContext, materialId, "u_Proj", &proj);
                renderer_material_update_property(g->app->renderContext, materialId, "u_ModelView", &modelView);
                renderer_queue_model(g->app->renderContext, renderComp->rendererModelId, entity->layer);
            }
        }
    }

    for (struct interface_element *elementPtr = g->gameInterface->_displayStack;
         elementPtr != NULL; elementPtr = elementPtr->next)
    {
        renderer_id render = *(renderer_id *)basic_dict_get(g->interfaceElementRenderMap, 
            g->interfaceElementRenders, elementPtr->name);

        if (elementPtr->isDirty)
        {
            struct mat44 uiProj = mat44_func_ortho_proj(0.f, (real32)100.f, 0.f, (real32)100.f, 0.f, 1.f);

            struct vec4 uiCamPos = {0.f, 0.f, 1.f, 1.f};
            struct vec4 uiCamDir = {0.f, 0.f, -1.f, 1.f};
            struct vec4 uiCamRight = {1.f, 0.f, 0.f, 1.f};
            struct vec4 uiCamUp = {0.f, 1.f, 0.f, 1.f};

            struct mat44 uiView = mat44_func_lookat(&uiCamPos, &uiCamDir, &uiCamRight, &uiCamUp);
            
            struct vec4 elPos = {elementPtr->x, elementPtr->y, 0.f, 1.f};
            
            struct mat44 uiTranslation = mat44_func_translation(&elPos);
            struct mat44 uiModelView = mat44_mult(&view, &uiTranslation);

            struct vec4 elColor = {(real32)(elementPtr->color & 0xFF)/255.f,
                (real32)((elementPtr->color >> 8) & 0xFF)/255.f,
                (real32)((elementPtr->color >> 16) & 0xFF)/255.f,
                (real32)((elementPtr->color >> 24) & 0xFF)/255.f};

            renderer_id mat = renderer_model_get_material(g->app->renderContext, render);
            renderer_material_update_property(g->app->renderContext, mat, "u_Width", &elementPtr->width);
            renderer_material_update_property(g->app->renderContext, mat, "u_Height", &elementPtr->height);
            u32 windowWidth = SCREEN_WIDTH;
            u32 windowHeight = SCREEN_HEIGHT;
            renderer_material_update_property(g->app->renderContext, mat, "u_WindowWidth", &windowWidth);
            renderer_material_update_property(g->app->renderContext, mat, "u_WindowHeight", &windowHeight);
            renderer_material_update_property(g->app->renderContext, mat, "u_Color", &elColor._[0]);
            renderer_material_update_property(g->app->renderContext, mat, "u_ModelView", &uiModelView._[0]);
            renderer_material_update_property(g->app->renderContext, mat, "u_Proj", &uiProj._[0]);

            for (i32 patternIndex = 0; patternIndex < elementPtr->patternCount; ++patternIndex)
            {
                struct interface_element_pattern *patternPtr = &elementPtr->patterns[patternIndex];

                switch(patternPtr->type)
                {
                    case INTERFACE_ELEMENT_PATTERN_BORDER:
                    {
                        b32 isBorder = B32_TRUE;
                        renderer_material_update_property(g->app->renderContext, mat, "u_IsBorder", &isBorder);
                        renderer_material_update_property(g->app->renderContext, mat, "u_BorderThickness", &patternPtr->border.pixelThickness);
                        struct vec4 borderColor = {(real32)(patternPtr->border.color & 0xFF)/255.f,
                            (real32)((patternPtr->border.color >> 8) & 0xFF)/255.f,
                            (real32)((patternPtr->border.color >> 16) & 0xFF)/255.f,
                            (real32)((patternPtr->border.color >> 24) & 0xFF)/255.f};

                        renderer_material_update_property(g->app->renderContext, mat, "u_BorderColor", &borderColor._[0]);
                    } break;
                    
                    case INTERFACE_ELEMENT_PATTERN_GRADIENT:
                    {
                        b32 isGradient = B32_TRUE;
                        renderer_material_update_property(g->app->renderContext, mat, "u_IsGradient", &isGradient);
                        struct vec4 bottomLeft = {(real32)(patternPtr->gradient.bottomLeftColor & 0xFF)/255.f,
                            (real32)((patternPtr->gradient.bottomLeftColor >> 8) & 0xFF)/255.f,
                            (real32)((patternPtr->gradient.bottomLeftColor >> 16) & 0xFF)/255.f,
                            (real32)((patternPtr->gradient.bottomLeftColor >> 24) & 0xFF)/255.f};
                        renderer_material_update_property(g->app->renderContext, mat, "u_GradientBottomLeftColor", &bottomLeft._[0]);
                        struct vec4 bottomRight = {(real32)(patternPtr->gradient.bottomRightColor & 0xFF)/255.f,
                            (real32)((patternPtr->gradient.bottomRightColor >> 8) & 0xFF)/255.f,
                            (real32)((patternPtr->gradient.bottomRightColor >> 16) & 0xFF)/255.f,
                            (real32)((patternPtr->gradient.bottomRightColor >> 24) & 0xFF)/255.f};
                        renderer_material_update_property(g->app->renderContext, mat, "u_GradientBottomRightColor", &bottomRight._[0]);
                        struct vec4 topRight = {(real32)(patternPtr->gradient.topRightColor & 0xFF)/255.f,
                            (real32)((patternPtr->gradient.topRightColor >> 8) & 0xFF)/255.f,
                            (real32)((patternPtr->gradient.topRightColor >> 16) & 0xFF)/255.f,
                            (real32)((patternPtr->gradient.topRightColor >> 24) & 0xFF)/255.f};
                        renderer_material_update_property(g->app->renderContext, mat, "u_GradientTopRightColor", &topRight._[0]);
                        struct vec4 topLeft = {(real32)(patternPtr->gradient.topLeftColor & 0xFF)/255.f,
                            (real32)((patternPtr->gradient.topLeftColor >> 8) & 0xFF)/255.f,
                            (real32)((patternPtr->gradient.topLeftColor >> 16) & 0xFF)/255.f,
                            (real32)((patternPtr->gradient.topLeftColor >> 24) & 0xFF)/255.f};
                        renderer_material_update_property(g->app->renderContext, mat, "u_GradientTopLeftColor", &topLeft._[0]);
                    } break;
                    
                    case INTERFACE_ELEMENT_PATTERN_TEXTURE:
                    {
                        b32 isTex = B32_TRUE;
                        renderer_material_update_property(g->app->renderContext, mat, "u_IsTex", &isTex);
                        u32 texWidth = 8;
                        renderer_material_update_property(g->app->renderContext, mat, "u_TexWidth", &texWidth);
                        u32 texHeight = 8;
                        renderer_material_update_property(g->app->renderContext, mat, "u_TexHeight", &texHeight);
                    } break;
                }
            }

            elementPtr->isDirty = B32_FALSE;
        }

        renderer_queue_model(g->app->renderContext, render, "ui");
    }
}

game_id
game_create_entity(struct game *g, const char *layer)
{
    const i32 entityIndex = g->entityCount;

    if (g->entities)
    {
        g->entities = memory_realloc(g->app->memoryContext, g->entities, sizeof(struct game_entity)*(++g->entityCount));
    }
    else
    {
        g->entities = memory_alloc(g->app->memoryContext, sizeof(struct game_entity)*(++g->entityCount));
    }

    g->entities[entityIndex].id = entityIndex;
    g->entities[entityIndex].renderComponent = GAME_NULL_ID;
    g->entities[entityIndex].transform = GAME_NULL_ID;
    g->entities[entityIndex].physicsComponent = GAME_NULL_ID;
    g->entities[entityIndex].isActive = B32_TRUE;

    if (basic_dict_get(g->layerDict, g->layers, layer))
    {
        g->entities[entityIndex].layer = memory_alloc(g->app->memoryContext, strlen(layer) + 1);
        sprintf((char *)g->entities[entityIndex].layer, "%s", layer);
    }
    else
    {
        g->entities[entityIndex].layer = memory_alloc(g->app->memoryContext, sizeof("default"));
        sprintf((char *)g->entities[entityIndex].layer, "%s", "default");
    }

    return entityIndex;
}

game_id
game_create_component(struct game *g, enum game_component_type type)
{
    const i32 componentIndex = g->componentCount;

    if (g->components)
    {
        g->components = memory_realloc(g->app->memoryContext, g->components, sizeof(struct game_component_index)
						*(++g->componentCount));
    }
    else
    {
        g->components = memory_alloc(g->app->memoryContext, sizeof(struct game_component_index)
						*(++g->componentCount));
    }

    g->components[componentIndex].type = type;

    switch (type)
    {
        case GAME_TRANSFORM:
        {
            const i32 transformIndex = g->transformCount;

            if (g->transforms)
            {
                g->transforms = memory_realloc(g->app->memoryContext, g->transforms, sizeof(struct game_transform)
										*(++g->transformCount));
            }
            else
            {
                g->transforms = memory_alloc(g->app->memoryContext, sizeof(struct game_transform)
										*(++g->transformCount));
            }

            g->transforms[transformIndex].id = transformIndex;
            g->transforms[transformIndex].position.x = 0.f;
            g->transforms[transformIndex].position.y = 0.f;
            g->transforms[transformIndex].pivot.x = 0.f;
            g->transforms[transformIndex].pivot.y = 0.f;
            g->transforms[transformIndex].scale.x = 1.f;
            g->transforms[transformIndex].scale.y = 1.f;
            g->transforms[transformIndex].rotation = 0.f;
            g->transforms[transformIndex].isDirty = B32_TRUE;
            
            g->components[componentIndex].index = transformIndex;
        } break;
        
        case GAME_RENDER_COMPONENT:
        {
            const i32 renderIndex = g->renderComponentCount;

            if (g->renderComponents)
            {
                g->renderComponents = memory_realloc(g->app->memoryContext, g->renderComponents, 
                    sizeof(struct game_render_component)*(++g->renderComponentCount));
            }
            else
            {
                g->renderComponents = memory_alloc(g->app->memoryContext, sizeof(struct game_render_component)*(++g->renderComponentCount));
            }

            g->renderComponents[renderIndex].id = renderIndex;
            g->renderComponents[renderIndex].rendererModelId = renderer_instantiate_model(g->app->renderContext, "texture_quad", "unlit_texture");
            g->components[componentIndex].index = renderIndex;

            u32 width = 25;
            renderer_material_update_property(g->app->renderContext, renderer_model_get_material(g->app->renderContext,
                g->renderComponents[renderIndex].rendererModelId), "u_TexWidth", &width);
            
            u32 height = 25;
            renderer_material_update_property(g->app->renderContext, renderer_model_get_material(g->app->renderContext,
                g->renderComponents[renderIndex].rendererModelId), "u_TexHeight", &height);

            real32 color[] = {1.f, 1.f, 1.f, 1.f};
            renderer_material_update_property(g->app->renderContext, renderer_model_get_material(g->app->renderContext,
                g->renderComponents[renderIndex].rendererModelId), "u_Color", color);

            renderer_material_set_texture(g->app->renderContext, renderer_model_get_material(g->app->renderContext, 
                g->renderComponents[renderIndex].rendererModelId), "test1");
        } break;
        
        case GAME_PHYSICS_COMPONENT:
        {
            const i32 physicsIndex = g->physicsComponentCount;

            if (g->physicsComponents)
            {
                g->physicsComponents = memory_realloc(g->app->memoryContext, g->physicsComponents, 
                    sizeof(struct game_physics_component)*(++g->physicsComponentCount));
            }
            else
            {
                g->physicsComponents = memory_alloc(g->app->memoryContext, 
                    sizeof(struct game_physics_component)*(++g->physicsComponentCount));
            }

            g->physicsComponents[physicsIndex].id = physicsIndex;
            g->physicsComponents[physicsIndex].rigidbody = physics_create_rigidbody(g->app->physics);
            g->physicsComponents[physicsIndex].isActive = B32_TRUE;

            g->components[componentIndex].index = physicsIndex;
        } break;

        default: break;
    }

    return componentIndex;
}

b32
game_attach_component(struct game *g, game_id entity, game_id component)
{
    assert(entity > GAME_NULL_ID && entity < g->entityCount);
    assert(component > GAME_NULL_ID && component < g->componentCount);

    b32 success;
    switch (g->components[component].type)
    {
        case GAME_TRANSFORM:
        {
            if (g->entities[entity].transform == GAME_NULL_ID)
            {
                g->entities[entity].transform = component;
                success = B32_TRUE;
            }
        } break;

        case GAME_RENDER_COMPONENT:
        {
            if (g->entities[entity].renderComponent == GAME_NULL_ID)
            {
                g->entities[entity].renderComponent = component;
                success = B32_TRUE;
            }
        } break;
        
        case GAME_PHYSICS_COMPONENT:
        {
            if (g->entities[entity].physicsComponent == GAME_NULL_ID)
            {
                g->entities[entity].physicsComponent = component;
                success = B32_TRUE;
            }
        } break;

        default: 
        {
            success = B32_FALSE;
        } break;
    }

    return success;
}

void *
game_get_component(struct game *g, game_id entity, enum game_component_type type)
{
    assert(entity > GAME_NULL_ID && entity < g->entityCount);

    struct game_entity *entityPtr = &g->entities[entity];

    switch (type)
    {
        case GAME_TRANSFORM:
        {
            if (entityPtr->transform != GAME_NULL_ID)
            {
                return &g->transforms[g->components[entityPtr->transform].index];
            }
        } break;
        
        case GAME_RENDER_COMPONENT:
        {
            if (entityPtr->renderComponent != GAME_NULL_ID)
            {
                return &g->renderComponents[g->components[entityPtr->renderComponent].index];
            }
        } break;
        
        case GAME_PHYSICS_COMPONENT:
        {
            if (entityPtr->physicsComponent != GAME_NULL_ID)
            {
                return &g->physicsComponents[g->components[entityPtr->physicsComponent].index];
            }
        } break;

        default: break;
    }

    return NULL;
}

b32
game_instantiate_interface_element(struct game *g, const char *name, 
    struct interface_element_pattern *patterns, i32 patternCount, real32 x, real32 y, 
    real32 width, real32 height, u32 color)
{
    assert(g);

    if (!basic_dict_get(g->interfaceElementRenderMap, g->interfaceElementRenders, name))
    {
        i32 renderIndex = g->gameInterface->_elementCount;

        if (g->gameInterface->_elementCount > 0)
        {
            if (g->gameInterface->_elementCount >= g->gameInterface->_elementCapacity)
            {
                g->interfaceElementRenders = memory_realloc(g->app->memoryContext, g->interfaceElementRenders,
                    sizeof(renderer_id)*(g->gameInterface->_elementCapacity + 10));
            }
        }
        else
        {
            g->interfaceElementRenders = memory_alloc(g->app->memoryContext,
                sizeof(renderer_id)*10);
        }

        interface_create_element(g->gameInterface, name, patterns, patternCount, x, y, width, height, color);
        
        g->interfaceElementRenders[renderIndex] = 
            renderer_instantiate_model(g->app->renderContext, "interface_quad", "interface");

        renderer_material_set_texture(g->app->renderContext, renderer_model_get_material(g->app->renderContext, g->interfaceElementRenders[renderIndex]), "test1");
        basic_dict_set(g->interfaceElementRenderMap, g->app->memoryContext, g->interfaceElementRenders, 
            name, strlen(name) + 1, &g->interfaceElementRenders[renderIndex]);

        interface_target_element(g->gameInterface, name);

        return B32_TRUE;
    }

    return B32_FALSE;
}

void
game_start_timer(struct game *g, real32 seconds, game_timer_callback callback, void *dataPtr)
{
    if (g->activeTimerCount >= g->timerCount)
    {
        g->timers = memory_realloc(g->app->memoryContext, g->timers, sizeof(struct game_timer)*(++g->timerCount));
    }

    i32 index = g->activeTimerCount++;

    g->timers[index].startTime = g->app->msSinceStart;
    g->timers[index].duration = (u32)(seconds*1000.f);
    g->timers[index].callback = callback;
    g->timers[index].dataPtr = dataPtr;
    g->timers[index].isActive = B32_TRUE;
}

void
game_init_layer(struct game *g, const char *name, u32 layer)
{
    if (!basic_dict_get(g->layerDict, g->layers, name))
    {
        const u32 index = g->layerCount;

        if (g->layerCount > 0)
        {
            g->layers = memory_realloc(g->app->memoryContext, g->layers, sizeof(struct game_layer)
								*(++g->layerCount));
            assert(g->layers);
        }
        else
        {
            g->layers = memory_alloc(g->app->memoryContext, sizeof(struct game_layer)
								*(++g->layerCount));
        }

        const size_t nameSize = strlen(name) + 1;

        g->layers[index].layer = layer;
        g->layers[index].name = memory_alloc(g->app->memoryContext, nameSize);
        sprintf((char *)g->layers[index].name, "%s", name);

        basic_dict_set(g->layerDict, g->app->memoryContext, g->layers, g->layers[index].name, nameSize, &g->layers[index]);
    }
    else
    {
        fprintf(stderr, "game_init_layer error: Layer '%s' already exists!\n", name);
    }
}

b32
game_load_texture(struct game *g, const char *name)
{
    return B32_TRUE;
}
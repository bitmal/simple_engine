#include "game.h"
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
#include "config.h"
#include "interface.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>

// TODO: TEST INTERFACE
// ***************************************

//#undef TEST_ENTITY_ENABLED
#define TEST_ENTITY_ENABLED

#ifdef TEST_ENTITY_ENABLED
	#define TEST_ENTITY_INIT_COUNT 1
	#define TEST_ENTITY_ANIMATION_TIMESTEP .0625f
	#define TEST_ENTITY_ANIMATION_FLAG_MIRROR 0x1b
	#define TEST_ENTITY_ANIMATION_FLAG_LOOP 0x10b
	
	#define TEST_ENTITY_0_SCALE 1.f
	#define TEST_ENTITY_0_ANIMATION_DURATION 2.f
	#define TEST_ENTITY_0_ANIMATION_START_POS {0.f, 0.f}
	#define TEST_ENTITY_0_ANIMATION_END_POS \
	{ \
		50.f, \
		50.f \
	}
	#define TEST_ENTITY_0_ANIMATION_START_COLOR \
	{ \
		1.f, \
		0.f, \
		1.f, \
		1.f \
	}
	#define TEST_ENTITY_0_ANIMATION_END_COLOR \
	{ \
		1.f, \
		1.f, \
		0.f, \
		1.f \
	}
	#define TEST_ENTITY_0_ANIMATION_FLAGS \
	( \
		TEST_ENTITY_ANIMATION_FLAG_MIRROR | \
		TEST_ENTITY_ANIMATION_FLAG_LOOP \
	)
	
	enum test_entity_animation_curve_type
	{
		TEST_ENTITY_ANIMATION_CURVE_LINEAR,
		TEST_ENTITY_ANIMATION_CURVE_QUADRATIC
	};
	
	struct test_entity
	{
		game_id id;
		game_id transform;
		game_id renderComp;
	
		struct
		{
			u32 flags;
			enum test_entity_animation_curve_type curveType;
			real32 durationSeconds;
			real32 elapsedSeconds;
			struct vec2 startPos;
			struct vec2 endPos;
			struct vec4 startColor;
			struct vec4 endColor;
		} anim;
	} g_testEntities[TEST_ENTITY_INIT_COUNT];
#endif

// **************************************

static void 
_game_input_callback(const char *key, const char *action, void *dataPtr)
{
    struct game *g = (struct game *)dataPtr;
    const b32 isKeyDown = !strcmp(action, "KEYDOWN");

    if (!strcmp(key, "escape") && !isKeyDown)
    {
        g->app->isRunning = B32_FALSE;
    }
}

#ifdef TEST_ENTITY_ENABLED
	static void
	_game_test_entity_0_timer_callback(struct game *g, real32 elapsedTime, void *dataPtr)
	{
		b32 isValid = B32_FALSE;
	
		struct test_entity *entity;
		struct game_transform *transform;
		struct game_render_component *renderComp;
		{
			assert(entity = dataPtr);
			assert(transform = game_get_component(g, entity->id, GAME_TRANSFORM));
			assert(renderComp = game_get_component(g, entity->id, GAME_RENDER_COMPONENT));
	
			isValid = B32_TRUE;
		}
	
		// calculate the interpolation result of the animation, 
		// according to a normalized scalar, known as 't'
		if (isValid)
		{
			// determine in a normalized domain, the value of t
			real32 d = entity->anim.durationSeconds;
			real32 t0 = entity->anim.elapsedSeconds;
			real32 t1 = t0 + elapsedTime;
			{
				t1 = (t1 >= d) ? d : t1;
				entity->anim.elapsedSeconds = t1;
			}
			
			real32 t = t1/d;
	
			// transformation(s)
			struct vec2 translationTotalVec = {entity->anim.endPos.x, entity->anim.endPos.y};
			{
				vec2_diff(&translationTotalVec, &entity->anim.startPos);
			}
	
			transform->position.x = entity->anim.startPos.x + (translationTotalVec.x*t);
			transform->position.y = entity->anim.startPos.y + (translationTotalVec.y*t);
			transform->isDirty = B32_TRUE;
	
			// color interpolation(s)
			struct vec4 interpolationVec = 
			{
				entity->anim.endColor.r, entity->anim.endColor.g,
				entity->anim.endColor.b, entity->anim.endColor.a
			};
			{
				vec4_3_diff(&interpolationVec, &entity->anim.startColor);
			}
	
			struct vec4 resultColor = 
			{
				entity->anim.startColor.r + (interpolationVec.r*t),
				entity->anim.startColor.g + (interpolationVec.g*t),
				entity->anim.startColor.b + (interpolationVec.b*t),
				entity->anim.startColor.a + (interpolationVec.a*t)
			};
	
			renderer_material_update_property(g->app->renderContext, renderComp->rendererModelId, 
					"u_Color", &resultColor._[0]);
	
			b32 isLooping = entity->anim.flags & TEST_ENTITY_ANIMATION_FLAG_LOOP;
	
			if ((t < 1.f) || isLooping)
			{
				if ((t == 1.f) && isLooping)
				{
					entity->anim.elapsedSeconds = 0.f;
				}
	
				game_start_timer(g, TEST_ENTITY_ANIMATION_TIMESTEP, _game_test_entity_0_timer_callback, 
						&g_testEntities[0]);
			}
		}
	}
#endif

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

    struct vec4 bgColor = {0.f, 0.f, 0.1f, 1.f};
    renderer_set_background_color(app->renderContext, &bgColor);
    
    renderer_load_program(app->renderContext, "unlit");
    renderer_load_program(app->renderContext, "unlit_frame");
    //renderer_load_program(app->renderContext, "unlit_texture");
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
    
    real32 unlitQuadVertices[] = {
        0.f, 0.f, 0.f, 1.f,   1.f, 1.f, 1.f, 1.f,
        1.f, 0.f, 0.f, 1.f,   1.f, 1.f, 1.f, 1.f,
        1.f, 1.f, 0.f, 1.f,   1.f, 1.f, 1.f, 1.f,
        0.f, 1.f, 0.f, 1.f,   1.f, 1.f, 1.f, 1.f
    };

    u16 unlitQuadIndices[] = {
        0, 1, 3,
        1, 2, 3
    };

    renderer_create_mesh(app->renderContext, "unlit_quad", unlitQuadVertices, sizeof(real32)*8, 
        4, unlitQuadIndices, 6);
    
    real32 interfaceQuadVertices[] = {
        0.f, 0.f, 0.f, 1.f,   1.f, 1.f, 1.f, 1.f,   0.f, 0.f,
        1.f, 0.f, 0.f, 1.f,   1.f, 1.f, 1.f, 1.f,   1.f, 0.f,
        1.f, 1.f, 0.f, 1.f,   1.f, 1.f, 1.f, 1.f,   1.f, 1.f,
        0.f, 1.f, 0.f, 1.f,   1.f, 1.f, 1.f, 1.f,   0.f, 1.f
    };

    u16 interfaceQuadIndices[] = {
        0, 1, 3,
        1, 2, 3
    };

    renderer_create_mesh(app->renderContext, "interface_quad", interfaceQuadVertices, sizeof(real32)*10, 
        4, interfaceQuadIndices, 6);
    
    real32 frameQuadVertices[] = {
        0.f, 0.f, 0.f, 1.f,   1.f, 1.f, 1.f, 1.f,   1.f, 0.f, 0.f,
        1.f, 0.f, 0.f, 1.f,   1.f, 1.f, 1.f, 1.f,   0.f, 1.f, 0.f,
        1.f, 1.f, 0.f, 1.f,   1.f, 1.f, 1.f, 1.f,   1.f, 1.f,
        0.f, 1.f, 0.f, 1.f,   1.f, 1.f, 1.f, 1.f,   0.f, 1.f
    };

    u16 frameQuadIndices[] = {
        0, 1, 3,
        1, 2, 3
    };

    renderer_create_mesh(app->renderContext, "frame_quad", interfaceQuadVertices, sizeof(real32)*10, 
        4, interfaceQuadIndices, 6);

    g->gameInterface = interface_init(g->app->memoryContext);
    g->interfaceElementRenderMap = DICTIONARY(g->app->memoryContext, NULL);

		// TODO: (test startup code here)

#ifdef TEST_ENTITY_ENABLED		
		// instantiate and configure the entity
		struct test_entity *entity0 = &g_testEntities[0];
		{
			entity0->id = game_create_entity(g, "default");

			// animation values & flags
			{
				entity0->anim.flags = TEST_ENTITY_0_ANIMATION_FLAGS;
				entity0->anim.durationSeconds = TEST_ENTITY_0_ANIMATION_DURATION;
				entity0->anim.elapsedSeconds = 0.f;
				struct vec2 startPos = TEST_ENTITY_0_ANIMATION_START_POS;
				entity0->anim.startPos.x = startPos.x;
				entity0->anim.startPos.y = startPos.y;
				struct vec2 endPos = TEST_ENTITY_0_ANIMATION_END_POS;
				entity0->anim.endPos.x = endPos.x;
				entity0->anim.endPos.y = endPos.y;
				struct vec4 startColor = TEST_ENTITY_0_ANIMATION_START_COLOR;
				entity0->anim.startColor.r = startColor.r;
				entity0->anim.startColor.g = startColor.g;
				entity0->anim.startColor.b = startColor.b;
				entity0->anim.startColor.a = startColor.a;
				struct vec4 endColor = TEST_ENTITY_0_ANIMATION_END_COLOR;
				entity0->anim.endColor.r = endColor.r;
				entity0->anim.endColor.g = endColor.g;
				entity0->anim.endColor.b = endColor.b;
				entity0->anim.endColor.a = endColor.a;
			}

			// transform values & flags
			entity0->transform = game_create_component(g, GAME_TRANSFORM);
			game_attach_component(g, entity0->id, entity0->transform);

			struct game_transform *entity0Transf = game_get_component(g, entity0->id, 
					GAME_TRANSFORM);
			{
				if (!entity0Transf)
				{
					fprintf(stderr, "Did not get transform component!\n");
				}
				entity0Transf->position.x = entity0->anim.startPos.x;
				entity0Transf->position.y = entity0->anim.startPos.y;
				entity0Transf->pivot.x = 0.f;
				entity0Transf->pivot.y = 0.f;
				entity0Transf->scale.x = TEST_ENTITY_0_SCALE;
				entity0Transf->scale.y = TEST_ENTITY_0_SCALE;
				entity0Transf->rotation = 0.f;
			}
		
			// renderer values & flags
			entity0->renderComp = game_create_component(g, GAME_RENDER_COMPONENT);
			game_attach_component(g, entity0->id, entity0->renderComp);

			struct game_render_component *entity0RenderComp = game_get_component(g, 
					entity0->id, GAME_RENDER_COMPONENT);
			{
				if (!entity0RenderComp)
				{
					fprintf(stderr, "Did not get render component!\n");
				}
				renderer_model_set_mesh(app->renderContext, entity0RenderComp->rendererModelId, "unlit_quad");
				renderer_id entity0RenderCompMatId = renderer_model_get_material(app->renderContext,
						entity0RenderComp->rendererModelId);
				renderer_material_set_program(app->renderContext, entity0RenderCompMatId, "unlit");
				renderer_material_update_property(app->renderContext, entity0RenderCompMatId, 
						"u_Color", &entity0->anim.startColor._[0]);
				float entity0Width = 20.f;
				renderer_material_update_property(app->renderContext, entity0RenderCompMatId, 
						"u_Width", &entity0Width);
				float entity0Height = 20.f;
				renderer_material_update_property(app->renderContext, entity0RenderCompMatId, 
						"u_Height", &entity0Height);
			}
		}

		// animate the entity
		game_start_timer(g, TEST_ENTITY_ANIMATION_TIMESTEP, _game_test_entity_0_timer_callback, &g_testEntities[0]);
#endif
		
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
                g->timers[timerIndex].callback(g, (real32)time/1000.f, g->timers[timerIndex].dataPtr);
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
        		    struct physics_rigidbody *rb = physics_get_rigidbody(g->app->physics, physicsComp->rigidbody);
        		    transform->position.x = rb->position[0];
        		    transform->position.y = rb->position[1];
        		    transform->rotation = rb->rotation[2];
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
            g->renderComponents[renderIndex].rendererModelId = renderer_instantiate_model(g->app->renderContext, "unlit_quad", "unlit");
            g->components[componentIndex].index = renderIndex;

            real32 width = 1.f;
            renderer_material_update_property(g->app->renderContext, renderer_model_get_material(g->app->renderContext,
                g->renderComponents[renderIndex].rendererModelId), "u_Width", &width);
            
            real32 height = 1.f;
            renderer_material_update_property(g->app->renderContext, renderer_model_get_material(g->app->renderContext,
                g->renderComponents[renderIndex].rendererModelId), "u_Height", &height);

            real32 color[] = {1.f, 1.f, 1.f, 1.f};
            renderer_material_update_property(g->app->renderContext, renderer_model_get_material(g->app->renderContext,
                g->renderComponents[renderIndex].rendererModelId), "u_Color", color);
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

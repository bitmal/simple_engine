#include "constants.h"
#include "memory.h"
#include "utils.h"
#include <SDL3/SDL_oldnames.h>
#define MEMORY_DEBUG

#include "engine.h"

#include <GL/glew.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_timer.h>

//#include <dlfcn.h>

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include "cimgui.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "memory.c"
#include "utils.c"
#include "config.c"
#include "vec2.c"
#include "vec4.c"
#include "mat44.c"
#include "mat44_func.c"
#include "basic_dict.c"
#include "input.c"
#include "physics_helpers.c"
#include "physics.c"
#include "opengl.c"
#include "renderer.c"
#include "interface.c"
#include "game.c"
#include "statistics.c"

int 
main(int argc, char **argv)
{
    // TODO: set seed to possibly some randomly generated value at startup, from gathered and (stored to disk) noise/entropy
    // from A. The Graphics Card or B. The CPU. Fallback could be some default seed value lookup table and result coupled with
    // date/time for the seed result hash
    utils_set_random_seed(0);

    #if 0
    void *cimguiPlugin = dlopen("libcimgui.so", RTLD_NOW);
    if (!cimguiPlugin)
    {
        fprintf(stderr, "Failure to load %s at runtime!\ndl_error: %s\n", "libcimgui", dlerror());

        return -1;
    }

    void *init = dlsym(cimguiPlugin, "init");

    const char *result = dlerror();
    if (result)
    {
        fprintf(stderr, "Failure to load init at runtime!\ndl_error: %s\n", result);
        return -1;
    }
    #endif

    const struct memory_context_key configMemoryKey;
    {
        memory_error_code resultCode;

        if ((resultCode = memory_create_debug_context(MEMORY_SAFE_PTR_REGION_SIZE, 
            CONFIG_MEMORY_SIZE, MEMORY_LABEL_REGION_SIZE, "config", &configMemoryKey))
            != MEMORY_OK)
        {
            return -1;
        }
    }

    struct context app;
    
    if (!config_init(&configMemoryKey, CONFIG_MEMORY_SIZE, &app.configKey))
    {
        return -1;
    }
    
    if (!config_load(&app.configKey, "system"))
    {
        return -1;
    }
    
    if (!config_load(&app.configKey, "game"))
    {
        return -1;
    }
    
    if (!config_load(&app.configKey, "physics"))
    {
        return -1;
    }
    
#if 0
    const struct memory_context_key statsMemoryKey;
    {
        memory_error_code resultCode;

        if ((resultCode = memory_create_debug_context(MEMORY_SAFE_PTR_REGION_SIZE, 
            CONFIG_MEMORY_SIZE, MEMORY_LABEL_REGION_SIZE, "stats", &statsMemoryKey))
            != MEMORY_OK)
        {
            return -1;
        }
    }

	if (!statistics_init(&app, &statsMemoryKey))
    {
        return -1;
    }
#endif
    
    const struct memory_context_key gameMemoryKey;
    {
        memory_error_code resultCode;

        if ((resultCode = memory_create_debug_context(MEMORY_SAFE_PTR_REGION_SIZE, 
            GAME_MEMORY_SIZE, MEMORY_LABEL_REGION_SIZE, "game", &gameMemoryKey))
            != MEMORY_OK)
        {
            return -1;
        }
    }
    
    const struct memory_context_key physicsMemoryKey;
    {
        memory_error_code resultCode;

        if ((resultCode = memory_create_debug_context(MEMORY_SAFE_PTR_REGION_SIZE, 
            PHYSICS_MEMORY_SIZE, MEMORY_LABEL_REGION_SIZE, "physics", &physicsMemoryKey))
            != MEMORY_OK)
        {
            return -1;
        }
    }

    const struct memory_context_key graphicsMemoryKey;
    {
        memory_error_code resultCode;

        if ((resultCode = memory_create_debug_context(MEMORY_SAFE_PTR_REGION_SIZE, 
            GRAPHICS_MEMORY_SIZE, MEMORY_LABEL_REGION_SIZE, "graphics", 
            &graphicsMemoryKey)) != MEMORY_OK)
        {
            return -1;
        }
    }
    
    const struct memory_context_key inputMemoryKey;
    {
        memory_error_code resultCode;

        if ((resultCode = memory_create_debug_context(MEMORY_SAFE_PTR_REGION_SIZE, 
            INPUT_MEMORY_SIZE, MEMORY_LABEL_REGION_SIZE, "input", &inputMemoryKey))
            != MEMORY_OK)
        {
            return -1;
        }
    }

    ImGuiContext *imguiContext = igCreateContext(NULL);
    
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);

    SDL_Window *window = SDL_CreateWindow("Simple Engine", 640, 480, 
        SDL_WINDOW_OPENGL);

    SDL_GLContext gl = SDL_GL_CreateContext(window);

    if (glewInit() != GLEW_OK)
    {
        fprintf(stderr, "GLEW could not init!\n");
        return -1;
    }
    
    SDL_ShowWindow(window);

    app.inputKey; input_init(&app.inputKey, NULL);
    app.isRunning = B32_TRUE;
    app.msSinceStart = SDL_GetTicks();
    app.physicsKey = physics_init(&physicsMemoryKey);
    app.renderContext = renderer_create_context(&graphicsMemoryKey);

    struct game *gameContext = game_init(&app);
    app.inputContext->dataPtr = gameContext;

    u64 previousTicksWide = SDL_GetTicks64();
    u32 previousTicks = SDL_GetTicks64();
    u64 elapsedTicksWide = 0;
    u64 elapsedNS = 0;
    u32 elapsedMS = 0;

    utils_set_elapsed_time_ns_ptr(&elapsedNS);
    utils_set_elapsed_time_ms_ptr(&elapsedMS);

    while (app.isRunning)
    {
        SDL_Event ev;
        while (SDL_PollEvent(&ev))
        {
            if (ev.key.type == SDL_EVENT_KEY_DOWN)
            {
                struct input_key *key = NULL;

                switch (ev.key.key)
                {
                    case SDLK_ESCAPE:
                    {
                        key = basic_dict_get(app.inputContext->keyDict, app.inputContext->keys, "escape");
                    } break;

                    case SDLK_SPACE:
                    {
                        key = basic_dict_get(app.inputContext->keyDict, app.inputContext->keys, "space");
                    } break;

                    case SDLK_A:
                    {
                        key = basic_dict_get(app.inputContext->keyDict, app.inputContext->keys, "a");
                    } break;

                    case SDLK_D:
                    {
                        key = basic_dict_get(app.inputContext->keyDict, app.inputContext->keys, "d");
                    } break;

                    case SDLK_S:
                    {
                        key = basic_dict_get(app.inputContext->keyDict, app.inputContext->keys, "s");
                    } break;

                    case SDLK_W:
                    {
                        key = basic_dict_get(app.inputContext->keyDict, app.inputContext->keys, "w");
                    } break;
                    
                    case SDLK_F:
                    {
                        key = basic_dict_get(app.inputContext->keyDict, app.inputContext->keys, "f");
                    } break;
                }

                if (key && !key->isDown)
                {
                    key->isDown = B32_TRUE;

                    for (i32 keybind = 0; keybind < app.inputContext->keybindCount; ++keybind)
                    {
                        if (!strcmp(app.inputContext->keybinds[keybind].key, key->key))
                        {
                            app.inputContext->keybinds[keybind].callback(key->key, "KEYDOWN", gameContext);
                        }
                    }
                }
            }
            else if (ev.key.type == SDL_EVENT_KEY_UP)
            {
                struct input_key *key = NULL;

                switch (ev.key.key)
                {
                    case SDLK_ESCAPE:
                    {
                        key = basic_dict_get(app.inputContext->keyDict, app.inputContext->keys, "escape");
                    } break;

                    case SDLK_SPACE:
                    {
                        key = basic_dict_get(app.inputContext->keyDict, app.inputContext->keys, "space");
                    } break;

                    case SDLK_A:
                    {
                        key = basic_dict_get(app.inputContext->keyDict, app.inputContext->keys, "a");
                    } break;

                    case SDLK_D:
                    {
                        key = basic_dict_get(app.inputContext->keyDict, app.inputContext->keys, "d");
                    } break;

                    case SDLK_S:
                    {
                        key = basic_dict_get(app.inputContext->keyDict, app.inputContext->keys, "s");
                    } break;

                    case SDLK_W:
                    {
                        key = basic_dict_get(app.inputContext->keyDict, app.inputContext->keys, "w");
                    } break;
                    
                    case SDLK_F:
                    {
                        key = basic_dict_get(app.inputContext->keyDict, app.inputContext->keys, "f");
                    } break;
                }

                if (key && key->isDown)
                {
                    key->isDown = B32_FALSE;

                    for (i32 keybind = 0; keybind < app.inputContext->keybindCount; ++keybind)
                    {
                        if (!strcmp(app.inputContext->keybinds[keybind].key, key->key))
                        {
                            app.inputContext->keybinds[keybind].callback(key->key, "KEYUP", gameContext);
                            break;
                        }
                    }
                }
            }
        }

        u64 currentTicksWide = SDL_GetTicks64();
        u32 currentTicks = SDL_GetTicks();
        elapsedMS += currentTicks - previousTicks;
        previousTicks = currentTicks;
                
        while (elapsedMS >= MS_PER_FRAME)
        {
            game_cycle(gameContext, 1.f/FRAMES_PER_SEC);
            physics_update(app.physics, 1.f/FRAMES_PER_SEC);
            game_render(gameContext);

            renderer_draw(app.renderContext);
            SDL_GL_SwapWindow(window);

            elapsedMS -= MS_PER_FRAME;
            elapsedMS += MS_PER_FRAME;
            app.msSinceStart += MS_PER_FRAME;
        }
    }

    SDL_Quit();

    return 0;
}
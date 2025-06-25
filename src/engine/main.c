#include "engine.h"

#include <GL/glew.h>
#include <SDL3/SDL.h>

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
#include "physics.c"
#include "opengl.c"
#include "renderer.c"
#include "interface.c"
#include "game.c"
#include "statistics.c"

MEMORY_DEFINE(config, 1 << 14);
MEMORY_DEFINE(game, GAME_MEMORY_SIZE);
MEMORY_DEFINE(physics, PHYSICS_MEMORY_SIZE);
MEMORY_DEFINE(graphics, GRAPHICS_MEMORY_SIZE);

int 
main(int argc, char **argv)
{
    MEMORY_INIT(config);

    struct context app;
    app.config = config_init(MEMORY_PTR(config));
    config_load_config(app.config, "system");

	app.statsContext = statistics_init(&app);

    MEMORY_INIT(game);
    MEMORY_INIT(physics);
    MEMORY_INIT(graphics);
    
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

    app.inputContext = input_init(MEMORY_PTR(game), NULL);
    app.isRunning = B32_TRUE;
    app.memoryContext = MEMORY_PTR(game);
    app.msSinceStart = 0;
    app.physics = physics_init(MEMORY_PTR(physics));
    app.renderContext = renderer_create_context(MEMORY_PTR(graphics));

    struct game *gameContext = game_init(&app);
    app.inputContext->dataPtr = gameContext;

    u32 previousTicks = SDL_GetTicks();
    i32 elapsedTicks = 0;

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

        u32 currentTicks = SDL_GetTicks();
        elapsedTicks += currentTicks - previousTicks;
        previousTicks = currentTicks;
                
        while (elapsedTicks >= MS_PER_FRAME)
        {
            game_cycle(gameContext, 1.f/FRAMES_PER_SEC);
            physics_update(app.physics, 1.f/FRAMES_PER_SEC);
            game_render(gameContext);

            renderer_draw(app.renderContext);
            SDL_GL_SwapWindow(window);

            elapsedTicks -= MS_PER_FRAME;
            app.msSinceStart += MS_PER_FRAME;
        }
    }

    SDL_Quit();

    return 0;
}

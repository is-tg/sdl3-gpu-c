#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include "common.h"
#include "game.h"

bool app_create(void **appstate, AppState **app)
{
    AppState *state = SDL_calloc(1, sizeof(AppState));
    if (!state) {
        SDL_Log("Failed appstate allocation\n%s", SDL_GetError());
        return false;
    }

    *app = state;
    *appstate = state;

    return true;
}

void try_depth_format(SDL_GPUDevice *device, SDL_GPUTextureFormat *depth_texture_format, SDL_GPUTextureFormat format) {
    if (SDL_GPUTextureSupportsFormat(device, format, SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET)) {
        *depth_texture_format = format;
    }
}

bool app_init(AppState *app)
{
    app->window_width = 1280;
    app->window_height = 780;

    app->window = SDL_CreateWindow("title", (int) app->window_width, (int) app->window_height, SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!app->window) {
        SDL_Log("Failed to create window\n%s", SDL_GetError());
        return false;
    }

    app->gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL, true, NULL);
    if (!app->gpu) {
        SDL_Log("Failed to create gpu device\n%s", SDL_GetError());
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(app->gpu, app->window)) {
        SDL_Log("Failed to claim window for gpu device\n%s", SDL_GetError());
        return false;
    }

    SDL_SetGPUSwapchainParameters(app->gpu, app->window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR_LINEAR, SDL_GPU_PRESENTMODE_VSYNC);
    app->swapchain_texture_format = SDL_GetGPUSwapchainTextureFormat(app->gpu, app->window);

    app->depth_texture_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    try_depth_format(app->gpu, &app->depth_texture_format, SDL_GPU_TEXTUREFORMAT_D32_FLOAT);
    try_depth_format(app->gpu, &app->depth_texture_format, SDL_GPU_TEXTUREFORMAT_D24_UNORM);
    // SDL_Log("Using texture format: %d", depth_texture_format);

    SDL_GPUTextureCreateInfo depth_tex_createinfo = {
        .format = app->depth_texture_format,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = app->window_width,
        .height = app->window_height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };
    app->depth_texture = SDL_CreateGPUTexture(app->gpu, &depth_tex_createinfo);

    return true;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    (void) argc;
    (void) argv;

    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("Couldn't initialize SDL");
        return SDL_APP_FAILURE;
    }

    AppState *app;
    if (!app_create(appstate, &app)) {
        SDL_Log("Failed to create app");
        return SDL_APP_FAILURE;
    }

    if (!app_init(app)) {
        SDL_Log("Failed to initialize app");
        return SDL_APP_FAILURE;
    }

    game_init(app);

    SDL_SetWindowRelativeMouseMode(app->window, true);
    app->time.last_ticks = SDL_GetTicks();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    AppState *app = (AppState *)appstate;

    switch (event->type)
    {
        case SDL_EVENT_KEY_DOWN:
            app->key_down[event->key.scancode] = true;
            break;
        case SDL_EVENT_KEY_UP:
            if (event->key.key == SDLK_ESCAPE) {
                return SDL_APP_SUCCESS;
            }
            app->key_down[event->key.scancode] = false;
            break;
        case SDL_EVENT_MOUSE_MOTION:
            app->mouse_move[0] = event->motion.xrel;
            app->mouse_move[1] = event->motion.yrel;
            break;
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState *app = (AppState *) appstate;
    
    TimeState *time = &app->time;
    time->new_ticks = SDL_GetTicks();
    time->delta_time = (float) (time->new_ticks - time->last_ticks) / 1000.0f;
    time->last_ticks = time->new_ticks;

    game_update(app);

    // render
    SDL_GPUCommandBuffer *cmd_buf = SDL_AcquireGPUCommandBuffer(app->gpu);
    if (!cmd_buf) {
        SDL_Log("SDL_AcquireGPUCommandBuffer failed\n%s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GPUTexture *swapchain_tex = NULL;
    SDL_WaitAndAcquireGPUSwapchainTexture(cmd_buf, app->window, &swapchain_tex, NULL, NULL);

    if (swapchain_tex) {
        game_render(app, cmd_buf, swapchain_tex);
    }

    SDL_SubmitGPUCommandBuffer(cmd_buf); 
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    (void) result;

    if (appstate) {
        AppState *app = (AppState *) appstate;

        Model model;
        for (int i = 0; i < app->model_count; i++) {
            model = app->models[i];
            SDL_ReleaseGPUBuffer(app->gpu, model.mesh.vertex_buffer);
            SDL_ReleaseGPUBuffer(app->gpu, model.mesh.index_buffer);
            if (i == 0 || i == 2) {
                SDL_ReleaseGPUTexture(app->gpu, model.texture);
            }
        }

        SDL_ReleaseGPUGraphicsPipeline(app->gpu, app->pipeline);
        SDL_ReleaseGPUSampler(app->gpu, app->sampler);
        SDL_ReleaseGPUTexture(app->gpu, app->depth_texture);

        SDL_ReleaseWindowFromGPUDevice(app->gpu, app->window);
        SDL_DestroyGPUDevice(app->gpu);
        SDL_DestroyWindow(app->window);

        SDL_free(app);
    }
}


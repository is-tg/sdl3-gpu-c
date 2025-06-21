#pragma once

#include <SDL3/SDL.h>
#include "common.h"

#define EYE_HEIGHT 1
#define MOVE_SPEED 5
#define LOOK_SENSITIVITY 0.3f
#define ROTATION_SPEED (90.0f * RAD_PER_DEG)

typedef struct {
    mat4 mvp;
} UniformBufferObject;

typedef struct {
    vec3 pos;
    SDL_FColor color;
    vec2 uv;
} Vertex;

void game_init(AppState *app);
void setup_pipeline(AppState *app);
void game_update(AppState *app);
void game_render(AppState *app, SDL_GPUCommandBuffer *cmd_buf, SDL_GPUTexture *swapchain_tex);
void update_camera(AppState *app, float dt);

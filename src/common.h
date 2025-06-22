#pragma once

#include <SDL3/SDL.h>
#include "lib/linalg.h"

#define MAX_MODELS 4
#define MAX_ENTITIES 8

// linear colors
#define WHITE_COLOR ((SDL_FColor){ 1, 1, 1, 1 })
#define DARK_COLOR  ((SDL_FColor){ 0.01098f, 0.01098f, 0.01385f, 1 })

typedef struct {
    Uint64 last_ticks;
    Uint64 new_ticks;
    float delta_time;
} TimeState;

typedef struct {
    vec3 position;
    vec3 target;
} Camera;

typedef struct {
    float yaw;
    float pitch;
} Look;

typedef struct {
    SDL_GPUBuffer *vertex_buffer;
    SDL_GPUBuffer *index_buffer;
    Uint32 index_count;
} Mesh;

typedef struct {
    Mesh mesh;
    SDL_GPUTexture *texture;
} Model;

typedef int Model_ID;

typedef struct {
    Model_ID model_id;
    vec3 position;
    quat rotation;
} Entity;

typedef struct {
    TimeState time;

    SDL_GPUDevice *gpu;
    SDL_Window *window;
    Uint32 window_width;
    Uint32 window_height;
    SDL_GPUTexture *depth_texture;
    SDL_GPUTextureFormat depth_texture_format;
    SDL_GPUTextureFormat swapchain_texture_format;
    
    SDL_GPUGraphicsPipeline *pipeline;
    SDL_GPUSampler *sampler;

    bool key_down[SDL_SCANCODE_COUNT];
    vec2 mouse_move;
    
    Camera camera;
    Look look;

    SDL_FColor clear_color;
    bool rotate;

    Model models[MAX_MODELS];
    int model_count;
    Entity entities[MAX_ENTITIES];
    int entity_count;

} AppState;


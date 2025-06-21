#pragma once

#include <SDL3/SDL.h>

typedef struct {
    Uint32 num_samplers;
    Uint32 num_storage_textures;
    Uint32 num_storage_buffers;
    Uint32 num_uniform_buffers;
} ShaderInfo;

SDL_GPUShader *LoadShader(SDL_GPUDevice *device, const char *shaderfile);
ShaderInfo load_shader_info(const char *shaderfile);


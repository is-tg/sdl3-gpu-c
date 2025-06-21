#pragma once

#include <SDL3/SDL.h>
#include "common.h"

SDL_GPUTexture *upload_texture(
        SDL_GPUDevice *gpu,
        SDL_GPUCopyPass *copy_pass,
        const void *pixels,
        Uint32 pixels_byte_size,
        Uint32 width,
        Uint32 height);

Mesh upload_mesh_bytes(SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass,
                       const void *vertex_bytes, Uint32 vertex_byte_size,
                       const void *index_bytes, Uint32 index_byte_size,
                       size_t num_indices);

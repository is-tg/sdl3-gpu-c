#pragma once

#include <SDL3/SDL.h>
#include "common.h"

SDL_GPUTexture *load_texture_file(SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass, const char *texturefile);
Mesh load_obj_file(SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass, const char *meshfile);
Model load_model(AppState *app, SDL_GPUCopyPass *copy_pass, const char *meshfile, const char *texturefile);


#include <SDL3/SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_MALLOC SDL_malloc
#define STBI_REALLOC SDL_realloc
#define STBI_FREE SDL_free

#include "stb_image.h"


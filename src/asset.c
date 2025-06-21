#include "asset.h"
#include "game.h"
#include "gpu.h"
#include "lib/fast_obj.h"
#include "lib/stb_image.h"

SDL_GPUTexture *load_texture_file(SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass, const char *texturefile)
{
    int raw_width, raw_height;
    char tex_filepath[256];
    SDL_snprintf(tex_filepath, sizeof(tex_filepath), "assets/textures/%s", texturefile);

    stbi_set_flip_vertically_on_load(1);
    Uint8 *pixels = stbi_load(tex_filepath, &raw_width, &raw_height, NULL, 4);
    if (!pixels) {
        SDL_Log("Failed to load texture image\n%s", stbi_failure_reason());
        return NULL;
    }

    Uint32 img_width = (Uint32) raw_width;
    Uint32 img_height = (Uint32) raw_height;
    Uint32 pixels_byte_size = img_width * img_height * 4;

    SDL_GPUTexture *texture = upload_texture(gpu, copy_pass, pixels, pixels_byte_size, img_width, img_height);
    stbi_image_free(pixels);

    return texture;
}

Mesh load_obj_file(SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass, const char *meshfile)
{
    char mesh_filepath[256];
    SDL_snprintf(mesh_filepath, sizeof(mesh_filepath), "assets/meshes/%s", meshfile);

    fastObjMesh *obj_data = fast_obj_read(mesh_filepath);
    if (!obj_data) {
        SDL_Log("Failed to load OBJ file\n%s", SDL_GetError());
        return (Mesh) {0};
    }

    Vertex *vertices = SDL_malloc(obj_data->index_count * sizeof *vertices);
    uint16_t *indices  = SDL_malloc(obj_data->index_count * sizeof *indices);

    if (!vertices || !indices) {
        fast_obj_destroy(obj_data);
        SDL_free(vertices);
        SDL_free(indices);

        SDL_Log("Failed to allocate vertices/indices");
        return (Mesh) {0};
    }

    for (size_t i = 0; i < obj_data->index_count; ++i) {
        fastObjIndex idx = obj_data->indices[i];

        float *positions = &obj_data->positions[idx.p * 3];
        SDL_memcpy(vertices[i].pos, positions, sizeof(vec3));

        vertices[i].color = WHITE_COLOR;

        if (idx.t != 0) {
            float *texcoords = &obj_data->texcoords[2 * idx.t];
            SDL_memcpy(vertices[i].uv, texcoords, sizeof(vec2));
        } else {
            vertices[i].uv[0] = 0.0f;
            vertices[i].uv[1] = 0.0f;
        }

        indices[i] = (uint16_t)i;
    }

    fast_obj_destroy(obj_data);

    Mesh mesh = upload_mesh_bytes(gpu, copy_pass,
    vertices, obj_data->index_count * sizeof(Vertex),
    indices, obj_data->index_count * sizeof(uint16_t),
    obj_data->index_count);

    SDL_free(indices);
    SDL_free(vertices);
    return mesh;
}

void load_model(AppState *app, SDL_GPUCopyPass *copy_pass, const char *texturefile, const char *meshfile)
{
    Model *model = &app->model;

    model->texture = load_texture_file(app->gpu, copy_pass, texturefile);
    model->mesh    = load_obj_file(app->gpu, copy_pass, meshfile);
}



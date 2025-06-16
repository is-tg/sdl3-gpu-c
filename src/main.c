#define SDL_MAIN_USE_CALLBACKS
#define STB_IMAGE_IMPLEMENTATION
#define STBI_MALLOC SDL_malloc
#define STBI_REALLOC SDL_realloc
#define STBI_FREE SDL_free
#define FAST_OBJ_IMPLEMENTATION
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include "lib/stb_image.h"
#include "lib/fast_obj.h"
#include "lib/linalg.h"

typedef struct {
    Uint64 last_ticks;
    Uint64 new_ticks;
    float delta_time;
} TimeState;

typedef struct {
    SDL_GPUBuffer *vertex_buffer;
    SDL_GPUBuffer *index_buffer;
    Uint32 index_count;
    SDL_GPUTexture *texture;
} Model;

typedef struct {
    vec3 position;
    vec3 target;
    mat4 model;
    mat4 view;
    mat4 projection;
} Camera;

typedef struct {
    float yaw;
    float pitch;
} Look;

typedef struct {
    ALIGN(16) mat4 mvp;
} UniformBufferObject;

typedef struct {
    TimeState time;
    Uint32 window_width;
    Uint32 window_height;
    
    SDL_Window *window;
    SDL_GPUDevice *gpu;
    SDL_GPUGraphicsPipeline *pipeline;
    SDL_GPUTexture *depth_texture;
    SDL_GPUSampler *sampler;
    
    Camera camera;
    Look look;
    Model model;
    float rotation_y;

    UniformBufferObject ubo;

} AppState;

typedef struct {
    vec3 pos;
    SDL_FColor color;
    vec2 uv;
} Vertex;

#define WHITE_COLOR 0xFFFFFF
#define DARK_COLOR 0x1A1A1D
#define EYE_HEIGHT 1
#define MOVE_SPEED 5
#define LOOK_SENSITIVITY 0.3f

SDL_GPUTextureFormat depth_texture_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
bool key_down[SDL_SCANCODE_COUNT] = { 0 };
vec2 mouse_move;

SDL_FColor RGBA_F(Uint32 hex, float alpha);
SDL_GPUShader *LoadShader(SDL_GPUDevice *device, const char *shaderfile, Uint32 num_samplers, Uint32 num_uniform_buffers);

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

void try_depth_format(SDL_GPUDevice *device, SDL_GPUTextureFormat format) {
    if (SDL_GPUTextureSupportsFormat(device, format, SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET)) {
        depth_texture_format = format;
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

    try_depth_format(app->gpu, SDL_GPU_TEXTUREFORMAT_D32_FLOAT);
    try_depth_format(app->gpu, SDL_GPU_TEXTUREFORMAT_D24_UNORM);
    SDL_Log("Using texture format: %d", depth_texture_format);

    SDL_GPUTextureCreateInfo depth_tex_createinfo = {
        .format = depth_texture_format,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = app->window_width,
        .height = app->window_height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };
    app->depth_texture = SDL_CreateGPUTexture(app->gpu, &depth_tex_createinfo);

    return true;
}

bool setup_pipeline(AppState *app)
{
    SDL_GPUShader *vertex_shader = LoadShader(app->gpu, "shader.vert", 0, 1);
    SDL_GPUShader *fragment_shader = LoadShader(app->gpu, "shader.frag", 1, 0);
    if (!vertex_shader || !fragment_shader) {
        SDL_ReleaseGPUShader(app->gpu, vertex_shader);
        SDL_ReleaseGPUShader(app->gpu, fragment_shader);

        SDL_Log("Failed to load shader(s)\n%s", SDL_GetError());
        return false;
    }

    SDL_GPUVertexAttribute vertex_attrs[] = {
        {
            .location = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = offsetof(Vertex, pos),
        },
        {
            .location = 1,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
            .offset = offsetof(Vertex, color),
        },
        {
            .location = 2,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            .offset = offsetof(Vertex, uv),
        },
    };

    SDL_GPUGraphicsPipelineCreateInfo pipeline_createinfo = {
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .vertex_input_state = (SDL_GPUVertexInputState) {
            .num_vertex_buffers = 1,
            .vertex_buffer_descriptions = &(SDL_GPUVertexBufferDescription) {
                .slot = 0,
                .pitch = sizeof(Vertex),
            },
            .num_vertex_attributes = sizeof(vertex_attrs) / sizeof(vertex_attrs[0]),
            .vertex_attributes = vertex_attrs,
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .rasterizer_state = (SDL_GPURasterizerState) {
            .cull_mode = SDL_GPU_CULLMODE_FRONT,
            // .fill_mode = SDL_GPU_FILLMODE_LINE,
        },
        .depth_stencil_state = (SDL_GPUDepthStencilState) {
            .enable_depth_test = true,
            .enable_depth_write = true,
            .compare_op = SDL_GPU_COMPAREOP_LESS,
        },
        .target_info = (SDL_GPUGraphicsPipelineTargetInfo) {
            .num_color_targets = 1,
            .color_target_descriptions = &(SDL_GPUColorTargetDescription) {
                .format = SDL_GetGPUSwapchainTextureFormat(app->gpu, app->window),
            },
            .has_depth_stencil_target = true,
            .depth_stencil_format = depth_texture_format,
        }
    };
    app->pipeline = SDL_CreateGPUGraphicsPipeline(app->gpu, &pipeline_createinfo);

    SDL_ReleaseGPUShader(app->gpu, vertex_shader);
    SDL_ReleaseGPUShader(app->gpu, fragment_shader);

    SDL_GPUSamplerCreateInfo sampler_createinfo = { 0 };
    app->sampler = SDL_CreateGPUSampler(app->gpu, &sampler_createinfo);

    return true;
}

bool load_model(AppState *app, const char *meshfile, const char *texturefile)
{
    int raw_width, raw_height;
    char tex_filepath[256];
    SDL_snprintf(tex_filepath, sizeof(tex_filepath), "assets/textures/%s", texturefile);

    stbi_set_flip_vertically_on_load(1);
    Uint8 *pixels = stbi_load(tex_filepath, &raw_width, &raw_height, NULL, 4);
    if (!pixels) {
        SDL_Log("Failed to load texture image\n%s", stbi_failure_reason());
        return false;
    }

    Model *model = &app->model;
    Uint32 img_width = (Uint32) raw_width;
    Uint32 img_height = (Uint32) raw_height;
    Uint32 pixels_byte_size = img_width * img_height * 4;

    SDL_GPUTextureCreateInfo texture_createinfo = {
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage  = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width  = img_width,
        .height = img_height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };
    model->texture = SDL_CreateGPUTexture(app->gpu, &texture_createinfo);

    char mesh_filepath[256];
    SDL_snprintf(mesh_filepath, sizeof(mesh_filepath), "assets/meshes/%s", meshfile);

    fastObjMesh *mesh = fast_obj_read(mesh_filepath);
    if (!mesh) {
        stbi_image_free(pixels);

        SDL_Log("Failed to load OBJ file\n%s", SDL_GetError());
        return false;
    }

    model->index_count = mesh->index_count;
    Vertex *vertices = SDL_malloc(model->index_count * sizeof *vertices);
    uint16_t *indices  = SDL_malloc(model->index_count * sizeof *indices);

    if (!vertices || !indices) {
        fast_obj_destroy(mesh);
        SDL_free(vertices);
        SDL_free(indices);
        stbi_image_free(pixels);

        SDL_Log("Failed to allocate vertices/indices");
        return false;
    }

    SDL_FColor white = RGBA_F(WHITE_COLOR, 1);
    for (size_t i = 0; i < model->index_count; ++i) {
        fastObjIndex idx = mesh->indices[i];

        float *positions = &mesh->positions[idx.p * 3];
        SDL_memcpy(vertices[i].pos, positions, sizeof(vec3));

        vertices[i].color = white;

        if (idx.t != 0) {
            float *texcoords = &mesh->texcoords[2 * idx.t];
            SDL_memcpy(vertices[i].uv, texcoords, sizeof(vec2));
        } else {
            vertices[i].uv[0] = 0.0f;
            vertices[i].uv[1] = 0.0f;
        }

        indices[i] = (uint16_t)i;
    }

    fast_obj_destroy(mesh);

    Uint32 vertices_byte_size = (Uint32) model->index_count * sizeof *vertices;
    Uint32 indices_byte_size  = (Uint32) model->index_count * sizeof *indices;

    SDL_GPUBufferCreateInfo vertbuf_createinfo = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = vertices_byte_size,
    };
    model->vertex_buffer = SDL_CreateGPUBuffer(app->gpu, &vertbuf_createinfo);

    SDL_GPUBufferCreateInfo indbuf_createinfo = {
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = indices_byte_size,
    };
    model->index_buffer = SDL_CreateGPUBuffer(app->gpu, &indbuf_createinfo);

    SDL_GPUTransferBufferCreateInfo transbuf_createinfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = vertices_byte_size + indices_byte_size,
    };
    SDL_GPUTransferBuffer *transfer_buf = SDL_CreateGPUTransferBuffer(app->gpu, &transbuf_createinfo);

    void *transfer_mem = SDL_MapGPUTransferBuffer(app->gpu, transfer_buf, false);
    SDL_memcpy(transfer_mem, vertices, vertices_byte_size);
    SDL_memcpy((char *)transfer_mem + vertices_byte_size, indices, indices_byte_size);
    SDL_UnmapGPUTransferBuffer(app->gpu, transfer_buf);

    SDL_free(indices);
    SDL_free(vertices);

    SDL_GPUTransferBufferCreateInfo tex_transbuf_createinfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = pixels_byte_size,
    };
    SDL_GPUTransferBuffer *tex_transfer_buf = SDL_CreateGPUTransferBuffer(app->gpu, &tex_transbuf_createinfo);

    void *tex_transfer_mem = SDL_MapGPUTransferBuffer(app->gpu, tex_transfer_buf, false);
    SDL_memcpy(tex_transfer_mem, pixels, pixels_byte_size);
    SDL_UnmapGPUTransferBuffer(app->gpu, tex_transfer_buf);
    stbi_image_free(pixels);

    SDL_GPUCommandBuffer *copy_cmd_buf = SDL_AcquireGPUCommandBuffer(app->gpu);
    
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(copy_cmd_buf);

    SDL_GPUTransferBufferLocation vert_location = {
        .transfer_buffer = transfer_buf,
        .offset = 0,
    };
    SDL_GPUBufferRegion vert_region = {
        .buffer = model->vertex_buffer,
        .size = vertices_byte_size,
    };
    
    SDL_GPUTransferBufferLocation index_location = {
        .transfer_buffer = transfer_buf,
        .offset = vertices_byte_size,
    };
    SDL_GPUBufferRegion index_region = {
        .buffer = model->index_buffer,
        .size = indices_byte_size,
    };
    
    SDL_UploadToGPUBuffer(copy_pass, &vert_location, &vert_region, false);
    SDL_UploadToGPUBuffer(copy_pass, &index_location, &index_region, false);

    SDL_GPUTextureTransferInfo tex_src = {
        .transfer_buffer = tex_transfer_buf,
    };
    SDL_GPUTextureRegion tex_dst = {
        .texture = model->texture,
        .w = img_width,
        .h = img_height,
        .d = 1,
    };
    SDL_UploadToGPUTexture(copy_pass, &tex_src, &tex_dst, false);

    SDL_EndGPUCopyPass(copy_pass);

    if (!SDL_SubmitGPUCommandBuffer(copy_cmd_buf)) {
        SDL_ReleaseGPUTransferBuffer(app->gpu, transfer_buf);
        SDL_ReleaseGPUTransferBuffer(app->gpu, tex_transfer_buf);

        SDL_Log("Failed to submit command buffer while loading model\n%s", SDL_GetError());
        return false;
    }

    SDL_ReleaseGPUTransferBuffer(app->gpu, transfer_buf);
    SDL_ReleaseGPUTransferBuffer(app->gpu, tex_transfer_buf);

    return true;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    (void) argc;
    (void) argv;

    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

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

    if (!setup_pipeline(app)) {
        return SDL_APP_FAILURE;
    }

    if (!load_model(app, "model.obj", "colormap.png")) {
        return SDL_APP_FAILURE;
    }

    app->rotation_y = 0.0f;

    vec3 cam_pos = { 0, EYE_HEIGHT, 3 };
    vec3 cam_tar = { 0, EYE_HEIGHT, 0 };
    SDL_memcpy(app->camera.position, cam_pos, sizeof(vec3));
    SDL_memcpy(app->camera.target, cam_tar, sizeof(vec3));

    perspective_lh_zo(
        rad(60.0f),
        (float)app->window_width / (float)app->window_height,
        0.01f,
        1000.0f,
        app->camera.projection
    );

    SDL_SetWindowRelativeMouseMode(app->window, true);
    app->time.last_ticks = SDL_GetTicks();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    (void) appstate;

    switch (event->type)
    {
        case SDL_EVENT_KEY_DOWN:
            key_down[event->key.scancode] = true;
            break;
        case SDL_EVENT_KEY_UP:
            if (event->key.key == SDLK_ESCAPE) {
                return SDL_APP_SUCCESS;
            }
            key_down[event->key.scancode] = false;
            break;
        case SDL_EVENT_MOUSE_MOTION:
            mouse_move[0] = event->motion.xrel;
            mouse_move[1] = event->motion.yrel;
            break;
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

void update_camera(AppState *app, float dt)
{
    Camera *camera = &app->camera;
    Look   *look   = &app->look;

    // Handle look input
    look->yaw   = wrap(look->yaw + mouse_move[0] * LOOK_SENSITIVITY, 360);
    look->pitch = SDL_clamp(look->pitch - mouse_move[1] * LOOK_SENSITIVITY, -89, 89);
    vec2_zero(mouse_move);

    // Handle movement input
    vec2 move_input = { 0, 0 };
    if      (key_down[SDL_SCANCODE_W]) move_input[1] = 1;
    else if (key_down[SDL_SCANCODE_S]) move_input[1] = -1;
    if      (key_down[SDL_SCANCODE_A]) move_input[0] = 1;
    else if (key_down[SDL_SCANCODE_D]) move_input[0] = -1;

    // Calculate look matrix from yaw and pitch
    mat4 look_mat;
    vec3 angles = { rad(look->pitch), rad(look->yaw), 0 };
    euler_xyz(angles, look_mat);

    // Extract forward and right vectors from look matrix
    vec3 forward, right;
    mat4_mulv3(look_mat, FORWARD, 0, forward);
    mat4_mulv3(look_mat, RIGHT, 0, right);

    // Calculate movement direction
    vec3 move_dir = { 0, 0, 0 };
    vec3 forward_scaled, right_scaled;
    vec3_scale(forward, move_input[1], forward_scaled);
    vec3_scale(right, move_input[0], right_scaled);
    vec3_add(forward_scaled, right_scaled, move_dir);

    move_dir[1] = 0;
    vec3_normalize(move_dir);
    
    // Apply movement
    vec3 motion;
    vec3_scale(move_dir, MOVE_SPEED * dt, motion);
    vec3_add(camera->position, motion, camera->position);

    // Set camera target based on look direction
    vec3_add(camera->position, forward, camera->target);
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState *app = (AppState *) appstate;
    
    TimeState *time = &app->time;
    time->new_ticks = SDL_GetTicks();
    time->delta_time = (float) (time->new_ticks - time->last_ticks) / 1000.0f;
    time->last_ticks = time->new_ticks;

    // update game state
    app->rotation_y += rad(90.0f) * app->time.delta_time;
    update_camera(app, time->delta_time);

    // render
    SDL_GPUCommandBuffer *cmd_buf = SDL_AcquireGPUCommandBuffer(app->gpu);
    if (!cmd_buf) {
        SDL_Log("SDL_AcquireGPUCommandBuffer failed\n%s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GPUTexture *swapchain_tex = NULL;
    SDL_WaitAndAcquireGPUSwapchainTexture(cmd_buf, app->window, &swapchain_tex, NULL, NULL);

    vec3 translation = { 0, 0, 0 };
    vec3 rotation    = { 0, 1, 0 };
    mat4 translate_mat, rotate_mat;

    lookat_lh(app->camera.position, app->camera.target, YUP, app->camera.view);
    translate_make(translate_mat, translation);
    rotate_make(rotate_mat, app->rotation_y, rotation);
    mat4_mul(
        translate_mat,
        rotate_mat,
        app->camera.model
    );

    mat4 *matrices[] = {
        &app->camera.projection,
        &app->camera.view,
        &app->camera.model,
    };
    mat4_mulN(matrices, 3, app->ubo.mvp);

    if (swapchain_tex) {
        SDL_GPUColorTargetInfo color_target = {
            .texture = swapchain_tex,
            .clear_color = RGBA_F(DARK_COLOR, 1.0f),
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
        };
        SDL_GPUDepthStencilTargetInfo depth_target_info = {
            .texture = app->depth_texture,
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .clear_depth = 1,
            .store_op = SDL_GPU_STOREOP_DONT_CARE,
        };
        
        SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(cmd_buf, &color_target, 1, &depth_target_info);
        
        SDL_BindGPUGraphicsPipeline(render_pass, app->pipeline);

        Model *model = &app->model;
        
        SDL_GPUBufferBinding vert_bindings = {
            .buffer = model->vertex_buffer,
        };
        SDL_BindGPUVertexBuffers(render_pass, 0, &vert_bindings, 1);
        
        SDL_GPUBufferBinding index_bindings = {
            .buffer = model->index_buffer,
        };
        SDL_BindGPUIndexBuffer(render_pass, &index_bindings, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        
        SDL_PushGPUVertexUniformData(cmd_buf, 0, &app->ubo, sizeof(app->ubo));

        SDL_GPUTextureSamplerBinding tex_bindings = {
            .sampler = app->sampler,
            .texture = model->texture,
        };
        SDL_BindGPUFragmentSamplers(render_pass, 0, &tex_bindings, 1);

        SDL_DrawGPUIndexedPrimitives(render_pass, model->index_count, 1, 0, 0, 0);

        SDL_EndGPURenderPass(render_pass);
    }

    SDL_SubmitGPUCommandBuffer(cmd_buf); 
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    (void) result;

    if (appstate) {
        AppState *app = (AppState *) appstate;

        SDL_ReleaseGPUBuffer(app->gpu, app->model.vertex_buffer);
        SDL_ReleaseGPUBuffer(app->gpu, app->model.index_buffer);
        SDL_ReleaseGPUTexture(app->gpu, app->model.texture);

        SDL_ReleaseGPUGraphicsPipeline(app->gpu, app->pipeline);
        SDL_ReleaseGPUSampler(app->gpu, app->sampler);
        SDL_ReleaseGPUTexture(app->gpu, app->depth_texture);

        SDL_ReleaseWindowFromGPUDevice(app->gpu, app->window);
        SDL_DestroyGPUDevice(app->gpu);
        SDL_DestroyWindow(app->window);

        SDL_free(app);
    }
}

SDL_FColor RGBA_F(Uint32 hex, float alpha)
{
    return (SDL_FColor) {
        ((hex >> 16) & 0xFF) / 255.0f,
        ((hex >> 8) & 0xFF) / 255.0f,
        (hex & 0xFF) / 255.0f,
        alpha,
    };
}

SDL_GPUShader *LoadShader(SDL_GPUDevice *device, const char *shaderfile, Uint32 num_samplers, Uint32 num_uniform_buffers)
{
    SDL_GPUShaderStage stage;

    if (SDL_strstr(shaderfile, ".vert")) {
        stage = SDL_GPU_SHADERSTAGE_VERTEX;
    } else if (SDL_strstr(shaderfile, ".frag")) {
        stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    } else {
        SDL_Log("Invalid filename to determine shader stage");
        return false;
    }

    SDL_GPUShaderFormat format;
    char *format_ext = NULL;
    const char *entrypoint = "main";

    SDL_GPUShaderFormat supported_formats = SDL_GetGPUShaderFormats(device);
    if (supported_formats & SDL_GPU_SHADERFORMAT_SPIRV) {
        format = SDL_GPU_SHADERFORMAT_SPIRV;
        format_ext = "spv";
    } else if (supported_formats & SDL_GPU_SHADERFORMAT_DXIL) {
        format = SDL_GPU_SHADERFORMAT_DXIL;
        format_ext = "dxil";
    } else if (supported_formats & SDL_GPU_SHADERFORMAT_MSL) {
        format = SDL_GPU_SHADERFORMAT_MSL;
        format_ext = "msl";
        entrypoint = "main0";
    } else {
        SDL_LogCritical(SDL_LOG_CATEGORY_GPU, "No supported shader format: %u", supported_formats);
    }

    char filepath[256];
    SDL_snprintf(filepath, sizeof(filepath), "assets/shaders/out/%s.%s", shaderfile, format_ext);
    
    size_t codesize;
    void *code = SDL_LoadFile(filepath, &codesize);
    if (!code) {
        SDL_Log("Failed to load shader file: %s", shaderfile);
        return NULL;
    }

    SDL_GPUShaderCreateInfo shader_createinfo = {
        .code_size = codesize,
        .code = (Uint8 *) code,
        .entrypoint = entrypoint,
        .format = format,
        .stage = stage,
        .num_samplers = num_samplers,
        .num_uniform_buffers = num_uniform_buffers,
    };
    SDL_GPUShader *shader = SDL_CreateGPUShader(device, &shader_createinfo);
    
    SDL_free(code);

    return shader;
}


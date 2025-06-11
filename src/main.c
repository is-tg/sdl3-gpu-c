#define SDL_MAIN_USE_CALLBACKS
#define STB_IMAGE_IMPLEMENTATION
#define STBI_MALLOC SDL_malloc
#define STBI_REALLOC SDL_realloc
#define STBI_FREE SDL_free
#define FAST_OBJ_IMPLEMENTATION
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include "stb_image.h"
#include "fast_obj.h"
#include "linalg.h"

#define DARK 0x1A1A1D

typedef struct {
    SDL_Window *window;
    SDL_GPUDevice *gpu;
    SDL_GPUGraphicsPipeline *pipeline;
    SDL_GPUTexture *texture;
    SDL_GPUTexture *depth_texture;
    SDL_GPUSampler *sampler;
} AppState;

struct {
    Uint64 last_ticks;
    Uint64 new_ticks;
    float delta_time;
} time;

struct {
    matrix4 mvp;
} ubo;

static const int ASPECT_RATIO_FACTOR = 75;
SDL_GPUBuffer *vertex_buf = NULL;
SDL_GPUBuffer *index_buf = NULL;
Uint32 num_indices = 0;

SDL_FColor RGBA_F(Uint32 hex, float alpha);
SDL_AppResult SDL_Abort(const char *report);
SDL_GPUShader *LoadShader(SDL_GPUDevice *device, const char *file, SDL_GPUShaderStage stage, Uint32 num_samplers, Uint32 num_uniform_buffers);

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    (void) argc;
    (void) argv;
    
    SDL_SetLogPriorities(SDL_LOG_PRIORITY_VERBOSE);
    SDL_SetHint(SDL_HINT_RENDER_VSYNC, "1");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return SDL_Abort("Couldn't initialize SDL");
    }

    AppState *as = (AppState *) SDL_calloc(1, sizeof(AppState));
    if (!as) {
        return SDL_Abort("Failed appstate allocation");
    }
    *appstate = as;

    as->window = SDL_CreateWindow("title", 
                                  16 * ASPECT_RATIO_FACTOR,
                                  9 * ASPECT_RATIO_FACTOR,
                                  SDL_WINDOW_RESIZABLE);
    if (!as->window) {
        return SDL_Abort("Couldn't create window");
    }

    as->gpu = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    if (!as->gpu) {
        return SDL_Abort("Couldn't create gpu device");
    }

    if (!SDL_ClaimWindowForGPUDevice(as->gpu, as->window)) {
        return SDL_Abort("Couldn't claim window for gpu device");
    }

    SDL_GPUShader *vertex_shader = LoadShader(as->gpu, "shader.spv.vert", SDL_GPU_SHADERSTAGE_VERTEX, 0, 1);
    SDL_GPUShader *fragment_shader = LoadShader(as->gpu, "shader.spv.frag", SDL_GPU_SHADERSTAGE_FRAGMENT, 1, 0);

    struct {
        int w, h;
    } img_size;

    stbi_set_flip_vertically_on_load(1);
    Uint8 *pixels = stbi_load("colormap.png", &img_size.w, &img_size.h, NULL, 4);
    if (!pixels) {
        SDL_Log("STBI failure: %s", stbi_failure_reason());
        return SDL_Abort("Failed to load texture image");
    }
    Uint32 pixels_byte_size = (Uint32)img_size.w * (Uint32)img_size.h * 4;

    SDL_GPUTextureCreateInfo texture_createinfo = {
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        .usage  = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width  = (Uint32) img_size.w,
        .height = (Uint32) img_size.h,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };
    as->texture = SDL_CreateGPUTexture(as->gpu, &texture_createinfo);

    int w = 0, h = 0;
    SDL_GetWindowSize(as->window, &w, &h);

    const SDL_GPUTextureFormat DEPTH_TEXTURE_FORMAT = SDL_GPU_TEXTUREFORMAT_D24_UNORM;

    SDL_GPUTextureCreateInfo depth_tex_createinfo = {
        .format = DEPTH_TEXTURE_FORMAT,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = (Uint32) w,
        .height = (Uint32) h,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };
    as->depth_texture = SDL_CreateGPUTexture(as->gpu, &depth_tex_createinfo);

    typedef struct {
        float pos[3];
        SDL_FColor color;
        float uv[2];
    } Vertex_Data;

    SDL_FColor WHITE = { 1, 1, 1, 1 };

    fastObjMesh *mesh = fast_obj_read("model.obj");
    if (!mesh) {
        return SDL_Abort("Failed to load OBJ file");
    }

    num_indices = mesh->index_count;
    Vertex_Data *vertices = SDL_malloc(num_indices * sizeof *vertices);
    uint16_t *indices  = SDL_malloc(num_indices * sizeof *indices);

    if (!vertices || !indices) {
        fast_obj_destroy(mesh);
        return SDL_Abort("Out of memory");
    }

    for (size_t i = 0; i < num_indices; ++i) {
        fastObjIndex idx = mesh->indices[i];

        float *pbase = &mesh->positions[idx.p * 3];
        SDL_memcpy(vertices[i].pos, pbase, 3 * sizeof(float));

        vertices[i].color = WHITE;

        if (idx.t != 0) {
            vertices[i].uv[0] = mesh->texcoords[2 * idx.t + 0];
            vertices[i].uv[1] = mesh->texcoords[2 * idx.t + 1];
        }

        indices[i] = (uint16_t)i;
    }

    fast_obj_destroy(mesh);

    Uint32 vertices_byte_size = (Uint32) num_indices * sizeof *vertices;
    Uint32 indices_byte_size  = (Uint32) num_indices * sizeof *indices;

    SDL_GPUBufferCreateInfo vertbuf_createinfo = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = vertices_byte_size,
    };
    vertex_buf = SDL_CreateGPUBuffer(as->gpu, &vertbuf_createinfo);

    SDL_GPUBufferCreateInfo indbuf_createinfo = {
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = indices_byte_size,
    };
    index_buf = SDL_CreateGPUBuffer(as->gpu, &indbuf_createinfo);

    SDL_GPUTransferBufferCreateInfo transbuf_createinfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = vertices_byte_size + indices_byte_size,
    };
    SDL_GPUTransferBuffer *transfer_buf = SDL_CreateGPUTransferBuffer(as->gpu, &transbuf_createinfo);

    void *transfer_mem = SDL_MapGPUTransferBuffer(as->gpu, transfer_buf, false);
    SDL_memcpy(transfer_mem, vertices, vertices_byte_size);
    SDL_memcpy((char *)transfer_mem + vertices_byte_size, indices, indices_byte_size);
    SDL_UnmapGPUTransferBuffer(as->gpu, transfer_buf);

    SDL_free(indices);
    SDL_free(vertices);

    SDL_GPUTransferBufferCreateInfo tex_transbuf_createinfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = pixels_byte_size,
    };
    SDL_GPUTransferBuffer *tex_transfer_buf = SDL_CreateGPUTransferBuffer(as->gpu, &tex_transbuf_createinfo);
    void *tex_transfer_mem = SDL_MapGPUTransferBuffer(as->gpu, tex_transfer_buf, false);
    SDL_memcpy(tex_transfer_mem, pixels, pixels_byte_size);
    SDL_UnmapGPUTransferBuffer(as->gpu, tex_transfer_buf);
    stbi_image_free(pixels);

    SDL_GPUCommandBuffer *copy_cmd_buf = SDL_AcquireGPUCommandBuffer(as->gpu);
    
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(copy_cmd_buf);

    SDL_GPUTransferBufferLocation vert_location = {
        .transfer_buffer = transfer_buf,
        .offset = 0,
    };
    SDL_GPUBufferRegion vert_region = {
        .buffer = vertex_buf,
        .size = vertices_byte_size,
    };
    
    SDL_GPUTransferBufferLocation index_location = {
        .transfer_buffer = transfer_buf,
        .offset = vertices_byte_size,
    };
    SDL_GPUBufferRegion index_region = {
        .buffer = index_buf,
        .size = indices_byte_size,
    };
    
    SDL_UploadToGPUBuffer(copy_pass, &vert_location, &vert_region, false);
    SDL_UploadToGPUBuffer(copy_pass, &index_location, &index_region, false);

    SDL_GPUTextureTransferInfo tex_src = {
        .transfer_buffer = tex_transfer_buf,
    };
    SDL_GPUTextureRegion tex_dst = {
        .texture = as->texture,
        .w = (Uint32) img_size.w,
        .h = (Uint32) img_size.h,
        .d = 1,
    };
    SDL_UploadToGPUTexture(copy_pass, &tex_src, &tex_dst, false);

    SDL_EndGPUCopyPass(copy_pass);

    SDL_SubmitGPUCommandBuffer(copy_cmd_buf);
    SDL_ReleaseGPUTransferBuffer(as->gpu, transfer_buf);
    SDL_ReleaseGPUTransferBuffer(as->gpu, tex_transfer_buf);

    SDL_GPUSamplerCreateInfo sampler_createinfo = { 0 };
    as->sampler = SDL_CreateGPUSampler(as->gpu, &sampler_createinfo);

    SDL_GPUVertexAttribute vertex_attrs[] = {
        {
            .location = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = offsetof(Vertex_Data, pos),
        },
        {
            .location = 1,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
            .offset = offsetof(Vertex_Data, color),
        },
        {
            .location = 2,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
            .offset = offsetof(Vertex_Data, uv),
        },
    };

    SDL_GPUGraphicsPipelineCreateInfo pipeline_createinfo = {
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .vertex_input_state = (SDL_GPUVertexInputState) {
            .num_vertex_buffers = 1,
            .vertex_buffer_descriptions = &(SDL_GPUVertexBufferDescription) {
                .slot = 0,
                .pitch = sizeof(Vertex_Data),
            },
            .num_vertex_attributes = sizeof(vertex_attrs) / sizeof(vertex_attrs[0]),
            .vertex_attributes = vertex_attrs,
        },
        .depth_stencil_state = (SDL_GPUDepthStencilState) {
            .enable_depth_test = true,
            .enable_depth_write = true,
            .compare_op = SDL_GPU_COMPAREOP_LESS,
        },
        .target_info = (SDL_GPUGraphicsPipelineTargetInfo) {
            .num_color_targets = 1,
            .color_target_descriptions = &(SDL_GPUColorTargetDescription) {
                .format = SDL_GetGPUSwapchainTextureFormat(as->gpu, as->window),
            },
            .has_depth_stencil_target = true,
            .depth_stencil_format = DEPTH_TEXTURE_FORMAT,
        }
    };
    as->pipeline = SDL_CreateGPUGraphicsPipeline(as->gpu, &pipeline_createinfo);

    SDL_ReleaseGPUShader(as->gpu, vertex_shader);
    SDL_ReleaseGPUShader(as->gpu, fragment_shader);

    matrix4 projection = matrix4_perspective(
        SDL_PI_F * 60 / 180, (float) w / (float) h, 0.0001f, 1000.0f
    );
    
    matrix4 translation = matrix4_translate(0, -1, -3);
    
    ubo.mvp = matrix4_multiply(&projection, &translation);

    time.last_ticks = SDL_GetTicks();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    (void) appstate;

    switch (event->type)
    {
        case SDL_EVENT_KEY_UP:
            if (event->key.key == SDLK_ESCAPE) {
                return SDL_APP_SUCCESS;
            } else {
                break;
            }
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState *as = (AppState *) appstate;
    
    time.new_ticks = SDL_GetTicks();
    time.delta_time = (float) (time.new_ticks - time.last_ticks) / 1000.0f;
    time.last_ticks = time.new_ticks;

    SDL_GPUCommandBuffer *cmd_buf = SDL_AcquireGPUCommandBuffer(as->gpu);
    if (!cmd_buf) {
        return SDL_Abort("SDL_AcquireGPUCommandBuffer failed");
    }

    SDL_GPUTexture *swapchain_tex;
    SDL_WaitAndAcquireGPUSwapchainTexture(cmd_buf, as->window, &swapchain_tex, NULL, NULL);

    matrix4 rotation = matrix4_rotate(SDL_PI_F / 2 * time.delta_time, 0, 1, 0);
    ubo.mvp = matrix4_multiply(&ubo.mvp, &rotation);

    if (swapchain_tex) {
        SDL_GPUColorTargetInfo color_target = {
            .texture = swapchain_tex,
            .clear_color = RGBA_F(DARK, 1.0f),
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_STORE,
        };
        SDL_GPUDepthStencilTargetInfo depth_target_info = {
            .texture = as->depth_texture,
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .clear_depth = 1,
            .store_op = SDL_GPU_STOREOP_DONT_CARE,
        };
        
        SDL_GPURenderPass *render_pass = SDL_BeginGPURenderPass(cmd_buf, &color_target, 1, &depth_target_info);
        
        SDL_BindGPUGraphicsPipeline(render_pass, as->pipeline);
        
        SDL_GPUBufferBinding vert_bindings = {
            .buffer = vertex_buf,
        };
        SDL_BindGPUVertexBuffers(render_pass, 0, &vert_bindings, 1);
        
        SDL_GPUBufferBinding index_bindings = {
            .buffer = index_buf,
        };
        SDL_BindGPUIndexBuffer(render_pass, &index_bindings, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        
        SDL_PushGPUVertexUniformData(cmd_buf, 0, &ubo, sizeof(ubo));

        SDL_GPUTextureSamplerBinding tex_bindings = {
            .sampler = as->sampler,
            .texture = as->texture,
        };
        SDL_BindGPUFragmentSamplers(render_pass, 0, &tex_bindings, 1);

        SDL_DrawGPUIndexedPrimitives(render_pass, num_indices, 1, 0, 0, 0);

        SDL_EndGPURenderPass(render_pass);
    }

    SDL_SubmitGPUCommandBuffer(cmd_buf); 
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    (void) result;

    if (appstate) {
        AppState *as = (AppState *) appstate;

        SDL_ReleaseGPUBuffer(as->gpu, vertex_buf);
        SDL_ReleaseGPUBuffer(as->gpu, index_buf);

        SDL_ReleaseGPUGraphicsPipeline(as->gpu, as->pipeline);
        SDL_ReleaseGPUSampler(as->gpu, as->sampler);
        SDL_ReleaseGPUTexture(as->gpu, as->depth_texture);
        SDL_ReleaseGPUTexture(as->gpu, as->texture);

        SDL_ReleaseWindowFromGPUDevice(as->gpu, as->window);
        SDL_DestroyGPUDevice(as->gpu);
        SDL_DestroyWindow(as->window);

        SDL_free(as);
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

SDL_GPUShader *LoadShader(SDL_GPUDevice *device, const char *file, SDL_GPUShaderStage stage, Uint32 num_samplers, Uint32 num_uniform_buffers)
{
    size_t codesize;
    void *code = SDL_LoadFile(file, &codesize);
    if (!code) {
        SDL_Log("Failed to load shader file: %s", file);
    }
    
    SDL_GPUShaderCreateInfo shader_createinfo = {
        .code_size = codesize,
        .code = (Uint8 *) code,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = stage,
        .num_samplers = num_samplers,
        .num_uniform_buffers = num_uniform_buffers,
    };
    SDL_GPUShader *shader = SDL_CreateGPUShader(device, &shader_createinfo);
    
    SDL_free(code);

    return shader;
}

SDL_AppResult SDL_Abort(const char *report)
{
    SDL_Log("%s\n%s", report, SDL_GetError());
    return SDL_APP_FAILURE;
}

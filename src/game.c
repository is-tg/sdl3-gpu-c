#include "game.h"
#include "shader.h"
#include "asset.h"

void game_init(AppState *app)
{
    setup_pipeline(app);

    SDL_GPUCommandBuffer *copy_cmd_buf = SDL_AcquireGPUCommandBuffer(app->gpu);
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(copy_cmd_buf);

    load_model(app, copy_pass, "colormap.png", "model.obj");

    SDL_EndGPUCopyPass(copy_pass);
    if (!SDL_SubmitGPUCommandBuffer(copy_cmd_buf))
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "failed model loading");

    app->rotate = true;
    app->rotation_y = 0.0f;

    app->clear_color = DARK_COLOR;

    vec3 cam_pos = { 0, EYE_HEIGHT, 3 };
    vec3 cam_tar = { 0, EYE_HEIGHT, 0 };
    SDL_memcpy(app->camera.position, cam_pos, sizeof(vec3));
    SDL_memcpy(app->camera.target, cam_tar, sizeof(vec3));
}

void game_update(AppState *app)
{
    if (app->rotate) app->rotation_y += ROTATION_SPEED * app->time.delta_time;
    update_camera(app, app->time.delta_time);
}

void game_render(AppState *app, SDL_GPUCommandBuffer *cmd_buf, SDL_GPUTexture *swapchain_tex)
{
    perspective_lh_zo(
        60.0f * RAD_PER_DEG,
        (float)app->window_width / (float)app->window_height,
        0.01f,
        1000.0f,
        app->camera.projection
    );

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
    UniformBufferObject ubo = {0};
    mat4_mulN(matrices, 3, ubo.mvp);

    SDL_GPUColorTargetInfo color_target = {
        .texture = swapchain_tex,
        .clear_color = app->clear_color,
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
    SDL_PushGPUVertexUniformData(cmd_buf, 0, &ubo, sizeof(ubo));
    SDL_BindGPUGraphicsPipeline(render_pass, app->pipeline);
    const Model *model = &app->model;
    SDL_GPUBufferBinding vert_bindings = {
        .buffer = model->mesh.vertex_buffer,
    };
    SDL_BindGPUVertexBuffers(render_pass, 0, &vert_bindings, 1);
    SDL_GPUBufferBinding index_bindings = {
        .buffer = model->mesh.index_buffer,
    };
    SDL_BindGPUIndexBuffer(render_pass, &index_bindings, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_GPUTextureSamplerBinding tex_bindings = {
        .sampler = app->sampler,
        .texture = model->texture,
    };
    SDL_BindGPUFragmentSamplers(render_pass, 0, &tex_bindings, 1);
    SDL_DrawGPUIndexedPrimitives(render_pass, model->mesh.index_count, 1, 0, 0, 0);

    SDL_EndGPURenderPass(render_pass);
}

void setup_pipeline(AppState *app)
{
    SDL_GPUShader *vertex_shader = LoadShader(app->gpu, "shader.vert");
    SDL_GPUShader *fragment_shader = LoadShader(app->gpu, "shader.frag");
    if (!vertex_shader || !fragment_shader) {
        SDL_ReleaseGPUShader(app->gpu, vertex_shader);
        SDL_ReleaseGPUShader(app->gpu, fragment_shader);

        SDL_Log("Failed to load shader(s)\n%s", SDL_GetError());
        return;
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
                .format = app->swapchain_texture_format,
            },
            .has_depth_stencil_target = true,
            .depth_stencil_format = app->depth_texture_format,
        }
    };
    app->pipeline = SDL_CreateGPUGraphicsPipeline(app->gpu, &pipeline_createinfo);

    SDL_ReleaseGPUShader(app->gpu, vertex_shader);
    SDL_ReleaseGPUShader(app->gpu, fragment_shader);

    SDL_GPUSamplerCreateInfo sampler_createinfo = { 0 };
    app->sampler = SDL_CreateGPUSampler(app->gpu, &sampler_createinfo);
}

void update_camera(AppState *app, float dt)
{
    Camera *camera = &app->camera;
    Look   *look   = &app->look;

    // Handle look input
    look->yaw   = wrap(look->yaw + app->mouse_move[0] * LOOK_SENSITIVITY, 360);
    look->pitch = SDL_clamp(look->pitch - app->mouse_move[1] * LOOK_SENSITIVITY, -89, 89);
    vec2_zero(app->mouse_move);

    // Handle movement input
    vec2 move_input = { 0, 0 };
    if      (app->key_down[SDL_SCANCODE_W]) move_input[1] = 1;
    else if (app->key_down[SDL_SCANCODE_S]) move_input[1] = -1;
    if      (app->key_down[SDL_SCANCODE_A]) move_input[0] = 1;
    else if (app->key_down[SDL_SCANCODE_D]) move_input[0] = -1;

    // Calculate look matrix from yaw and pitch
    mat4 look_mat;
    vec3 angles = { look->pitch * RAD_PER_DEG, look->yaw * RAD_PER_DEG, 0 };
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


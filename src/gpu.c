#include "gpu.h"

SDL_GPUTexture *upload_texture(
        SDL_GPUDevice *gpu,
        SDL_GPUCopyPass *copy_pass,
        const void *pixels,
        Uint32 pixels_byte_size,
        Uint32 width,
        Uint32 height)
{
    SDL_GPUTextureCreateInfo texture_createinfo = {
        .format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB,
        .usage  = SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width  = width,
        .height = height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
    };
    SDL_GPUTexture *texture = SDL_CreateGPUTexture(gpu, &texture_createinfo);

    SDL_GPUTransferBufferCreateInfo tex_transbuf_createinfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = pixels_byte_size,
    };
    SDL_GPUTransferBuffer *tex_transfer_buf = SDL_CreateGPUTransferBuffer(gpu, &tex_transbuf_createinfo);

    void *tex_transfer_mem = SDL_MapGPUTransferBuffer(gpu, tex_transfer_buf, false);
    SDL_memcpy(tex_transfer_mem, pixels, pixels_byte_size);
    SDL_UnmapGPUTransferBuffer(gpu, tex_transfer_buf);

    SDL_GPUTextureTransferInfo tex_src = {
        .transfer_buffer = tex_transfer_buf,
    };
    SDL_GPUTextureRegion tex_dst = {
        .texture = texture,
        .w = width,
        .h = height,
        .d = 1,
    };
    SDL_UploadToGPUTexture(copy_pass, &tex_src, &tex_dst, false);

    SDL_ReleaseGPUTransferBuffer(gpu, tex_transfer_buf);

    return texture;
}

Mesh upload_mesh_bytes(SDL_GPUDevice *gpu, SDL_GPUCopyPass *copy_pass,
                       const void *vertex_bytes, Uint32 vertex_byte_size,
                       const void *index_bytes, Uint32 index_byte_size,
                       size_t num_indices)
{
    SDL_GPUBufferCreateInfo vertbuf_createinfo = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size  = vertex_byte_size,
    };
    SDL_GPUBuffer *vertex_buffer = SDL_CreateGPUBuffer(gpu, &vertbuf_createinfo);

    SDL_GPUBufferCreateInfo indbuf_createinfo = {
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size  = index_byte_size,
    };
    SDL_GPUBuffer *index_buffer = SDL_CreateGPUBuffer(gpu, &indbuf_createinfo);

    SDL_GPUTransferBufferCreateInfo transbuf_createinfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size  = vertex_byte_size + index_byte_size,
    };
    SDL_GPUTransferBuffer *transfer_buf = SDL_CreateGPUTransferBuffer(gpu, &transbuf_createinfo);

    void *transfer_mem = SDL_MapGPUTransferBuffer(gpu, transfer_buf, false);
    SDL_memcpy(transfer_mem, vertex_bytes, vertex_byte_size);
    SDL_memcpy((char *)transfer_mem + vertex_byte_size, index_bytes, index_byte_size);
    SDL_UnmapGPUTransferBuffer(gpu, transfer_buf);

    SDL_GPUTransferBufferLocation vert_location = {
        .transfer_buffer = transfer_buf,
        .offset = 0,
    };
    SDL_GPUBufferRegion vert_region = {
        .buffer = vertex_buffer,
        .size   = vertex_byte_size,
    };

    SDL_GPUTransferBufferLocation index_location = {
        .transfer_buffer = transfer_buf,
        .offset = vertex_byte_size,
    };
    SDL_GPUBufferRegion index_region = {
        .buffer = index_buffer,
        .size   = index_byte_size,
    };

    SDL_UploadToGPUBuffer(copy_pass, &vert_location, &vert_region, false);
    SDL_UploadToGPUBuffer(copy_pass, &index_location, &index_region, false);

    SDL_ReleaseGPUTransferBuffer(gpu, transfer_buf);

    return (Mesh) {
        .vertex_buffer = vertex_buffer,
        .index_buffer  = index_buffer,
        .index_count   = (Uint32) num_indices,
    };
}

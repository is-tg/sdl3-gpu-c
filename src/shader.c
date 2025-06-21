#include "shader.h"
#include "lib/cJSON.h"

SDL_GPUShader *LoadShader(SDL_GPUDevice *device, const char *shaderfile)
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

    char pathbuffer[256], filename[256];
    SDL_snprintf(pathbuffer, sizeof(pathbuffer), "assets/shaders/out/%s", shaderfile);
    SDL_snprintf(filename, sizeof(filename), "%s.%s", pathbuffer, format_ext);
    
    size_t codesize;
    void *code = SDL_LoadFile(filename, &codesize);
    if (!code) {
        SDL_Log("Failed to load shader file: %s", shaderfile);
        return NULL;
    }

    ShaderInfo info = load_shader_info(pathbuffer);

    SDL_GPUShaderCreateInfo shader_createinfo = {
        .code_size = codesize,
        .code = (Uint8 *) code,
        .entrypoint = entrypoint,
        .format = format,
        .stage = stage,
        .num_samplers = info.num_samplers,
        .num_uniform_buffers = info.num_uniform_buffers,
        .num_storage_buffers = info.num_storage_buffers,
        .num_storage_textures = info.num_storage_textures,
    };
    SDL_GPUShader *shader = SDL_CreateGPUShader(device, &shader_createinfo);
    
    SDL_free(code);

    return shader;
}

ShaderInfo load_shader_info(const char *shaderfile)
{
    ShaderInfo info = {0};

    char filename[256];
    SDL_snprintf(filename, sizeof(filename), "%s.json", shaderfile);

    size_t file_size = 0;
    char *file_data = (char *)SDL_LoadFile(filename, &file_size);
    if (!file_data) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "JSON Load Error", SDL_GetError(), NULL);
        return info;
    }

    cJSON *json = cJSON_Parse(file_data);
    if (!json) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Failed to parse JSON");
        SDL_free(file_data);
        return info;
    }

    #define JSON_GET_UINT(json, name) \
        ((Uint32)(cJSON_GetObjectItemCaseSensitive((json), (name)) ? \
        cJSON_GetObjectItemCaseSensitive((json), (name))->valueint : 0))

    info.num_samplers         = JSON_GET_UINT(json, "samplers");
    info.num_storage_textures = JSON_GET_UINT(json, "storage_textures");
    info.num_storage_buffers  = JSON_GET_UINT(json, "storage_buffers");
    info.num_uniform_buffers  = JSON_GET_UINT(json, "uniform_buffers");

    cJSON_Delete(json);
    SDL_free(file_data);

    return info;
}


#pragma once

#if defined(_MSC_VER)
#  define ALIGN(X) __declspec(align(X))
#else
#  define ALIGN(X) __attribute__((aligned(X)))
#endif

#if defined(__GNUC__) || defined(__clang__)
#  define UNLIKELY(expr) __builtin_expect(!!(expr), 0)
#  define LIKELY(expr)   __builtin_expect(!!(expr), 1)
#else
#  define UNLIKELY(expr) (expr)
#  define LIKELY(expr)   (expr)
#endif

#define FLT_EPSILON 1e-5f

#include <SDL3/SDL.h>

typedef float vec2[2];
typedef float vec3[3];
typedef ALIGN(16) float vec4[4];
typedef ALIGN(16) vec4  mat4[4];

#define VEC2_ONE_INIT   {1.0f, 1.0f}
#define VEC2_ZERO_INIT  {0.0f, 0.0f}
#define VEC2_ONE  ((vec2)VEC2_ONE_INIT)
#define VEC2_ZERO ((vec2)VEC2_ZERO_INIT)

#define VEC3_ONE_INIT   {1.0f, 1.0f, 1.0f}
#define VEC3_ZERO_INIT  {0.0f, 0.0f, 0.0f}
#define VEC3_ONE  ((vec3)VEC3_ONE_INIT)
#define VEC3_ZERO ((vec3)VEC3_ZERO_INIT)

#define YUP       ((vec3){0.0f,  1.0f,  0.0f})
#define FORWARD   ((vec3){0.0f,  0.0f, -1.0f})
#define RIGHT     ((vec3){1.0f,  0.0f, 0.0f})

#define MAT4_IDENTITY_INIT  {{1.0f, 0.0f, 0.0f, 0.0f},  \
                             {0.0f, 1.0f, 0.0f, 0.0f},  \
                             {0.0f, 0.0f, 1.0f, 0.0f},  \
                             {0.0f, 0.0f, 0.0f, 1.0f}}

#define MAT4_ZERO_INIT      {{0.0f, 0.0f, 0.0f, 0.0f},  \
                             {0.0f, 0.0f, 0.0f, 0.0f},  \
                             {0.0f, 0.0f, 0.0f, 0.0f},  \
                             {0.0f, 0.0f, 0.0f, 0.0f}}

#define MAT4_IDENTITY ((mat4)MAT4_IDENTITY_INIT)
#define MAT4_ZERO     ((mat4)MAT4_ZERO_INIT)

#define VEC2_FMT "{%f, %f}"
#define VEC3_FMT "{%f, %f, %f}"

float wrap(float x, float y);
float rad(float deg);
float deg(float rad);

bool vec2_equals(vec2 a, vec2 b);
void vec2_zero(vec2 v);
void vec2_add(vec2 a, vec2 b, vec2 dest);
void vec2_scale(vec2 v, float s, vec2 dest);

void  vec3_zero(vec3 v);
void  vec3_copy(vec3 a, vec3 dest);
void  vec3_add(vec3 a, vec3 b, vec3 dest);
void  vec3_sub(vec3 a, vec3 b, vec3 dest);
void  vec3_scale(vec3 v, float s, vec3 dest);
float vec3_dot(vec3 a, vec3 b);
void  vec3_cross(vec3 a, vec3 b, vec3 dest);
float vec3_norm2(vec3 v);
float vec3_norm(vec3 v);
void  vec3_normalize(vec3 v);
void  vec3_normalize_to(vec3 v, vec3 dest);
void  vec3_crossn(vec3 a, vec3 b, vec3 dest);

void mat4_mulv3(mat4 m, vec3 v, float last, vec3 dest);
void mat4_copy(mat4 mat, mat4 dest);
void mat4_zero(mat4 mat);
void mat4_identity(mat4 mat);
void mat4_mul(mat4 m1, mat4 m2, mat4 dest);
void mat4_mulN(mat4 * __restrict matrices[], uint32_t len, mat4 dest);

void translate_make(mat4 m, vec3 v);
void rotate_make(mat4 m, float angle, vec3 axis);

void perspective_lh_zo(float fovy, float aspect, float nearZ, float farZ, mat4 dest);
void lookat_lh(vec3 eye, vec3 center, vec3 up, mat4 dest);
void euler_xyz(vec3 angles, mat4 dest);


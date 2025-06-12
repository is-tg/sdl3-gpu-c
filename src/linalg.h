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

#define VEC3_ONE_INIT   {1.0f, 1.0f, 1.0f}
#define VEC3_ZERO_INIT  {0.0f, 0.0f, 0.0f}

#define VEC3_ONE  ((vec3)VEC3_ONE_INIT)
#define VEC3_ZERO ((vec3)VEC3_ZERO_INIT)

#define YUP       ((vec3){0.0f,  1.0f,  0.0f})
#define FORWARD   ((vec3){0.0f,  0.0f, 1.0f})

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


float rad(float deg);
float deg(float rad);

void mat4_identity(mat4 mat);
void mat4_mul(mat4 m1, mat4 m2, mat4 dest);
void mat4_mulN(mat4 * __restrict matrices[], uint32_t len, mat4 dest);

void translate_make(mat4 m, vec3 v);
void rotate_make(mat4 m, float angle, vec3 axis);

void perspective_lh_zo(float fovy, float aspect, float nearZ, float farZ, mat4 dest);
void lookat_lh(vec3 eye, vec3 center, vec3 up, mat4 dest);

// Function prototypes
// matrix4 matrix4_transpose(matrix4 in);
// matrix4 matrix4_perspective(float fov_y, float aspect, float near, float far, bool flip_z_axis);
// matrix4 matrix4_look_at(Vec3 eye, Vec3 centre, Vec3 up, bool flip_z_axis);
//
// matrix4 matrix4_translate(float x, float y, float z);
// matrix4 matrix4_rotate(float angle, float x, float y, float z);
// matrix4 matrix4_rotate_x(float angle);
// matrix4 matrix4_rotate_y(float angle);
// matrix4 matrix4_rotate_z(float angle);
//
// matrix4 matrix4_multiply_val(matrix4 a, matrix4 b);
// matrix4 matrix4_multiply(const matrix4 *a, const matrix4 *b);
// matrix4 matrix4_multiply3(const matrix4 *a, const matrix4 *b, const matrix4 *c);
//
// matrix4 matrix4_identity(void);


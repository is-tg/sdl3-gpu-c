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

#include <SDL3/SDL.h>

typedef float           vec2[2];
typedef float           vec3[3];
typedef ALIGN(16) float vec4[4];
typedef vec4            quat;
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

#define QUAT_IDENTITY_INIT  {0.0f, 0.0f, 0.0f, 1.0f}
#define QUAT_IDENTITY       ((quat)QUAT_IDENTITY_INIT)

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

#define TAU 6.28318530717958647692528676655900576f
#define RAD_PER_DEG (TAU / 360.0f)
#define DEG_PER_RAD (360.0f / TAU)

#define VEC2_FMT "{%f, %f}"
#define VEC3_FMT "{%f, %f, %f}"

float wrap(float x, float y);

bool vec2_equals(const vec2 a, const vec2 b);
void vec2_zero(vec2 v);
void vec2_add(const vec2 a, const vec2 b, vec2 dest);
void vec2_scale(const vec2 v, float s, vec2 dest);

void  vec3_zero(vec3 v);
void  vec3_copy(const vec3 a, vec3 dest);
void  vec3_add(const vec3 a, const vec3 b, vec3 dest);
void  vec3_sub(const vec3 a, const vec3 b, vec3 dest);
void  vec3_scale(const vec3 v, float s, vec3 dest);
float vec3_dot(const vec3 a, const vec3 b);
void  vec3_cross(const vec3 a, const vec3 b, vec3 dest);
float vec3_norm2(const vec3 v);
float vec3_norm(const vec3 v);
void  vec3_normalize(vec3 v);
void  vec3_normalize_to(const vec3 v, vec3 dest);
void  vec3_crossn(const vec3 a, const vec3 b, vec3 dest);

void vec3_to_quat(const vec3 v, float w, quat dest);
void quat_angle_axis(float angle_radians, const vec3 axis, quat dest);
void quat_mul(const quat p, const quat q, quat dest);
void vec4_copy(const vec4 v, vec4 dest);
void quat_copy(const quat q, quat dest);

void mat4_scale(const vec3 v, mat4 dest);
void mat4_mulv3(const mat4 m, const vec3 v, float last, vec3 dest);
void mat4_copy(const mat4 mat, mat4 dest);
void mat4_zero(mat4 mat);
void mat4_identity(mat4 mat);
void mat4_mul(const mat4 m1, const mat4 m2, mat4 dest);
void mat4_mulN(mat4 * __restrict matrices[], uint32_t len, mat4 dest);

void mat4_translate(const vec3 v, mat4 m);
void mat4_from_quat(const quat q, mat4 m);
void mat4_from_trs(const vec3 t, const quat r, const vec3 s, mat4 dest);

void perspective_lh_zo(float fovy, float aspect, float nearZ, float farZ, mat4 dest);
void lookat_lh(const vec3 eye, const vec3 center, const vec3 up, mat4 dest);
void euler_xyz(const vec3 angles, mat4 dest);

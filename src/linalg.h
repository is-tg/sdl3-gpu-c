#ifndef LINALG_H
#define LINALG_H

#include <SDL3/SDL.h>
#define VEC3_UP ((Vec3){ 0, 1, 0 })

typedef struct { float x, y, z; } Vec3;
typedef struct { float x, y; } Vec2;

// 4x4 matrix type (column-major order)
typedef struct {
    float m[4 * 4];
} matrix4;

// Function prototypes
matrix4 matrix4_transpose(matrix4 in);
matrix4 matrix4_perspective(float fov_y, float aspect, float near, float far, bool flip_z_axis);
matrix4 matrix4_look_at(Vec3 eye, Vec3 centre, Vec3 up, bool flip_z_axis);

matrix4 matrix4_translate(float x, float y, float z);
matrix4 matrix4_rotate(float angle, float x, float y, float z);
matrix4 matrix4_rotate_x(float angle);
matrix4 matrix4_rotate_y(float angle);
matrix4 matrix4_rotate_z(float angle);

matrix4 matrix4_multiply_val(matrix4 a, matrix4 b);
matrix4 matrix4_multiply(const matrix4 *a, const matrix4 *b);
matrix4 matrix4_multiply3(const matrix4 *a, const matrix4 *b, const matrix4 *c);

matrix4 matrix4_identity(void);

#endif // LINALG_H

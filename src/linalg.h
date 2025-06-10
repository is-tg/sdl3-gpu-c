#ifndef LINALG_H
#define LINALG_H

#include <SDL3/SDL.h>

// 4x4 matrix type (column-major order)
typedef struct {
    float m[16];
} matrix4;

// Function prototypes
matrix4 matrix4_perspective(float fov_y, float aspect, float near_z, float far_z);
matrix4 matrix4_translate(float x, float y, float z);
matrix4 matrix4_rotate(float angle, float x, float y, float z);
matrix4 matrix4_rotate_x(float angle);
matrix4 matrix4_rotate_y(float angle);
matrix4 matrix4_rotate_z(float angle);
matrix4 matrix4_multiply(const matrix4 *a, const matrix4 *b);
matrix4 matrix4_identity(void);

#endif // LINALG_H

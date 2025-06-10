#include <SDL3/SDL.h>

// 4x4 matrix type (column-major order)
typedef struct {
    float m[16];
} matrix4;

// Create perspective projection matrix
// fov_y: field of view in radians (vertical)
// aspect: aspect ratio (width/height)
// near_z: near clipping plane distance
// far_z: far clipping plane distance
matrix4 matrix4_perspective(float fov_y, float aspect, float near_z, float far_z)
{
    matrix4 result = {0};

    float tan_half_fov = SDL_tanf(fov_y * 0.5f);
    float range = near_z - far_z;

    // Column 0
    result.m[0] = 1.0f / (aspect * tan_half_fov);
    result.m[1] = 0.0f;
    result.m[2] = 0.0f;
    result.m[3] = 0.0f;

    // Column 1
    result.m[4] = 0.0f;
    result.m[5] = 1.0f / tan_half_fov;
    result.m[6] = 0.0f;
    result.m[7] = 0.0f;

    // Column 2
    result.m[8] = 0.0f;
    result.m[9] = 0.0f;
    result.m[10] = (far_z + near_z) / range;
    result.m[11] = -1.0f;

    // Column 3
    result.m[12] = 0.0f;
    result.m[13] = 0.0f;
    result.m[14] = (2.0f * far_z * near_z) / range;
    result.m[15] = 0.0f;

    return result;
}

// Create translation matrix
matrix4 matrix4_translate(float x, float y, float z)
{
    matrix4 result = {0};

    // Identity matrix with translation in last column
    result.m[0] = 1.0f; // [0][0]
    result.m[1] = 0.0f; // [1][0]
    result.m[2] = 0.0f; // [2][0]
    result.m[3] = 0.0f; // [3][0]

    result.m[4] = 0.0f; // [0][1]
    result.m[5] = 1.0f; // [1][1]
    result.m[6] = 0.0f; // [2][1]
    result.m[7] = 0.0f; // [3][1]

    result.m[8] = 0.0f; // [0][2]
    result.m[9] = 0.0f; // [1][2]
    result.m[10] = 1.0f; // [2][2]
    result.m[11] = 0.0f; // [3][2]

    result.m[12] = x; // [0][3]
    result.m[13] = y; // [1][3]
    result.m[14] = z; // [2][3]
    result.m[15] = 1.0f; // [3][3]

    return result;
}

// Create rotation matrix around arbitrary axis
// angle: rotation angle in radians
// x, y, z: axis of rotation (should be normalized)
matrix4 matrix4_rotate(float angle, float x, float y, float z)
{
    matrix4 result = {0};

    float c = SDL_cosf(angle);
    float s = SDL_sinf(angle);
    float one_minus_c = 1.0f - c;

    // Normalize the axis vector
    float length = sqrtf(x * x + y * y + z * z);
    if (length > 0.0f) {
        x /= length;
        y /= length;
        z /= length;
    }

    // Rodrigues' rotation formula in matrix form
    // Column 0
    result.m[0] = c + x * x * one_minus_c;
    result.m[1] = y * x * one_minus_c + z * s;
    result.m[2] = z * x * one_minus_c - y * s;
    result.m[3] = 0.0f;

    // Column 1
    result.m[4] = x * y * one_minus_c - z * s;
    result.m[5] = c + y * y * one_minus_c;
    result.m[6] = z * y * one_minus_c + x * s;
    result.m[7] = 0.0f;

    // Column 2
    result.m[8] = x * z * one_minus_c + y * s;
    result.m[9] = y * z * one_minus_c - x * s;
    result.m[10] = c + z * z * one_minus_c;
    result.m[11] = 0.0f;

    // Column 3
    result.m[12] = 0.0f;
    result.m[13] = 0.0f;
    result.m[14] = 0.0f;
    result.m[15] = 1.0f;

    return result;
}

// Helper functions for common rotations
matrix4 matrix4_rotate_x(float angle)
{
    return matrix4_rotate(angle, 1.0f, 0.0f, 0.0f);
}

matrix4 matrix4_rotate_y(float angle)
{
    return matrix4_rotate(angle, 0.0f, 1.0f, 0.0f);
}

matrix4 matrix4_rotate_z(float angle)
{
    return matrix4_rotate(angle, 0.0f, 0.0f, 1.0f);
}

// Matrix multiplication (A * B)
matrix4 matrix4_multiply(const matrix4 *a, const matrix4 *b)
{
    matrix4 result = {0};

    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += a->m[k * 4 + row] * b->m[col * 4 + k];
            }
            result.m[col * 4 + row] = sum;
        }
    }

    return result;
}

// Identity matrix
matrix4 matrix4_identity(void)
{
    matrix4 result = {0};
    result.m[0] = 1.0f; // [0][0]
    result.m[5] = 1.0f; // [1][1]
    result.m[10] = 1.0f; // [2][2]
    result.m[15] = 1.0f; // [3][3]
    return result;
}

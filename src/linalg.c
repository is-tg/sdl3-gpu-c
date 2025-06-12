#include "linalg.h"

matrix4 matrix4_transpose(matrix4 in)
{
    matrix4 out;

    out.m[0]  = in.m[0];   out.m[1]  = in.m[4];   out.m[2]  = in.m[8];   out.m[3]  = in.m[12];
    out.m[4]  = in.m[1];   out.m[5]  = in.m[5];   out.m[6]  = in.m[9];   out.m[7]  = in.m[13];
    out.m[8]  = in.m[2];   out.m[9]  = in.m[6];   out.m[10] = in.m[10];  out.m[11] = in.m[14];
    out.m[12] = in.m[3];   out.m[13] = in.m[7];   out.m[14] = in.m[11];  out.m[15] = in.m[15];

    return out;
}

matrix4 matrix4_perspective(float fov_y, float aspect, float near, float far, bool forward_is_negative_z)
{
    matrix4 result = {0};

    float tan_half_fov = SDL_tanf(fov_y * 0.5f);
    float range = far - near;

    // Standard perspective projection
    result.m[0]  = 1.0f / (aspect * tan_half_fov);  // [0][0]
    result.m[5]  = 1.0f / tan_half_fov;             // [1][1]  
    
    if (forward_is_negative_z) {
        // Standard OpenGL-style (forward = -Z)
        result.m[10] = -(far + near) / range;       // [2][2]
        result.m[14] = -(2.0f * far * near) / range; // [3][2] - translation
        result.m[11] = -1.0f;                       // [2][3] - w component handling
    } else {
        // Direct3D-style (forward = +Z)
        result.m[10] = (far + near) / range;        // [2][2]
        result.m[14] = (2.0f * far * near) / range; // [3][2] - translation
        result.m[11] = 1.0f;                        // [2][3] - w component handling
    }

    return result;
}

// Create translation matrix
matrix4 matrix4_translate(float x, float y, float z)
{
    matrix4 result = matrix4_identity();

    result.m[12] = x; // [3][0]
    result.m[13] = y; // [3][1]
    result.m[14] = z; // [3][2]

    return result;
}

// Create rotation matrix around arbitrary axis
matrix4 matrix4_rotate(float angle, float x, float y, float z)
{
    matrix4 result = {0};

    float c = SDL_cosf(angle);
    float s = SDL_sinf(angle);
    float one_minus_c = 1.0f - c;

    // Normalize the axis vector
    float length = SDL_sqrtf(x * x + y * y + z * z);
    if (length > 0.0f) {
        x /= length;
        y /= length;
        z /= length;
    }

    // Rodrigues' rotation formula in matrix form (column-major)
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

// Matrix multiplication (A * B) - column-major order
matrix4 matrix4_multiply_val(const matrix4 a, const matrix4 b)
{
    matrix4 result = {0};

    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            float sum = 0.0f;
            for (int k = 0; k < 4; k++) {
                sum += a.m[k * 4 + row] * b.m[col * 4 + k];
            }
            result.m[col * 4 + row] = sum;
        }
    }

    return result;
}

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

matrix4 matrix4_multiply3(const matrix4 *a, const matrix4 *b, const matrix4 *c)
{
    matrix4 ab = matrix4_multiply(a, b);
    return matrix4_multiply(&ab, c);
}

// Identity matrix
matrix4 matrix4_identity(void)
{
    static const matrix4 identity = {
        .m = { 1, 0, 0, 0,
               0, 1, 0, 0,
               0, 0, 1, 0,
               0, 0, 0, 1 }
    };
    return identity;
}

Vec3 vector3_difference(Vec3 minuend, Vec3 subtrahend)
{
    return (Vec3) {
        minuend.x - subtrahend.x,
        minuend.y - subtrahend.y,
        minuend.z - subtrahend.z,
    };
}

float vector3_dot(Vec3 a, Vec3 b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 vector3_cross(Vec3 a, Vec3 b)
{
    return (Vec3) {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x,
    };
}

Vec3 vector3_normalize(Vec3 a)
{
    float magnitude = SDL_sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
    if (magnitude == 0.0f) return (Vec3) { 0, 0, 0 };
    
    return (Vec3) { 
        a.x / magnitude, 
        a.y / magnitude, 
        a.z / magnitude 
    };
}

matrix4 matrix4_look_at(Vec3 eye, Vec3 centre, Vec3 up, bool forward_is_negative_z)
{
    Vec3 f = vector3_normalize(vector3_difference(centre, eye));
    Vec3 s = vector3_normalize(vector3_cross(f, up));
    Vec3 u = vector3_cross(s, f);

    // Handle coordinate system
    if (forward_is_negative_z) {
        f.x = -f.x;
        f.y = -f.y; 
        f.z = -f.z;
    }

    // Column-major storage
    return (matrix4) {
        .m = {
            // Column 0 (right vector)
            s.x, s.y, s.z, 0.0f,
            // Column 1 (up vector)  
            u.x, u.y, u.z, 0.0f,
            // Column 2 (forward vector)
            f.x, f.y, f.z, 0.0f,
            // Column 3 (translation)
            -vector3_dot(s, eye), -vector3_dot(u, eye), -vector3_dot(f, eye), 1.0f,
        },
    };
}

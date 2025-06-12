#include "linalg.h"

float rad(float deg)
{
    return deg * SDL_PI_F / 180.0f;
}

float deg(float rad)
{
    return rad * 180.0f / SDL_PI_F;
}

void vec3_zero(vec3 v)
{
    v[0] = v[1] = v[2] = 0.0f;
}

void vec3_copy(vec3 a, vec3 dest)
{
    dest[0] = a[0];
    dest[1] = a[1];
    dest[2] = a[2];
}

void vec3_sub(vec3 a, vec3 b, vec3 dest)
{
    dest[0] = a[0] - b[0];
    dest[1] = a[1] - b[1];
    dest[2] = a[2] - b[2];
}

void vec3_scale(vec3 v, float s, vec3 dest)
{
    dest[0] = v[0] * s;
    dest[1] = v[1] * s;
    dest[2] = v[2] * s;
}

float vec3_dot(vec3 a, vec3 b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void vec3_cross(vec3 a, vec3 b, vec3 dest)
{
    vec3 c;
    c[0] = a[1] * b[2] - a[2] * b[1];
    c[1] = a[2] * b[0] - a[0] * b[2];
    c[2] = a[0] * b[1] - a[1] * b[0];
    vec3_copy(c, dest);
}

float vec3_norm2(vec3 v)
{
    return vec3_dot(v, v);
}

float vec3_norm(vec3 v)
{
    return SDL_sqrtf(vec3_norm2(v));
}

void vec3_normalize(vec3 v)
{
    float norm = vec3_norm(v);

    if (UNLIKELY(norm < FLT_EPSILON)) {
        v[0] = v[1] = v[2] = 0.0f;
        return;
    }

    vec3_scale(v, 1.0f / norm, v);
}

void vec3_normalize_to(vec3 v, vec3 dest)
{
    float norm = vec3_norm(v);

    if (UNLIKELY(norm < FLT_EPSILON)) {
        vec3_zero(dest);
        return;
    }

    vec3_scale(v, 1.0f / norm, dest);
}

void vec3_crossn(vec3 a, vec3 b, vec3 dest)
{
    vec3_cross(a, b, dest);
    vec3_normalize(dest);
}

void mat4_copy(mat4 mat, mat4 dest)
{
    dest[0][0] = mat[0][0];  dest[1][0] = mat[1][0];
    dest[0][1] = mat[0][1];  dest[1][1] = mat[1][1];
    dest[0][2] = mat[0][2];  dest[1][2] = mat[1][2];
    dest[0][3] = mat[0][3];  dest[1][3] = mat[1][3];

    dest[2][0] = mat[2][0];  dest[3][0] = mat[3][0];
    dest[2][1] = mat[2][1];  dest[3][1] = mat[3][1];
    dest[2][2] = mat[2][2];  dest[3][2] = mat[3][2];
    dest[2][3] = mat[2][3];  dest[3][3] = mat[3][3];
}

void mat4_zero(mat4 mat)
{
    mat4 t = MAT4_ZERO_INIT;
    mat4_copy(t, mat);
}

void mat4_identity(mat4 mat)
{
    ALIGN(16) mat4 t = MAT4_IDENTITY_INIT;
    mat4_copy(t, mat);
}

void mat4_mul(mat4 m1, mat4 m2, mat4 dest)
{
    float a00 = m1[0][0], a01 = m1[0][1], a02 = m1[0][2], a03 = m1[0][3],
          a10 = m1[1][0], a11 = m1[1][1], a12 = m1[1][2], a13 = m1[1][3],
          a20 = m1[2][0], a21 = m1[2][1], a22 = m1[2][2], a23 = m1[2][3],
          a30 = m1[3][0], a31 = m1[3][1], a32 = m1[3][2], a33 = m1[3][3],

          b00 = m2[0][0], b01 = m2[0][1], b02 = m2[0][2], b03 = m2[0][3],
          b10 = m2[1][0], b11 = m2[1][1], b12 = m2[1][2], b13 = m2[1][3],
          b20 = m2[2][0], b21 = m2[2][1], b22 = m2[2][2], b23 = m2[2][3],
          b30 = m2[3][0], b31 = m2[3][1], b32 = m2[3][2], b33 = m2[3][3];

    dest[0][0] = a00 * b00 + a10 * b01 + a20 * b02 + a30 * b03;
    dest[0][1] = a01 * b00 + a11 * b01 + a21 * b02 + a31 * b03;
    dest[0][2] = a02 * b00 + a12 * b01 + a22 * b02 + a32 * b03;
    dest[0][3] = a03 * b00 + a13 * b01 + a23 * b02 + a33 * b03;
    dest[1][0] = a00 * b10 + a10 * b11 + a20 * b12 + a30 * b13;
    dest[1][1] = a01 * b10 + a11 * b11 + a21 * b12 + a31 * b13;
    dest[1][2] = a02 * b10 + a12 * b11 + a22 * b12 + a32 * b13;
    dest[1][3] = a03 * b10 + a13 * b11 + a23 * b12 + a33 * b13;
    dest[2][0] = a00 * b20 + a10 * b21 + a20 * b22 + a30 * b23;
    dest[2][1] = a01 * b20 + a11 * b21 + a21 * b22 + a31 * b23;
    dest[2][2] = a02 * b20 + a12 * b21 + a22 * b22 + a32 * b23;
    dest[2][3] = a03 * b20 + a13 * b21 + a23 * b22 + a33 * b23;
    dest[3][0] = a00 * b30 + a10 * b31 + a20 * b32 + a30 * b33;
    dest[3][1] = a01 * b30 + a11 * b31 + a21 * b32 + a31 * b33;
    dest[3][2] = a02 * b30 + a12 * b31 + a22 * b32 + a32 * b33;
    dest[3][3] = a03 * b30 + a13 * b31 + a23 * b32 + a33 * b33;
}

void mat4_mulN(mat4 * __restrict matrices[], uint32_t len, mat4 dest)
{
    if (len < 2) {
        mat4_copy(*matrices[0], dest);
        return;
    }

    for (uint32_t i = 2; i < len; i++) {
        mat4_mul(dest, *matrices[i], dest);
    }
}

void translate_make(mat4 m, vec3 v)
{
    mat4_identity(m);
    vec3_copy(v, m[3]);
}

void rotate_make(mat4 m, float angle, vec3 axis)
{
    ALIGN(16) vec3 axisn, v, vs;
    float c = SDL_cosf(angle);

    vec3_normalize_to(axis, axisn);
    vec3_scale(axisn, 1.0f - c, v);
    vec3_scale(axisn, SDL_sinf(angle), vs);

    vec3_scale(axisn, v[0], m[0]);
    vec3_scale(axisn, v[1], m[1]);
    vec3_scale(axisn, v[2], m[2]);

    m[0][0] += c;       m[1][0] -= vs[2];   m[2][0] += vs[1];
    m[0][1] += vs[2];   m[1][1] += c;       m[2][1] -= vs[0];
    m[0][2] -= vs[1];   m[1][2] += vs[0];   m[2][2] += c;

    m[0][3] = m[1][3] = m[2][3] = m[3][0] = m[3][1] = m[3][2] = 0.0f;
    m[3][3] = 1.0f;
}

// left-handed coordinate system: positive Z goes into the screen
// zero-to-one depth range
void perspective_lh_zo(float fovy, float aspect, float nearZ, float farZ, mat4 dest)
{
    float f, fn;

    mat4_zero(dest);

    f  = 1.0f / SDL_tanf(fovy * 0.5f);
    fn = 1.0f / (nearZ - farZ);

    dest[0][0] = f / aspect;
    dest[1][1] = f;
    dest[2][2] = -farZ * fn;
    dest[2][3] = 1.0f;
    dest[3][2] = nearZ * farZ * fn;
}

void lookat_lh(vec3 eye, vec3 center, vec3 up, mat4 dest)
{
    ALIGN(16) vec3 f, u, s;

    vec3_sub(center, eye, f);
    vec3_normalize(f);

    vec3_crossn(up, f, s);
    vec3_cross(f, s, u);

    dest[0][0] = s[0];
    dest[0][1] = u[0];
    dest[0][2] = f[0];
    dest[1][0] = s[1];
    dest[1][1] = u[1];
    dest[1][2] = f[1];
    dest[2][0] = s[2];
    dest[2][1] = u[2];
    dest[2][2] = f[2];
    dest[3][0] = -vec3_dot(s, eye);
    dest[3][1] = -vec3_dot(u, eye);
    dest[3][2] = -vec3_dot(f, eye);
    dest[0][3] = dest[1][3] = dest[2][3] = 0.0f;
    dest[3][3] = 1.0f;
}

#include "linalg.h"

float wrap(float x, float y)
{
    float tmp = SDL_fmodf(x, y);
    return tmp < 0 ? y + tmp : tmp;
}

bool vec2_equals(const vec2 a, const vec2 b)
{
    return a[0] == b[0] && a[1] == b[1];
}

void vec2_zero(vec2 v)
{
    v[0] = v[1] = 0.0f;
}

void vec2_add(const vec2 a, const vec2 b, vec2 dest)
{
    dest[0] = a[0] + b[0];
    dest[1] = a[1] + b[1];
}

void vec2_scale(const vec2 v, float s, vec2 dest)
{
    dest[0] = v[0] * s;
    dest[1] = v[1] * s;
}

void vec3_zero(vec3 v)
{
    v[0] = v[1] = v[2] = 0.0f;
}

void vec3_copy(const vec3 a, vec3 dest)
{
    dest[0] = a[0];
    dest[1] = a[1];
    dest[2] = a[2];
}

void vec3_add(const vec3 a, const vec3 b, vec3 dest)
{
    dest[0] = a[0] + b[0];
    dest[1] = a[1] + b[1];
    dest[2] = a[2] + b[2];
}

void vec3_sub(const vec3 a, const vec3 b, vec3 dest)
{
    dest[0] = a[0] - b[0];
    dest[1] = a[1] - b[1];
    dest[2] = a[2] - b[2];
}

void vec3_scale(const vec3 v, float s, vec3 dest)
{
    dest[0] = v[0] * s;
    dest[1] = v[1] * s;
    dest[2] = v[2] * s;
}

float vec3_dot(const vec3 a, const vec3 b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void vec3_cross(const vec3 a, const vec3 b, vec3 dest)
{
    vec3 c;
    c[0] = a[1] * b[2] - a[2] * b[1];
    c[1] = a[2] * b[0] - a[0] * b[2];
    c[2] = a[0] * b[1] - a[1] * b[0];
    vec3_copy(c, dest);
}

float vec3_norm2(const vec3 v)
{
    return vec3_dot(v, v);
}

float vec3_norm(const vec3 v)
{
    return SDL_sqrtf(vec3_norm2(v));
}

void vec3_normalize(vec3 v)
{
    float norm = vec3_norm(v);

    if (UNLIKELY(norm < SDL_FLT_EPSILON)) {
        v[0] = v[1] = v[2] = 0.0f;
        return;
    }

    vec3_scale(v, 1.0f / norm, v);
}

void vec3_normalize_to(const vec3 v, vec3 dest)
{
    float norm = vec3_norm(v);

    if (UNLIKELY(norm < SDL_FLT_EPSILON)) {
        vec3_zero(dest);
        return;
    }

    vec3_scale(v, 1.0f / norm, dest);
}

void vec3_crossn(const vec3 a, const vec3 b, vec3 dest)
{
    vec3_cross(a, b, dest);
    vec3_normalize(dest);
}

void vec3_to_quat(const vec3 v, float w, quat dest)
{
    dest[0] = v[0];
    dest[1] = v[1];
    dest[2] = v[2];
    dest[3] = w;
}

void quat_angle_axis(float angle_radians, const vec3 axis, quat dest)
{
    float t = angle_radians * 0.5f;
    vec3 v;
    vec3_copy(axis, v);
    vec3_normalize(v);
    vec3_scale(v, SDL_sinf(t), v);

    vec3_to_quat(v, SDL_cosf(t), dest);
}

void quat_mul(const quat p, const quat q, quat dest)
{
    /*
    + (a1 b2 + b1 a2 + c1 d2 − d1 c2)i
    + (a1 c2 − b1 d2 + c1 a2 + d1 b2)j
    + (a1 d2 + b1 c2 − c1 b2 + d1 a2)k
        a1 a2 − b1 b2 − c1 c2 − d1 d2
    */
    dest[0] = p[3] * q[0] + p[0] * q[3] + p[1] * q[2] - p[2] * q[1];
    dest[1] = p[3] * q[1] - p[0] * q[2] + p[1] * q[3] + p[2] * q[0];
    dest[2] = p[3] * q[2] + p[0] * q[1] - p[1] * q[0] + p[2] * q[3];
    dest[3] = p[3] * q[3] - p[0] * q[0] - p[1] * q[1] - p[2] * q[2];
}

void demote_vec4(const vec4 v4, vec3 dest)
{
    dest[0] = v4[0];
    dest[1] = v4[1];
    dest[2] = v4[2];
}

void promote_vec3(const vec3 v3, float last, vec4 dest)
{
    dest[0] = v3[0];
    dest[1] = v3[1];
    dest[2] = v3[2];
    dest[3] = last;
}

void vec4_copy(const vec4 v, vec4 dest)
{
    dest[0] = v[0];
    dest[1] = v[1];
    dest[2] = v[2];
    dest[3] = v[3];
}

void quat_copy(const quat q, quat dest)
{
    vec4_copy(q, dest);
}

void mat4_copy(const mat4 mat, mat4 dest)
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
    mat4 t = MAT4_IDENTITY_INIT;
    mat4_copy(t, mat);
}

void mat4_scale(const vec3 v, mat4 dest)
{
    mat4_zero(dest);
    dest[0][0] = v[0];
    dest[1][1] = v[1];
    dest[2][2] = v[2];
    dest[3][3] = 1.0f;
}

void mat4_mulv(const mat4 m, const vec4 v, vec4 dest)
{
    vec4 res;  // avoid issues if arrays overlap
    res[0] = m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2] + m[3][0] * v[3];
    res[1] = m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2] + m[3][1] * v[3];
    res[2] = m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2] + m[3][2] * v[3];
    res[3] = m[0][3] * v[0] + m[1][3] * v[1] + m[2][3] * v[2] + m[3][3] * v[3];
    vec4_copy(res, dest);
}

void mat4_mulv3(const mat4 m, const vec3 v, float last, vec3 dest)
{
    vec4 res;
    promote_vec3(v, last, res);
    mat4_mulv(m, res, res);
    demote_vec4(res, dest);
}

void mat4_mul(const mat4 m1, const mat4 m2, mat4 dest)
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
    if (len == 0) return;
    if (len == 1) {
        mat4_copy(*matrices[0], dest);
        return;
    }

    mat4_mul(*matrices[0], *matrices[1], dest);

    for (uint32_t i = 2; i < len; i++) {
        mat4_mul(dest, *matrices[i], dest);
    }
}

void mat4_translate(const vec3 v, mat4 m)
{
    mat4_identity(m);
    vec3_copy(v, m[3]);
}

void mat4_from_quat(const quat q, mat4 m)
{
    float
        qxx = q[0] * q[0],
        qyy = q[1] * q[1],
        qzz = q[2] * q[2],
        qxz = q[0] * q[2],
        qxy = q[0] * q[1],
        qyz = q[1] * q[2],
        qwx = q[3] * q[0],
        qwy = q[3] * q[1],
        qwz = q[3] * q[2];

    // Column 0
    m[0][0] = 1.0f - 2.0f * (qyy + qzz);
    m[0][1] = 2.0f * (qxy + qwz);
    m[0][2] = 2.0f * (qxz - qwy);
    m[0][3] = 0.0f;

    // Column 1
    m[1][0] = 2.0f * (qxy - qwz);
    m[1][1] = 1.0f - 2.0f * (qxx + qzz);
    m[1][2] = 2.0f * (qyz + qwx);
    m[1][3] = 0.0f;

    // Column 2
    m[2][0] = 2.0f * (qxz + qwy);
    m[2][1] = 2.0f * (qyz - qwx);
    m[2][2] = 1.0f - 2.0f * (qxx + qyy);
    m[2][3] = 0.0f;

    // Column 3 (translation/homogeneous)
    m[3][0] = 0.0f;
    m[3][1] = 0.0f;
    m[3][2] = 0.0f;
    m[3][3] = 1.0f;
}

void mat4_from_trs(const vec3 t, const quat r, const vec3 s, mat4 dest)
{
    mat4 translation, rotation, scale;
    mat4_translate(t, translation);
    mat4_from_quat(r, rotation);
    mat4_scale(s, scale);
    mat4 *matrices[3] = {
        &scale,
        &rotation,
        &translation,
    };
    mat4_mulN(matrices, 3, dest);
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

void lookat_lh(const vec3 eye, const vec3 center, const vec3 up, mat4 dest)
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

void euler_xyz(const vec3 angles, mat4 dest)
{
    float cx, cy, cz, sx, sy, sz, czsx, cxcz, sysz;

    sx   = SDL_sinf(angles[0]); cx = SDL_cosf(angles[0]);
    sy   = SDL_sinf(angles[1]); cy = SDL_cosf(angles[1]);
    sz   = SDL_sinf(angles[2]); cz = SDL_cosf(angles[2]);

    czsx = cz * sx;
    cxcz = cx * cz;
    sysz = sy * sz;

    dest[0][0] =  cy * cz;
    dest[0][1] =  czsx * sy + cx * sz;
    dest[0][2] = -cxcz * sy + sx * sz;
    dest[1][0] = -cy * sz;
    dest[1][1] =  cxcz - sx * sysz;
    dest[1][2] =  czsx + cx * sysz;
    dest[2][0] =  sy;
    dest[2][1] = -cy * sx;
    dest[2][2] =  cx * cy;
    dest[0][3] =  0.0f;
    dest[1][3] =  0.0f;
    dest[2][3] =  0.0f;
    dest[3][0] =  0.0f;
    dest[3][1] =  0.0f;
    dest[3][2] =  0.0f;
    dest[3][3] =  1.0f;
}

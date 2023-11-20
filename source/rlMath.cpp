#include "rlMath.hpp"
#include <algorithm>
#include <cmath>

using namespace rlgl;

Matrix::Matrix(const float *mat)
{
    std::copy(mat, mat + 16, m);
}

Matrix::Matrix(float m0, float m4, float m8,  float m12,
           float m1, float m5, float m9,  float m13,
           float m2, float m6, float m10, float m14,
           float m3, float m7, float m11, float m15)
{
    m[0] = m0;  m[4] = m4;  m[8] = m8;   m[12] = m12;
    m[1] = m1;  m[5] = m5;  m[9] = m9;   m[13] = m13;
    m[2] = m2;  m[6] = m6;  m[10] = m10; m[14] = m14;
    m[3] = m3;  m[7] = m7;  m[11] = m11; m[15] = m15;
}

Matrix Matrix::Identity()
{
    return {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

Matrix Matrix::Translate(float x, float y, float z)
{
    return {
        1.0f, 0.0f, 0.0f, x,
        0.0f, 1.0f, 0.0f, y,
        0.0f, 0.0f, 1.0f, z,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

Matrix Matrix::Rotate(float angle, float x, float y, float z)
{
    float c = std::cos(angle);
    float s = std::sin(angle);
    float t = 1.0f - c;

    float tx = t * x;
    float ty = t * y;
    float tz = t * z;

    float sx = s * x;
    float sy = s * y;
    float sz = s * z;

    return {
        tx * x + c, tx * y - sz, tx * z + sy, 0.0f,
        ty * x + sz, ty * y + c, ty * z - sx, 0.0f,
        tz * x - sy, tz * y + sx, tz * z + c, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

Matrix Matrix::RotateX(float angle)
{
    float c = std::cos(angle);
    float s = std::sin(angle);

    return {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, c,    -s,   0.0f,
        0.0f, s,     c,   0.0f,
        0.0f, 0.0f,  0.0f, 1.0f
    };
}

Matrix Matrix::RotateY(float angle)
{
    float c = std::cos(angle);
    float s = std::sin(angle);

    return {
        c,    0.0f, s,    0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        -s,   0.0f, c,   0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

Matrix Matrix::RotateZ(float angle)
{
    float c = std::cos(angle);
    float s = std::sin(angle);

    return {
        c,    -s,   0.0f, 0.0f,
        s,     c,   0.0f, 0.0f,
        0.0f,  0.0f, 1.0f, 0.0f,
        0.0f,  0.0f, 0.0f, 1.0f
    };
}

Matrix Matrix::RotateXYZ(float angleX, float angleY, float angleZ)
{
    float cx = std::cos(angleX);
    float sx = std::sin(angleX);
    float cy = std::cos(angleY);
    float sy = std::sin(angleY);
    float cz = std::cos(angleZ);
    float sz = std::sin(angleZ);

    return {
        cy * cz, -cy * sz, sy, 0.0f,
        sx * sy * cz + cx * sz, -sx * sy * sz + cx * cz, -sx * cy, 0.0f,
        -cx * sy * cz + sx * sz, cx * sy * sz + sx * cz, cx * cy, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

Matrix Matrix::RotateZYX(float angleZ, float angleY, float angleX)
{
    float cx = std::cos(angleX);
    float sx = std::sin(angleX);
    float cy = std::cos(angleY);
    float sy = std::sin(angleY);
    float cz = std::cos(angleZ);
    float sz = std::sin(angleZ);

    return {
        cy * cz, -sz, cz * sy, 0.0f,
        cx * sz + sx * sy * cz, cx * cz - sx * sy * sz, -sx * cy, 0.0f,
        sx * sz - cx * sy * cz, cx * sy * sz + sx * cz, cx * cy, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

Matrix Matrix::Scale(float x, float y, float z)
{
    return {
        x, 0.0f, 0.0f, 0.0f,
        0.0f, y, 0.0f, 0.0f,
        0.0f, 0.0f, z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

Matrix Matrix::Frustum(float left, float right, float bottom, float top, float near, float far)
{
    float rl = right - left;
    float tb = top - bottom;
    float fn = far - near;

    return {
        2.0f * near / rl, 0.0f, (right + left) / rl, 0.0f,
        0.0f, 2.0f * near / tb, (top + bottom) / tb, 0.0f,
        0.0f, 0.0f, -(far + near) / fn, -2.0f * far * near / fn,
        0.0f, 0.0f, -1.0f, 0.0f
    };
}

Matrix Matrix::Perspective(float fovy, float aspect, float near, float far)
{
    float tanHalfFovy = std::tan(fovy / 2.0f);

    return {
        1.0f / (aspect * tanHalfFovy), 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f / tanHalfFovy, 0.0f, 0.0f,
        0.0f, 0.0f, -(far + near) / (far - near), -2.0f * far * near / (far - near),
        0.0f, 0.0f, -1.0f, 0.0f
    };
}

Matrix Matrix::Ortho(float left, float right, float bottom, float top, float near, float far)
{
    float rl = right - left;
    float tb = top - bottom;
    float fn = far - near;

    return {
        2.0f / rl, 0.0f, 0.0f, -(right + left) / rl,
        0.0f, 2.0f / tb, 0.0f, -(top + bottom) / tb,
        0.0f, 0.0f, -2.0f / fn, -(far + near) / fn,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

// Inverse la matrice (si elle est inversible)
Matrix Matrix::Invert() const
{
    float det = Determinant();
    if (det == 0.0f) return Matrix::Identity();

    float invDet = 1.0f / det;
    Matrix result;
    result.m[0] = (m[5] * m[10] - m[6] * m[9]) * invDet;
    result.m[1] = (m[2] * m[9] - m[1] * m[10]) * invDet;
    result.m[2] = (m[1] * m[6] - m[2] * m[5]) * invDet;
    result.m[4] = (m[6] * m[8] - m[4] * m[10]) * invDet;
    result.m[5] = (m[0] * m[10] - m[2] * m[8]) * invDet;
    result.m[6] = (m[2] * m[4] - m[0] * m[6]) * invDet;
    result.m[8] = (m[4] * m[9] - m[5] * m[8]) * invDet;
    result.m[9] = (m[1] * m[8] - m[0] * m[9]) * invDet;
    result.m[10] = (m[0] * m[5] - m[1] * m[4]) * invDet;
    result.m[3] = result.m[7] = result.m[11] = result.m[12] = result.m[13] = result.m[14] = 0.0f;
    result.m[15] = 1.0f;
    return result;
}

// Opérateur d'addition pour la matrice 4x4
Matrix Matrix::operator+(const Matrix& other) const
{
    Matrix result;

    for (int i = 0; i < 16; ++i)
    {
        result.m[i] = m[i] + other.m[i];
    }

    return result;
}

// Opérateur de soustraction pour la matrice 4x4
Matrix Matrix::operator-(const Matrix& other) const
{
    Matrix result;

    for (int i = 0; i < 16; ++i)
    {
        result.m[i] = m[i] - other.m[i];
    }

    return result;
}

// Opérateur de multiplication de matrice 4x4
Matrix Matrix::operator*(const Matrix& other) const
{
    Matrix result;

    for (int i = 0; i < 4; ++i)
    {
        for (int j = 0; j < 4; ++j)
        {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k)
            {
                sum += m[i * 4 + k] * other.m[k * 4 + j];
            }
            result.m[i * 4 + j] = sum;
        }
    }

    return result;
}

// Opérateur de multiplication par un scalaire pour la matrice 4x4
Matrix Matrix::operator*(float scalar) const
{
    Matrix result;

    for (int i = 0; i < 16; ++i)
    {
        result.m[i] = m[i] * scalar;
    }

    return result;
}

// Opérateur d'égalité pour la matrice 4x4
bool Matrix::operator==(const Matrix& other) const
{
    for (int i = 0; i < 16; ++i)
    {
        if (m[i] != other.m[i])
        {
            return false;
        }
    }

    return true;
}

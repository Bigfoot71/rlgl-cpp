#include "rlMath.hpp"
#include <algorithm>

using namespace rlgl;


Matrix::Matrix(const float *mat)
{
    std::copy(mat, mat + 16, m);
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

Matrix::Matrix(
    float m0, float m4, float m8,  float m12,
    float m1, float m5, float m9,  float m13,
    float m2, float m6, float m10, float m14,
    float m3, float m7, float m11, float m15
)
{
    m[0] = m0;  m[4] = m4;  m[8] = m8;   m[12] = m12;
    m[1] = m1;  m[5] = m5;  m[9] = m9;   m[13] = m13;
    m[2] = m2;  m[6] = m6;  m[10] = m10; m[14] = m14;
    m[3] = m3;  m[7] = m7;  m[11] = m11; m[15] = m15;
}

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

#include "rlMath.hpp"

using namespace rlgl;

Matrix Matrix::Identity()
{
    return {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
}

Matrix Matrix::operator*(const Matrix& other) const
{
    Matrix result;

    result.m0 = m0*other.m0 + m1*other.m4 + m2*other.m8 + m3*other.m12;
    result.m1 = m0*other.m1 + m1*other.m5 + m2*other.m9 + m3*other.m13;
    result.m2 = m0*other.m2 + m1*other.m6 + m2*other.m10 + m3*other.m14;
    result.m3 = m0*other.m3 + m1*other.m7 + m2*other.m11 + m3*other.m15;
    result.m4 = m4*other.m0 + m5*other.m4 + m6*other.m8 + m7*other.m12;
    result.m5 = m4*other.m1 + m5*other.m5 + m6*other.m9 + m7*other.m13;
    result.m6 = m4*other.m2 + m5*other.m6 + m6*other.m10 + m7*other.m14;
    result.m7 = m4*other.m3 + m5*other.m7 + m6*other.m11 + m7*other.m15;
    result.m8 = m8*other.m0 + m9*other.m4 + m10*other.m8 + m11*other.m12;
    result.m9 = m8*other.m1 + m9*other.m5 + m10*other.m9 + m11*other.m13;
    result.m10 = m8*other.m2 + m9*other.m6 + m10*other.m10 + m11*other.m14;
    result.m11 = m8*other.m3 + m9*other.m7 + m10*other.m11 + m11*other.m15;
    result.m12 = m12*other.m0 + m13*other.m4 + m14*other.m8 + m15*other.m12;
    result.m13 = m12*other.m1 + m13*other.m5 + m14*other.m9 + m15*other.m13;
    result.m14 = m12*other.m2 + m13*other.m6 + m14*other.m10 + m15*other.m14;
    result.m15 = m12*other.m3 + m13*other.m7 + m14*other.m11 + m15*other.m15;

    return result;
}
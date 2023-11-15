#ifndef RLGL_MATH_HPP
#define RLGL_MATH_HPP

namespace rlgl {

    constexpr float PI = 3.14159265358979323846f;
    constexpr float DEG2RAD = PI/180.0f;
    constexpr float RAD2DEG = 180.0f/PI;

    struct Matrix
    {
        float m0, m4, m8, m12;      ///< Matrix first row (4 components)
        float m1, m5, m9, m13;      ///< Matrix second row (4 components)
        float m2, m6, m10, m14;     ///< Matrix third row (4 components)
        float m3, m7, m11, m15;     ///< Matrix fourth row (4 components)

        Matrix() = default;
        Matrix(const float *mat);

        Matrix(float m0, float m4, float m8, float m12,
               float m1, float m5, float m9, float m13,
               float m2, float m6, float m10, float m14,
               float m3, float m7, float m11, float m15);

        static Matrix Identity();
        Matrix operator*(const Matrix& other) const;
    };

}

#endif

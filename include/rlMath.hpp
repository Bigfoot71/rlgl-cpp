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

        static Matrix Identity();
        Matrix operator*(const Matrix& other) const;
    };

}

#endif

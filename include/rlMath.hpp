#ifndef RLGL_MATH_HPP
#define RLGL_MATH_HPP

namespace rlgl {

    constexpr float PI = 3.14159265358979323846f;
    constexpr float DEG2RAD = PI/180.0f;
    constexpr float RAD2DEG = 180.0f/PI;

    struct Matrix
    {
        float m[16]{};

        Matrix() = default;
        Matrix(const float *mat);

        Matrix(float m0, float m4, float m8,  float m12,
               float m1, float m5, float m9,  float m13,
               float m2, float m6, float m10, float m14,
               float m3, float m7, float m11, float m15);

        static Matrix Identity();

        static Matrix Translate(float x, float y, float z);

        static Matrix Rotate(float angle, float x, float y, float z);

        static Matrix RotateX(float angle);
        static Matrix RotateY(float angle);
        static Matrix RotateZ(float angle);

        static Matrix RotateXYZ(float angleX, float angleY, float angleZ);
        static Matrix RotateZYX(float angleZ, float angleY, float angleX);

        static Matrix Scale(float x, float y, float z);

        static Matrix Frustum(float left, float right, float bottom, float top, float near, float far);

        static Matrix Perspective(float fovy, float aspect, float near, float far);
        static Matrix Ortho(float left, float right, float bottom, float top, float near, float far);

        // Calcule le déterminant de la matrice
        float Determinant() const
        {
            return m[0] * (m[5] * m[10] - m[6] * m[9]) - m[1] * (m[4] * m[10] - m[6] * m[8]) + m[2] * (m[4] * m[9] - m[5] * m[8]);
        }

        // Calcule la trace de la matrice (somme des valeurs sur la diagonale)
        float Trace() const
        {
            return m[0] + m[5] + m[10] + m[15];
        }

        // Transpose la matrice
        Matrix Transpose() const
        {
            return {
                m[0], m[1], m[2], m[3],
                m[4], m[5], m[6], m[7],
                m[8], m[9], m[10], m[11],
                m[12], m[13], m[14], m[15]
            };
        }

        // Inverse la matrice (si elle est inversible)
        Matrix Invert() const;

        operator const float*() const
        {
            return m;
        }

        // Opérateur d'addition pour la matrice 4x4
        Matrix operator+(const Matrix& other) const;

        // Opérateur de soustraction pour la matrice 4x4
        Matrix operator-(const Matrix& other) const;

        // Opérateur de multiplication de matrice 4x4
        Matrix operator*(const Matrix& other) const;

        // Opérateur de multiplication par un scalaire pour la matrice 4x4
        Matrix operator*(float scalar) const;

        // Opérateur d'égalité pour la matrice 4x4
        bool operator==(const Matrix& other) const;

        // Opérateur de différence pour la matrice 4x4
        bool operator!=(const Matrix& other) const
        {
            return !(*this == other);
        }
    };

}

#endif

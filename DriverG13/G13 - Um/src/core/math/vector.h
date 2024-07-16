#pragma once
#ifndef VECTOR_H
#define VECTOR_H
#include <cmath>
#include <iostream>
#include <windows.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

class Vector3 {
public:
    float x, y, z;

    Vector3() : x(0), y(0), z(0) {}
    Vector3(float x, float y, float z) : x(x), y(y), z(z) {}

    // Addition
    Vector3 operator+(const Vector3& v) const {
        return Vector3(x + v.x, y + v.y, z + v.z);
    }

    // Subtraction
    Vector3 operator-(const Vector3& v) const {
        return Vector3(x - v.x, y - v.y, z - v.z);
    }

    // Scalar multiplication
    Vector3 operator*(float scalar) const {
        return Vector3(x * scalar, y * scalar, z * scalar);
    }

    // Scalar division
    Vector3 operator/(float scalar) const {
        return Vector3(x / scalar, y / scalar, z / scalar);
    }

    // Dot product
    float Dot(const Vector3& v) const {
        return x * v.x + y * v.y + z * v.z;
    }

    // Cross product
    Vector3 Cross(const Vector3& v) const {
        return Vector3(
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x
        );
    }

    // Magnitude
    float Magnitude() const {
        return std::sqrt(x * x + y * y + z * z);
    }

    // Normalize
    Vector3 Normalize() const {
        float mag = Magnitude();
        return Vector3(x / mag, y / mag, z / mag);
    }

    // Distance between two vectors
    float Distance(const Vector3& v) const {
        return (*this - v).Magnitude();
    }

    // Angle between two vectors
    Vector3 RelativeAngle()
    {
        return {
            std::atan2(-z, std::hypot(x, y)) * (180.0f / 3.14159265358979323846f),
            std::atan2(y, x) * (180.0f / 3.14159265358979323846f),
            0.0f
        };
    }
    // Print vector (for debugging)
    void Print() const {
        std::cout << "Vector3(" << x << ", " << y << ", " << z << ")" << std::endl;
    }
};

// Define the view matrix structure
 struct view_matrix_t {
    float matrix[4][4];
};

 inline Vector3 worldToScreen(const view_matrix_t& matrix, const Vector3& worldPos) {
     float _x = matrix.matrix[0][0] * worldPos.x + matrix.matrix[0][1] * worldPos.y + matrix.matrix[0][2] * worldPos.z + matrix.matrix[0][3];
     float _y = matrix.matrix[1][0] * worldPos.x + matrix.matrix[1][1] * worldPos.y + matrix.matrix[1][2] * worldPos.z + matrix.matrix[1][3];
     float _w = matrix.matrix[3][0] * worldPos.x + matrix.matrix[3][1] * worldPos.y + matrix.matrix[3][2] * worldPos.z + matrix.matrix[3][3];


     float inv_w = 1.f / _w;
     _x *= inv_w;
     _y *= inv_w;

     int screen_x = static_cast<int>((0.5f * _x + 0.5f) * static_cast<float>(GetSystemMetrics(SM_CXSCREEN)));
     int screen_y = static_cast<int>((-0.5f * _y + 0.5f) * static_cast<float>(GetSystemMetrics(SM_CYSCREEN)));

     return Vector3(static_cast<float>(screen_x), static_cast<float>(screen_y), _w);
 }
 inline Vector3 CalcAngle(const Vector3& src, const Vector3& dst) {
     Vector3 delta = { dst.x - src.x, dst.y - src.y, dst.z - src.z };
     float hyp = sqrt(delta.x * delta.x + delta.y * delta.y);
     Vector3 angles = { static_cast<float>(atan(delta.z / hyp) * (180.0f / M_PI)),
                        static_cast<float>(atan(delta.y / delta.x) * (180.0f / M_PI)),
                        0.0f };
     if (delta.x >= 0.0f) {
         angles.y += 180.0f;
     }
     return angles;
 }
 inline Vector3 Lerp(Vector3 start, Vector3 end, float t) {
     return { start.x + t * (end.x - start.x), start.y + t * (end.y - start.y), start.z + t * (end.z - start.z) };
 }

 inline float MapSmoothingFactor(float smoothingFactor) {
     if (smoothingFactor <= 0.0f) return 0.0f;
     if (smoothingFactor < 1.0f) return 1.0f - smoothingFactor;
     return 1.0f / smoothingFactor;
 }

#endif // VECTOR_H

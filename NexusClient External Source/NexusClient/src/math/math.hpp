#pragma once


struct Vector3 {
    float x, y, z;
    Vector3 operator+(const Vector3& o) const { return { x + o.x, y + o.y, z + o.z }; }
    Vector3 operator-(const Vector3& o) const { return { x - o.x, y - o.y, z - o.z }; }
    Vector3 operator*(float s) const { return { x * s, y * s, z * s }; }
    Vector3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
    float LengthSq() const { return x * x + y * y + z * z; }
};

struct Vector2 {
    float x, y;
};

struct Matrix4x4 {
    float m[4][4];
};

struct QAngle {
    float x, y, z;
};

bool WorldToScreen(const Vector3& pos, Vector2& screen, const Matrix4x4& matrix, float width, float height);

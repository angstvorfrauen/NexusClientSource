
#include "math.hpp"

bool WorldToScreen(const Vector3& pos, Vector2& screen, const Matrix4x4& matrix, float width, float height) {
    float w = matrix.m[3][0] * pos.x + matrix.m[3][1] * pos.y + matrix.m[3][2] * pos.z + matrix.m[3][3];
    if (w < 0.01f) return false;
    float invW = 1.0f / w;
    float x = (matrix.m[0][0] * pos.x + matrix.m[0][1] * pos.y + matrix.m[0][2] * pos.z + matrix.m[0][3]) * invW;
    float y = (matrix.m[1][0] * pos.x + matrix.m[1][1] * pos.y + matrix.m[1][2] * pos.z + matrix.m[1][3]) * invW;
    screen.x = (width / 2.0f) * (1.0f + x);
    screen.y = (height / 2.0f) * (1.0f - y);
    return true;
}

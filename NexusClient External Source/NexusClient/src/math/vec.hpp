#pragma once

#include "../imgui/imgui.h"

namespace Overlay {
    void Box(float x, float y, float width, float height, ImU32 color, float thickness = 1.0f, bool filled = false);
    void Line(float x1, float y1, float x2, float y2, ImU32 color, float thickness = 1.0f);
    void Circle(float x, float y, float radius, ImU32 color, int segments = 32, float thickness = 1.0f, bool filled = false);
    void Text(float x, float y, const char* text, ImU32 color, bool drawShadow = true);
}

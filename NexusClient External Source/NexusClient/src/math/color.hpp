
#include "../imgui/imgui.h"

#ifndef COLOR_HPP
#define COLOR_HPP

inline ImU32 FloatToImU32(float r, float g, float b, float a) {
    return IM_COL32((int)(r * 255.0f), (int)(g * 255.0f), (int)(b * 255.0f), (int)(a * 255.0f));
}

#endif

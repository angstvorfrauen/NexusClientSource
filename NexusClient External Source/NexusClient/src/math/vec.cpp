
#include "vec.hpp"

void Overlay::Box(float x, float y, float width, float height, ImU32 color, float thickness, bool filled) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    if (filled) {
        drawList->AddRectFilled(ImVec2(x, y), ImVec2(x + width, y + height), color);
    }
    else {
        drawList->AddRect(ImVec2(x, y), ImVec2(x + width, y + height), color, 0.0f, 0, thickness);
    }
}

void Overlay::Line(float x1, float y1, float x2, float y2, ImU32 color, float thickness) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    drawList->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), color, thickness);
}

void Overlay::Circle(float x, float y, float radius, ImU32 color, int segments, float thickness, bool filled) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    if (filled) {
        drawList->AddCircleFilled(ImVec2(x, y), radius, color, segments);
    }
    else {
        drawList->AddCircle(ImVec2(x, y), radius, color, segments, thickness);
    }
}

void Overlay::Text(float x, float y, const char* text, ImU32 color, bool drawShadow) {
    ImDrawList* drawList = ImGui::GetBackgroundDrawList();
    if (drawShadow) {
        ImU32 shadowColor = IM_COL32(0, 0, 0, 255);
        drawList->AddText(ImVec2(x + 1.0f, y + 1.0f), shadowColor, text);
        drawList->AddText(ImVec2(x - 1.0f, y + 1.0f), shadowColor, text);
        drawList->AddText(ImVec2(x + 1.0f, y - 1.0f), shadowColor, text);
        drawList->AddText(ImVec2(x - 1.0f, y - 1.0f), shadowColor, text);
    }
    drawList->AddText(ImVec2(x, y), color, text);
}

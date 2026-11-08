#pragma once
#include "../imgui/imgui.h"

namespace UI {
    bool StyledCheckbox(const char* label, bool* value);
    bool BeginCard(const char* id, const char* title, const char* subtitle = nullptr, const ImVec2& size = ImVec2(0, 0));
    void EndCard();
    void SectionLabel(const char* label);
    bool AccentButton(const char* label, const ImVec2& size = ImVec2(0, 0));
    bool SecondaryButton(const char* label, const ImVec2& size = ImVec2(0, 0));
    void ValueRow(const char* label, const char* value);
    void SliderLabel(const char* label, const char* value);
    void Divider();
}

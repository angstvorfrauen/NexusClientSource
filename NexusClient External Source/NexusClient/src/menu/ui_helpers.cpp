#include "ui_helpers.hpp"
#include "menu.hpp"
#include <cmath>

namespace {
    ImVec4 Accent(float alpha = 1.0f) {
        return ImVec4(g_accentColor[0], g_accentColor[1], g_accentColor[2], alpha);
    }

    ImU32 Col(const ImVec4& c) { return ImGui::ColorConvertFloat4ToU32(c); }
}

namespace UI {

bool StyledCheckbox(const char* label, bool* value) {
    const float h = 36.0f;
    const float w = ImGui::GetContentRegionAvail().x;
    const ImVec2 p = ImGui::GetCursorScreenPos();

    ImGui::PushID(label);
    const bool pressed = ImGui::InvisibleButton("##toggle", ImVec2(w, h));
    if (pressed) *value = !*value;
    const bool hovered = ImGui::IsItemHovered();

    ImGuiID id = ImGui::GetItemID();
    ImGuiStorage* storage = ImGui::GetStateStorage();
    float anim = storage->GetFloat(id, *value ? 1.0f : 0.0f);
    const float target = *value ? 1.0f : 0.0f;
    anim += (target - anim) * ((ImGui::GetIO().DeltaTime * 16.0f) < 1.0f ? (ImGui::GetIO().DeltaTime * 16.0f) : 1.0f);
    storage->SetFloat(id, anim);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    if (hovered) {
        dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), Col(ImVec4(0.060f, 0.064f, 0.074f, 1.0f)), 7.0f);
    }

    const ImU32 labelCol = ImGui::GetColorU32(ImGuiCol_Text);
    dl->AddText(ImVec2(p.x + 3.0f, p.y + 10.0f), labelCol, label);

    const float sw = 40.0f;
    const float sh = 22.0f;
    const float sx = p.x + w - sw - 3.0f;
    const float sy = p.y + (h - sh) * 0.5f;
    const ImVec4 off = ImVec4(0.105f, 0.112f, 0.127f, 1.0f);
    const ImVec4 on = ImVec4(g_accentColor[0], g_accentColor[1], g_accentColor[2], 0.95f);
    dl->AddRectFilled(ImVec2(sx, sy), ImVec2(sx + sw, sy + sh), Col(ImVec4(
        off.x + (on.x - off.x) * anim,
        off.y + (on.y - off.y) * anim,
        off.z + (on.z - off.z) * anim,
        off.w + (on.w - off.w) * anim
    )), sh * 0.5f);
    dl->AddCircleFilled(ImVec2(sx + 11.0f + anim * 18.0f, sy + sh * 0.5f), 7.0f, Col(ImVec4(0.95f, 0.96f, 0.98f, 1.0f)));

    ImGui::PopID();
    return pressed;
}

bool BeginCard(const char* id, const char* title, const char* subtitle, const ImVec2& size) {
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(18.0f, 17.0f));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.040f, 0.043f, 0.050f, 1.0f));
    ImGui::BeginChild(id, size, true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);

    ImGui::TextUnformatted(title);
    if (subtitle && subtitle[0]) {
        ImGui::SameLine(0.0f, 9.0f);
        ImGui::TextDisabled("%s", subtitle);
    }
    ImGui::Dummy(ImVec2(0.0f, 6.0f));
    return true;
}

void EndCard() {
    ImGui::EndChild();
    ImGui::Dummy(ImVec2(0.0f, 5.0f));
}

void SectionLabel(const char* label) {
    ImGui::PushStyleColor(ImGuiCol_Text, Accent(1.0f));
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
}

bool AccentButton(const char* label, const ImVec2& size) {
    const ImVec4 a = Accent(0.94f);
    const ImVec4 h = Accent(1.0f);
    const ImVec4 p = Accent(0.78f);
    ImGui::PushStyleColor(ImGuiCol_Button, a);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, h);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, p);
    const bool r = ImGui::Button(label, size);
    ImGui::PopStyleColor(3);
    return r;
}

bool SecondaryButton(const char* label, const ImVec2& size) {
    return ImGui::Button(label, size);
}

void ValueRow(const char* label, const char* value) {
    const float valueWidth = ImGui::CalcTextSize(value).x;
    ImGui::TextDisabled("%s", label);
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - valueWidth);
    ImGui::TextUnformatted(value);
}

void SliderLabel(const char* label, const char* value) {
    ImGui::TextDisabled("%s", label);
    const float valueWidth = ImGui::CalcTextSize(value).x;
    ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - valueWidth);
    ImGui::TextUnformatted(value);
}

void Divider() {
    ImGui::Dummy(ImVec2(0, 2));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 4));
}

}

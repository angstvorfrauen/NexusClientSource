#include "tab_settings.hpp"
#include "ui_helpers.hpp"
#include "menu.hpp"

void DrawSettingsTab() {
    UI::BeginCard("##SettingsOverlay", "Overlay", "interface", ImVec2(0, 145));
    UI::StyledCheckbox("Show watermark", &g_watermarkEnabled);
    UI::EndCard();

    UI::BeginCard("##SettingsAccent", "Accent", "theme", ImVec2(0, 150));
    ImGui::TextDisabled("Menu accent");
    ImGui::SameLine();
    ImGui::ColorEdit4("##Accent", g_accentColor,
        ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    UI::EndCard();
}

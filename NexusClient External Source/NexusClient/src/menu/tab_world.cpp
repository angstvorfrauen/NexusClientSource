#include "tab_world.hpp"
#include "ui_helpers.hpp"

void DrawWorldTab() {
    UI::BeginCard("##WorldOverview", "World", "environment controls", ImVec2(0, 180));
    ImGui::TextDisabled("World options are ready for additional modules.");
    ImGui::Spacing();
    ImGui::TextDisabled("Keep this page focused: only environment-related settings belong here.");
    UI::EndCard();
}

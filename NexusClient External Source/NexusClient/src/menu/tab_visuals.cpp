#include "tab_visuals.hpp"
#include "menu.hpp"
#include "ui_helpers.hpp"

static void ColorSetting(const char* label, bool* enabled, float color[4], const char* id) {
    UI::StyledCheckbox(label, enabled);
    if (*enabled) {
        ImGui::SameLine();
        ImGui::ColorEdit4(id, color, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    }
}

void DrawVisualsTab() {
    ImGui::Columns(2, "##VisualColumns", false);
    ImGui::SetColumnWidth(0, 420.0f);

    UI::BeginCard("##VisualTarget", "Target", "entity group", ImVec2(0, 125));
    const char* targetOptions[] = { "Enemy", "Team", "Local" };
    ImGui::SetNextItemWidth(-1);
    ImGui::Combo("##VisualTargetCombo", &g_espTarget, targetOptions, IM_ARRAYSIZE(targetOptions));
    UI::EndCard();

    UI::BeginCard("##VisualBoxes", "Boxes", "geometry", ImVec2(0, 245));
    EspSettings& cur = g_espSettings[g_espTarget];
    ColorSetting("Box", &cur.box, cur.boxColor, "##BoxColor");
    ColorSetting("Corner box", &cur.cornerBox, cur.cornerColor, "##CornerColor");
    ColorSetting("Fill", &cur.fillBox, cur.fillColor, "##FillColor");
    UI::EndCard();

    ImGui::NextColumn();
    UI::BeginCard("##VisualInfo", "Information", "player data", ImVec2(0, 315));
    ColorSetting("Name", &cur.name, cur.nameColor, "##NameColor");
    ColorSetting("Snap lines", &cur.lines, cur.linesColor, "##LinesColor");
    ColorSetting("Skeleton", &cur.skeleton, cur.skeletonColor, "##SkeletonColor");
    ColorSetting("Money", &cur.money, cur.moneyColor, "##MoneyColor");
    ColorSetting("Health number", &cur.healthNumber, cur.healthNumberColor, "##HealthNumColor");
    UI::StyledCheckbox("Healthbar", &cur.health);
    UI::EndCard();

    ImGui::Columns(1);
}

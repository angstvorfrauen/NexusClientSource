#include "tab_aimbot.hpp"
#include "menu.hpp"
#include "ui_helpers.hpp"
#include <windows.h>
#include <string>

static std::string KeyToString(int vk) {
    if (vk >= 0x30 && vk <= 0x5A) return std::string(1, static_cast<char>(vk));
    switch (vk) {
    case VK_F1: return "F1"; case VK_F2: return "F2"; case VK_F3: return "F3"; case VK_F4: return "F4";
    case VK_F5: return "F5"; case VK_F6: return "F6"; case VK_F7: return "F7"; case VK_F8: return "F8";
    case VK_F9: return "F9"; case VK_F10: return "F10"; case VK_F11: return "F11"; case VK_F12: return "F12";
    case VK_LBUTTON: return "LMB"; case VK_RBUTTON: return "RMB"; case VK_MBUTTON: return "MMB";
    case VK_XBUTTON1: return "X1"; case VK_XBUTTON2: return "X2"; case VK_SHIFT: return "Shift";
    case VK_CONTROL: return "Ctrl"; case VK_MENU: return "Alt"; case VK_TAB: return "Tab";
    case VK_RETURN: return "Enter"; case VK_ESCAPE: return "Esc"; case VK_SPACE: return "Space";
    case VK_BACK: return "Back"; case VK_UP: return "Up"; case VK_DOWN: return "Down";
    case VK_LEFT: return "Left"; case VK_RIGHT: return "Right";
    default: return "Key" + std::to_string(vk);
    }
}

extern bool g_waitingForKey;

void DrawAimbotTab() {
    ImGui::Columns(2, "##AimbotColumns", false);
    ImGui::SetColumnWidth(0, 420.0f);

    UI::BeginCard("##AimbotMain", "General", "core targeting", ImVec2(0, 210));
    UI::StyledCheckbox("Enable", &g_enableAim);
    UI::StyledCheckbox("Show FOV circle", &g_showAimFov);
    if (g_showAimFov) {
        ImGui::TextDisabled("FOV color");
        ImGui::SameLine();
        ImGui::ColorEdit4("##AimFovColor", g_aimFovColor,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
    }
    ImGui::Dummy(ImVec2(0, 3));
    UI::SectionLabel("Target");
    const char* bones[] = { "Head", "Neck", "Left Arm", "Right Arm", "Body", "Feet" };
    ImGui::SetNextItemWidth(-1);
    ImGui::Combo("##TargetBone", &g_selectedBone, bones, IM_ARRAYSIZE(bones));
    UI::EndCard();

    UI::BeginCard("##AimbotPrecision", "Precision", "movement response", ImVec2(0, 250));
    ImGui::TextDisabled("Smoothness");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 65.0f);
    ImGui::Text("%.1f", g_aimSmooth);
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##Smoothness", &g_aimSmooth, 1.0f, 90.0f, "");
    ImGui::Spacing();
    ImGui::TextDisabled("Aimbot FOV");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 65.0f);
    ImGui::Text("%.1f", g_aimFov);
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##AimbotFov", &g_aimFov, 1.0f, 360.0f, "");
    UI::EndCard();

    ImGui::NextColumn();
    UI::BeginCard("##SmartTargeting", "Smart targeting", "priority model", ImVec2(0, 355));
    UI::StyledCheckbox("Prioritize threats", &g_prioritizeThreat);
    ImGui::TextDisabled("Health weight");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 55.0f);
    ImGui::Text("%.1f", g_healthWeight);
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##HealthWeight", &g_healthWeight, 0.0f, 5.0f, "");
    ImGui::TextDisabled("Distance penalty");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 55.0f);
    ImGui::Text("%.1f", g_distanceWeight);
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##DistanceWeight", &g_distanceWeight, 0.0f, 2.0f, "");
    ImGui::TextDisabled("Prediction");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 55.0f);
    ImGui::Text("%.2fs", g_aimPrediction);
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##Prediction", &g_aimPrediction, 0.0f, 0.5f, "");
    ImGui::TextDisabled("Switch delay");
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 55.0f);
    ImGui::Text("%.0fms", g_targetSwitchDelay);
    ImGui::SetNextItemWidth(-1);
    ImGui::SliderFloat("##SwitchDelay", &g_targetSwitchDelay, 0.0f, 1000.0f, "");
    UI::EndCard();

    UI::BeginCard("##AimbotKeybind", "Activation", "optional keybind", ImVec2(0, 135));
    UI::StyledCheckbox("Use keybind", &g_aimbotUseKeybind);
    if (g_aimbotUseKeybind) {
        ImGui::TextDisabled("Current key");
        ImGui::SameLine();
        char keyLabel[32]{};
        if (g_waitingForKey) strcpy_s(keyLabel, "Press any key...");
        else strcpy_s(keyLabel, sizeof(keyLabel), KeyToString(g_aimbotKey).c_str());
        if (ImGui::Button(keyLabel, ImVec2(130, 30))) g_waitingForKey = true;
        if (g_waitingForKey) {
            for (int key = 0x01; key <= 0xFE; ++key) {
                if (GetAsyncKeyState(key) & 0x8000) {
                    g_aimbotKey = key;
                    g_waitingForKey = false;
                    break;
                }
            }
        }
    }
    UI::EndCard();

    ImGui::Columns(1);
}

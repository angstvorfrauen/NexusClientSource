#include "watermark.hpp"

#include <windows.h>
#include <sstream>
#include <string>
#include <cstring>

#include "../../menu/menu.hpp"
#include "../../memory.hpp"
#include "../../offsets/offsets.hpp"

extern MemoryManager mem;
extern uintptr_t module_base;

void RenderWatermark() {
    if (!g_watermarkEnabled) return;

    const ImVec2 display = ImGui::GetIO().DisplaySize;
    constexpr float width = 330.0f;
    constexpr float height = 52.0f;
    const float x = display.x - width - 14.0f;
    const float y = 14.0f;

    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 9.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

    ImGui::Begin("##NexusWatermark", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoInputs);

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();
    const ImVec2 max = ImVec2(pos.x + size.x, pos.y + size.y);
    for (int i = 1; i <= 3; ++i) {
        const float off = static_cast<float>(i) * 1.2f;
        draw->AddRect(ImVec2(pos.x - off, pos.y - off), ImVec2(max.x + off, max.y + off),
            ImGui::GetColorU32(ImVec4(g_accentColor[0], g_accentColor[1], g_accentColor[2], 0.035f)), 9.0f, 0, 1.0f);
    }

    draw->AddRectFilled(pos, max, ImGui::GetColorU32(ImVec4(0.035f, 0.035f, 0.043f, 0.94f)), 9.0f);
    draw->AddRect(pos, max, ImGui::GetColorU32(ImVec4(0.12f, 0.12f, 0.15f, 1.0f)), 9.0f, 0, 1.0f);
    draw->AddRectFilled(ImVec2(pos.x, pos.y), ImVec2(pos.x + 3.0f, max.y),
        ImGui::GetColorU32(ImVec4(g_accentColor[0], g_accentColor[1], g_accentColor[2], 1.0f)), 2.0f);

    if (g_logoTexture) {
        const float icon = 34.0f;
        const ImVec2 iconMin(pos.x + 11.0f, pos.y + 9.0f);
        const ImVec2 iconMax(iconMin.x + icon, iconMin.y + icon);
        draw->AddImage(g_logoTexture, iconMin, iconMax);
    }

    char pcName[MAX_COMPUTERNAME_LENGTH + 1]{};
    DWORD pcNameLen = MAX_COMPUTERNAME_LENGTH + 1;
    if (!GetComputerNameA(pcName, &pcNameLen)) strcpy_s(pcName, "UNKNOWN");

    const int fps = static_cast<int>(ImGui::GetIO().Framerate);
    int ping = 0;
    if (module_base != 0 && mem.IsAttached()) {
        uintptr_t localController = mem.Read<uintptr_t>(
            module_base + cs2_dumper::offsets::client_dll::dwLocalPlayerController);
        if (localController) ping = mem.Read<int>(localController + 0x830);
    }

    std::ostringstream stats;
    stats << fps << " FPS  /  " << ping << " ms";

    const ImVec2 brandPos(pos.x + 55.0f, pos.y + 9.0f);
    draw->AddText(brandPos,
        ImGui::GetColorU32(ImVec4(0.95f, 0.95f, 0.97f, 1.0f)), "NEXUSCLIENT");
    draw->AddText(ImVec2(brandPos.x, brandPos.y + 18.0f),
        ImGui::GetColorU32(ImVec4(0.48f, 0.49f, 0.54f, 1.0f)), pcName);

    const std::string metricText = stats.str();
    const ImVec2 metricSize = ImGui::CalcTextSize(metricText.c_str());
    draw->AddText(ImVec2(max.x - metricSize.x - 13.0f, pos.y + 18.0f),
        ImGui::GetColorU32(ImVec4(g_accentColor[0], g_accentColor[1], g_accentColor[2], 1.0f)),
        metricText.c_str());

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);
}

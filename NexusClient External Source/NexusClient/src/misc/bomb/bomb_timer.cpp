
#include "../../offsets/client_dll.hpp"
#include "../../offsets/offsets.hpp"
#include "../../imgui/imgui.h"
#include "../../menu/menu.hpp"
#include "bomb_timer.hpp"

#include <cmath>
#include <cstdio>
#include <chrono>

bool g_bombPlanted = false;
float g_bombDetonateTime = 0.0f;
float g_bombTimerRemaining = 0.0f;
bool g_bombExploded = false;
float g_bombExplodedTime = 0.0f;

void UpdateBombTimer(MemoryManager& mem, uintptr_t module_base) {
    static bool wasPlanted = false;
    static int lastRound = -1;

    if (g_bombExploded) {
        static auto explodedStart = std::chrono::steady_clock::now();
        const auto now = std::chrono::steady_clock::now();
        const float elapsed = std::chrono::duration<float>(now - explodedStart).count();
        g_bombExplodedTime = 6.0f - elapsed;
        if (g_bombExplodedTime <= 0.0f) {
            g_bombExploded = false;
            g_bombExplodedTime = 0.0f;
            wasPlanted = false;
        }
        else {
            return;
        }
    }

    if (!g_bombTimerEnabled) {
        g_bombPlanted = false;
        g_bombTimerRemaining = 0.0f;
        g_bombDetonateTime = 0.0f;
        g_bombExploded = false;
        g_bombExplodedTime = 0.0f;
        return;
    }

    if (!mem.IsAttached() || module_base == 0) {
        g_bombPlanted = false;
        g_bombTimerRemaining = 0.0f;
        g_bombDetonateTime = 0.0f;
        g_bombExploded = false;
        g_bombExplodedTime = 0.0f;
        return;
    }

    const uintptr_t gameRules = mem.Read<uintptr_t>(module_base + cs2_dumper::offsets::client_dll::dwGameRules);
    int currentRound = 0;
    if (gameRules) {
        currentRound = mem.Read<int>(gameRules + 0x88);
    }

    if (currentRound != lastRound) {
        g_bombPlanted = false;
        g_bombTimerRemaining = 0.0f;
        g_bombDetonateTime = 0.0f;
        wasPlanted = false;
        lastRound = currentRound;
    }

    const uintptr_t plantedC4 = mem.Read<uintptr_t>(module_base + cs2_dumper::offsets::client_dll::dwPlantedC4);
    const uintptr_t globalVars = mem.Read<uintptr_t>(module_base + cs2_dumper::offsets::client_dll::dwGlobalVars);

    if (plantedC4 == 0 || globalVars == 0) {
        if (wasPlanted) {
            g_bombExploded = true;
            g_bombExplodedTime = 6.0f;
            static auto explodedStart = std::chrono::steady_clock::now();
            explodedStart = std::chrono::steady_clock::now();
            wasPlanted = false;
        }
        g_bombPlanted = false;
        g_bombTimerRemaining = 0.0f;
        g_bombDetonateTime = 0.0f;
        return;
    }

    const float currentTime = mem.Read<float>(globalVars + 0x30);
    const float blowTime = mem.Read<float>(plantedC4 + 0x11D0);

    if (!std::isfinite(currentTime) || !std::isfinite(blowTime)) {
        if (wasPlanted) {
            g_bombExploded = true;
            g_bombExplodedTime = 6.0f;
            static auto explodedStart = std::chrono::steady_clock::now();
            explodedStart = std::chrono::steady_clock::now();
            wasPlanted = false;
        }
        g_bombPlanted = false;
        g_bombTimerRemaining = 0.0f;
        g_bombDetonateTime = 0.0f;
        return;
    }

    const float remaining = blowTime - currentTime;
    if (!std::isfinite(remaining) || remaining <= 0.0f || remaining > 60.0f) {
        if (wasPlanted) {
            g_bombExploded = true;
            g_bombExplodedTime = 6.0f;
            static auto explodedStart = std::chrono::steady_clock::now();
            explodedStart = std::chrono::steady_clock::now();
            wasPlanted = false;
        }
        g_bombPlanted = false;
        g_bombTimerRemaining = 0.0f;
        g_bombDetonateTime = 0.0f;
        return;
    }

    g_bombPlanted = true;
    g_bombDetonateTime = blowTime;
    g_bombTimerRemaining = remaining;
    wasPlanted = true;
}

void RenderBombTimer() {
    if (!g_bombTimerEnabled) return;

    const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    constexpr float width = 250.0f;
    constexpr float height = 92.0f;
    const float x = 10.0f;
    const float y = displaySize.y - height - 10.0f;

    ImGui::SetNextWindowPos(ImVec2(x, y), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.018f, 0.018f, 0.022f, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.12f, 0.12f, 0.15f, 0.85f));

    ImGui::Begin("##BombTimer", nullptr,
        ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse);

    ImDrawList* draw = ImGui::GetWindowDrawList();
    const ImVec2 pos = ImGui::GetWindowPos();
    const ImVec2 size = ImGui::GetWindowSize();

    if (g_bombExploded) {
        const ImVec4 redColor(1.0f, 0.22f, 0.22f, 1.0f);
        const ImU32 red = ImGui::ColorConvertFloat4ToU32(redColor);
        const ImU32 dark = ImGui::ColorConvertFloat4ToU32(ImVec4(0.028f, 0.028f, 0.034f, 1.0f));
        const ImU32 line = ImGui::ColorConvertFloat4ToU32(ImVec4(0.10f, 0.10f, 0.12f, 1.0f));

        draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), dark, 10.0f);
        draw->AddRect(ImVec2(pos.x + 0.5f, pos.y + 0.5f),
            ImVec2(pos.x + size.x - 0.5f, pos.y + size.y - 0.5f), line, 10.0f);
        draw->AddRectFilled(ImVec2(pos.x + 12.0f, pos.y + 12.0f),
            ImVec2(pos.x + 40.0f, pos.y + 40.0f),
            ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.20f, 0.20f, 0.12f)), 8.0f);

        const ImVec2 bombCenter(pos.x + 26.0f, pos.y + 26.0f);
        draw->AddCircle(bombCenter, 8.0f, red, 20, 1.7f);
        draw->AddCircleFilled(bombCenter, 2.5f, red, 12);
        draw->AddLine(ImVec2(bombCenter.x + 5.5f, bombCenter.y - 5.5f),
            ImVec2(bombCenter.x + 10.0f, bombCenter.y - 10.0f), red, 1.7f);

        ImGui::SetCursorPos(ImVec2(52.0f, 12.0f));
        ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.95f, 1.0f), "Bomb");
        ImGui::SameLine(0.0f, 6.0f);
        ImGui::TextColored(ImVec4(0.42f, 0.42f, 0.47f, 1.0f), "Timer");

        const char* status = "EXPLODED";
        const ImVec2 statusSize = ImGui::CalcTextSize(status);
        ImGui::SetCursorPos(ImVec2(size.x - statusSize.x - 13.0f, 15.0f));
        ImGui::TextColored(redColor, "%s", status);

        ImGui::SetCursorPos(ImVec2(53.0f, 40.0f));
        ImGui::SetWindowFontScale(1.45f);
        ImGui::TextColored(redColor, "BOOM");
        ImGui::SetWindowFontScale(1.0f);
        ImGui::SameLine(0.0f, 8.0f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 7.0f);
        ImGui::TextColored(ImVec4(0.40f, 0.40f, 0.45f, 1.0f), "bomb detonated");

        const float barX = 53.0f;
        const float barY = 75.0f;
        const float barWidth = size.x - 66.0f;
        const float barHeight = 3.0f;
        draw->AddRectFilled(ImVec2(pos.x + barX, pos.y + barY),
            ImVec2(pos.x + barX + barWidth, pos.y + barY + barHeight),
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.07f, 0.07f, 0.085f, 1.0f)), 2.0f);

        float progress = g_bombExplodedTime / 6.0f;
        if (progress < 0.0f) progress = 0.0f;
        if (progress > 1.0f) progress = 1.0f;
        draw->AddRectFilled(ImVec2(pos.x + barX, pos.y + barY),
            ImVec2(pos.x + barX + barWidth * progress, pos.y + barY + barHeight),
            red, 2.0f);

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(3);
        return;
    }

    const float accentR = g_accentColor[0];
    const float accentG = g_accentColor[1];
    const float accentB = g_accentColor[2];
    const ImU32 accent = ImGui::ColorConvertFloat4ToU32(ImVec4(accentR, accentG, accentB, 1.0f));
    const ImU32 dark = ImGui::ColorConvertFloat4ToU32(ImVec4(0.028f, 0.028f, 0.034f, 1.0f));
    const ImU32 line = ImGui::ColorConvertFloat4ToU32(ImVec4(0.10f, 0.10f, 0.12f, 1.0f));

    draw->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), dark, 10.0f);
    draw->AddRect(ImVec2(pos.x + 0.5f, pos.y + 0.5f),
        ImVec2(pos.x + size.x - 0.5f, pos.y + size.y - 0.5f), line, 10.0f);
    draw->AddRectFilled(ImVec2(pos.x + 12.0f, pos.y + 12.0f),
        ImVec2(pos.x + 40.0f, pos.y + 40.0f),
        ImGui::ColorConvertFloat4ToU32(ImVec4(accentR, accentG, accentB, 0.12f)), 8.0f);

    const ImVec2 bombCenter(pos.x + 26.0f, pos.y + 26.0f);
    draw->AddCircle(bombCenter, 8.0f, accent, 20, 1.7f);
    draw->AddCircleFilled(bombCenter, 2.5f, accent, 12);
    draw->AddLine(ImVec2(bombCenter.x + 5.5f, bombCenter.y - 5.5f),
        ImVec2(bombCenter.x + 10.0f, bombCenter.y - 10.0f), accent, 1.7f);

    ImGui::SetCursorPos(ImVec2(52.0f, 12.0f));
    ImGui::TextColored(ImVec4(0.92f, 0.92f, 0.95f, 1.0f), "Bomb");
    ImGui::SameLine(0.0f, 6.0f);
    ImGui::TextColored(ImVec4(0.42f, 0.42f, 0.47f, 1.0f), "Timer");

    const char* status = g_bombPlanted ? "ACTIVE" : "NO BOMB";
    const ImVec2 statusSize = ImGui::CalcTextSize(status);
    ImGui::SetCursorPos(ImVec2(size.x - statusSize.x - 13.0f, 15.0f));
    if (g_bombPlanted) {
        ImGui::TextColored(ImVec4(0.35f, 0.90f, 0.52f, 1.0f), "%s", status);
    }
    else {
        ImGui::TextColored(ImVec4(0.36f, 0.36f, 0.41f, 1.0f), "%s", status);
    }

    if (g_bombPlanted) {
        float remaining = g_bombTimerRemaining;
        if (remaining < 0.0f) remaining = 0.0f;
        if (remaining > 40.0f) remaining = 40.0f;
        int seconds = static_cast<int>(std::ceil(remaining));
        if (seconds < 0) seconds = 0;

        ImVec4 timerColor;
        if (remaining <= 5.0f) {
            timerColor = ImVec4(1.0f, 0.25f, 0.25f, 1.0f);
        }
        else if (remaining <= 10.0f) {
            timerColor = ImVec4(1.0f, 0.65f, 0.25f, 1.0f);
        }
        else {
            timerColor = ImVec4(0.93f, 0.93f, 0.96f, 1.0f);
        }

        char timerText[16];
        std::snprintf(timerText, sizeof(timerText), "%02d", seconds);

        ImGui::SetCursorPos(ImVec2(53.0f, 35.0f));
        ImGui::SetWindowFontScale(1.45f);
        ImGui::TextColored(timerColor, "%s", timerText);
        ImGui::SetWindowFontScale(1.0f);
        ImGui::SameLine(0.0f, 8.0f);
        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 9.0f);
        ImGui::TextColored(ImVec4(0.40f, 0.40f, 0.45f, 1.0f), "seconds remaining");

        const float progress = remaining / 40.0f;
        const float barX = 53.0f;
        const float barY = 75.0f;
        const float barWidth = size.x - 66.0f;
        const float barHeight = 3.0f;
        draw->AddRectFilled(ImVec2(pos.x + barX, pos.y + barY),
            ImVec2(pos.x + barX + barWidth, pos.y + barY + barHeight),
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.07f, 0.07f, 0.085f, 1.0f)), 2.0f);
        draw->AddRectFilled(ImVec2(pos.x + barX, pos.y + barY),
            ImVec2(pos.x + barX + barWidth * progress, pos.y + barY + barHeight),
            ImGui::ColorConvertFloat4ToU32(timerColor), 2.0f);
    }
    else {
        ImGui::SetCursorPos(ImVec2(53.0f, 42.0f));
        ImGui::TextColored(ImVec4(0.38f, 0.38f, 0.43f, 1.0f), "Waiting for bomb...");
    }

    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(3);
}

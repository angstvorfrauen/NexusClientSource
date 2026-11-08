#include "../../offsets/client_dll.hpp"
#include "../../offsets/offsets.hpp"
#include "../../imgui/imgui.h"
#include "../../math/vec.hpp"
#include "../bones/bones.hpp"
#include "../../math/color.hpp"
#include "../../menu/menu.hpp"
#include "../../memory.hpp"
#include "health.hpp"
#include <string>

void DrawHealthESP(uintptr_t pawn, int health,
    float boxX, float boxY, float boxWidth, float boxHeight,
    const Matrix4x4& viewMatrix,
    float screenW, float screenH,
    MemoryManager& mem,
    const EspSettings& settings,
    float verticalOffset)
{
    if (settings.health) {
        const float gap = 2.5f;
        const float barWidth = 2.5f;
        float barX = boxX - gap - barWidth;
        float fullHeight = boxHeight;
        float healthHeight = (health / 100.0f) * fullHeight;
        float barY = boxY + boxHeight - healthHeight;

        ImU32 barColor = IM_COL32(
            (int)(255 - (health * 2.55f)),
            (int)(health * 2.55f),
            0, 255
        );
        Overlay::Box(barX, barY, barWidth, healthHeight, barColor, 1.0f, true);
    }

    if (settings.healthNumber) {
        std::string healthText = std::to_string(health);
        float textX = boxX + boxWidth + 3.0f;
        float textY = boxY + verticalOffset;

        ImU32 textColor = FloatToImU32(
            settings.healthNumberColor[0],
            settings.healthNumberColor[1],
            settings.healthNumberColor[2],
            settings.healthNumberColor[3]
        );
        ImFont* pixelFont = ImGui::GetIO().Fonts->Fonts.size() > 1 ? ImGui::GetIO().Fonts->Fonts[1] : nullptr;
        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        if (pixelFont) ImGui::PushFont(pixelFont);
        drawList->AddText(ImVec2(textX, textY), textColor, healthText.c_str());
        if (pixelFont) ImGui::PopFont();
    }
}
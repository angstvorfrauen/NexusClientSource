#include "../../offsets/client_dll.hpp"
#include "../../offsets/offsets.hpp"
#include "../../imgui/imgui.h"
#include "../../math/vec.hpp"
#include "../bones/bones.hpp"
#include "../../math/color.hpp"
#include "../../memory.hpp"
#include "info.hpp"
#include <string>
#include <cstring>

using namespace cs2_dumper::schemas::client_dll;

static std::string ReadString(uintptr_t address, MemoryManager& mem, size_t maxLen = 64) {
    char buffer[128] = {};
    if (mem.ReadBuffer(address, buffer, maxLen)) {
        return std::string(buffer, strnlen(buffer, maxLen));
    }
    return "";
}

PlayerInfo GetPlayerInfo(uintptr_t controller, uintptr_t pawn, uintptr_t entityList, MemoryManager& mem) {
    PlayerInfo info;
    uintptr_t moneyServices = mem.Read<uintptr_t>(
        controller + CCSPlayerController::m_pInGameMoneyServices
    );
    if (moneyServices) {
        info.money = mem.Read<int>(moneyServices + 0x40);
    }

    return info;
}

float DrawInfoESP(uintptr_t pawn, const PlayerInfo& info,
    const Matrix4x4& viewMatrix, float screenW, float screenH,
    MemoryManager& mem, const EspSettings& settings) {

    uintptr_t gameSceneNode = mem.Read<uintptr_t>(pawn + C_BaseEntity::m_pGameSceneNode);
    if (!gameSceneNode) return 0.0f;
    uintptr_t boneArray = mem.Read<uintptr_t>(gameSceneNode + CSkeletonInstance::m_modelState + 0x80);
    if (!boneArray) return 0.0f;

    uintptr_t headAddr = boneArray + (HEAD * sizeof(BoneJointData));
    uintptr_t rootAddr = boneArray + (ROOT * sizeof(BoneJointData));
    BoneJointData head = mem.Read<BoneJointData>(headAddr);
    BoneJointData root = mem.Read<BoneJointData>(rootAddr);

    Vector2 headScreen, rootScreen;
    if (!WorldToScreen(head.Pos, headScreen, viewMatrix, screenW, screenH)) return 0.0f;
    if (!WorldToScreen(root.Pos, rootScreen, viewMatrix, screenW, screenH)) return 0.0f;

    float height = rootScreen.y - headScreen.y;
    float width = height / 2.0f;
    float rightX = headScreen.x + width / 2.0f;
    float topY = headScreen.y;

    float fontSize = 13.0f;
    float lineHeight = fontSize + 2.0f;
    float currentY = topY;
    float textX = rightX + 4.0f;

    ImFont* pixelFont = ImGui::GetIO().Fonts->Fonts.size() > 1 ? ImGui::GetIO().Fonts->Fonts[1] : nullptr;

    auto DrawText = [&](const std::string& text, const float color[4], bool addShadow = true) {
        if (text.empty()) return;
        ImU32 col = FloatToImU32(color[0], color[1], color[2], color[3]);

        if (pixelFont) ImGui::PushFont(pixelFont);

        if (addShadow) {
            ImU32 shadow = IM_COL32(0, 0, 0, 200);
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(textX + 1, currentY + 1), shadow, text.c_str());
            ImGui::GetBackgroundDrawList()->AddText(ImVec2(textX - 1, currentY + 1), shadow, text.c_str());
        }
        ImGui::GetBackgroundDrawList()->AddText(ImVec2(textX, currentY), col, text.c_str());

        if (pixelFont) ImGui::PopFont();

        currentY += lineHeight;
        };

    if (settings.money) {
        DrawText("$" + std::to_string(info.money), settings.moneyColor);
    }

    return currentY - topY;
}

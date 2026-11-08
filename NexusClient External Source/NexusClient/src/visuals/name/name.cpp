#include "../../offsets/client_dll.hpp"
#include "../../offsets/offsets.hpp"
#include "../../imgui/imgui.h"
#include "../../math/vec.hpp"
#include "../bones/bones.hpp"
#include "../../math/color.hpp"
#include "../../memory.hpp"
#include "name.hpp"
#include <cstring>

void DrawNameESP(uintptr_t controller, uintptr_t pawn,
    const Matrix4x4& viewMatrix, float screenW, float screenH,
    MemoryManager& mem, const float color[4]) {
    struct NameBuffer { char data[128]; };
    NameBuffer nameBuf = mem.Read<NameBuffer>(
        controller + cs2_dumper::schemas::client_dll::CBasePlayerController::m_iszPlayerName
    );
    std::string playerName(nameBuf.data, strnlen(nameBuf.data, 128));
    if (playerName.empty()) return;

    uintptr_t gameSceneNode = mem.Read<uintptr_t>(pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_pGameSceneNode);
    if (!gameSceneNode) return;
    uintptr_t boneArray = mem.Read<uintptr_t>(gameSceneNode + cs2_dumper::schemas::client_dll::CSkeletonInstance::m_modelState + 0x80);
    if (!boneArray) return;
    uintptr_t headAddr = boneArray + (HEAD * sizeof(BoneJointData));
    uintptr_t rootAddr = boneArray + (ROOT * sizeof(BoneJointData));
    BoneJointData head = mem.Read<BoneJointData>(headAddr);
    BoneJointData root = mem.Read<BoneJointData>(rootAddr);
    Vector2 headScreen, rootScreen;
    if (!WorldToScreen(head.Pos, headScreen, viewMatrix, screenW, screenH)) return;
    if (!WorldToScreen(root.Pos, rootScreen, viewMatrix, screenW, screenH)) return;

    const char* name = playerName.c_str();
    ImVec2 textSize = ImGui::CalcTextSize(name);
    float textX = headScreen.x - textSize.x / 2.0f;
    float textY = rootScreen.y + 5.0f;

    ImU32 col = FloatToImU32(color[0], color[1], color[2], color[3]);
    ImGui::GetForegroundDrawList()->AddText(
        ImGui::GetFont(), 14.0f,
        ImVec2(textX, textY),
        col,
        name
    );
}

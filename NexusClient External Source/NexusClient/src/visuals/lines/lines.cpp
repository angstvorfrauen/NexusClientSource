#include "../../offsets/client_dll.hpp"
#include "../../offsets/offsets.hpp"
#include "../../imgui/imgui.h"
#include "../bones/bones.hpp"
#include "../../math/vec.hpp"
#include "../../math/color.hpp"
#include "../../memory.hpp"
#include "lines.hpp"

void DrawLineESP(uintptr_t pawn, const Matrix4x4& viewMatrix, float screenW, float screenH,
    MemoryManager& mem, const float color[4]) {
    uintptr_t gameSceneNode = mem.Read<uintptr_t>(pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_pGameSceneNode);
    if (!gameSceneNode) return;
    uintptr_t boneArray = mem.Read<uintptr_t>(gameSceneNode + cs2_dumper::schemas::client_dll::CSkeletonInstance::m_modelState + 0x80);
    if (!boneArray) return;
    uintptr_t rootAddr = boneArray + (ROOT * sizeof(BoneJointData));
    BoneJointData root = mem.Read<BoneJointData>(rootAddr);
    Vector2 rootScreen;
    if (!WorldToScreen(root.Pos, rootScreen, viewMatrix, screenW, screenH)) return;
    float centerX = screenW / 2.0f;

    ImU32 col = FloatToImU32(color[0], color[1], color[2], color[3]);
    Overlay::Line(centerX, screenH, rootScreen.x, rootScreen.y, col, 1.0f);
}

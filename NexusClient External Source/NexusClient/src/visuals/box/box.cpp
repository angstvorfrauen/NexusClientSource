#include "../../offsets/client_dll.hpp"
#include "../../math/vec.hpp"
#include "../bones/bones.hpp"
#include "../../math/color.hpp"
#include "../../memory.hpp"
#include "box.hpp"
#include <algorithm>

static void DrawCornerBox(
    float x,
    float y,
    float width,
    float height,
    float thickness,
    float cornerLength,
    float cornerOffset,
    ImU32 topLeftColor,
    ImU32 topRightColor,
    ImU32 bottomLeftColor,
    ImU32 bottomRightColor)
{
    ImDrawList* draw = ImGui::GetBackgroundDrawList();

    float x1 = x - cornerOffset;
    float y1 = y - cornerOffset;
    float x2 = x + width + cornerOffset;
    float y2 = y + height + cornerOffset;

    draw->AddLine(
        ImVec2(x1, y1),
        ImVec2(x1 + cornerLength, y1),
        topLeftColor,
        thickness
    );

    draw->AddLine(
        ImVec2(x1, y1),
        ImVec2(x1, y1 + cornerLength),
        topLeftColor,
        thickness
    );

    draw->AddLine(
        ImVec2(x2 - cornerLength, y1),
        ImVec2(x2, y1),
        topRightColor,
        thickness
    );

    draw->AddLine(
        ImVec2(x2, y1),
        ImVec2(x2, y1 + cornerLength),
        topRightColor,
        thickness
    );

    draw->AddLine(
        ImVec2(x1, y2 - cornerLength),
        ImVec2(x1, y2),
        bottomLeftColor,
        thickness
    );

    draw->AddLine(
        ImVec2(x1, y2),
        ImVec2(x1 + cornerLength, y2),
        bottomLeftColor,
        thickness
    );

    draw->AddLine(
        ImVec2(x2 - cornerLength, y2),
        ImVec2(x2, y2),
        bottomRightColor,
        thickness
    );

    draw->AddLine(
        ImVec2(x2, y2 - cornerLength),
        ImVec2(x2, y2),
        bottomRightColor,
        thickness
    );
}

void DrawBoxESP(
    uintptr_t pawn,
    const Matrix4x4& viewMatrix,
    float screenW,
    float screenH,
    MemoryManager& mem,
    const EspSettings& settings)
{
    uintptr_t gameSceneNode = mem.Read<uintptr_t>(
        pawn +
        cs2_dumper::schemas::client_dll::C_BaseEntity::m_pGameSceneNode
    );

    if (!gameSceneNode)
        return;

    uintptr_t boneArray = mem.Read<uintptr_t>(
        gameSceneNode +
        cs2_dumper::schemas::client_dll::CSkeletonInstance::m_modelState +
        0x80
    );

    if (!boneArray)
        return;

    uintptr_t headAddr =
        boneArray + (HEAD * sizeof(BoneJointData));

    uintptr_t rootAddr =
        boneArray + (ROOT * sizeof(BoneJointData));

    BoneJointData head =
        mem.Read<BoneJointData>(headAddr);

    BoneJointData root =
        mem.Read<BoneJointData>(rootAddr);

    Vector2 headScreen;
    Vector2 rootScreen;

    if (!WorldToScreen(
        head.Pos,
        headScreen,
        viewMatrix,
        screenW,
        screenH))
        return;

    if (!WorldToScreen(
        root.Pos,
        rootScreen,
        viewMatrix,
        screenW,
        screenH))
        return;

    float height = rootScreen.y - headScreen.y;

    if (height <= 0.0f)
        return;

    float topPadding = height * 0.08f;
    float bottomPadding = height * 0.02f;

    float y = headScreen.y - topPadding;

    height += topPadding + bottomPadding;

    float width = height * 0.50f;

    float centerX = rootScreen.x;
    float x = centerX - width * 0.5f;

    ImU32 outlineCol = FloatToImU32(
        settings.boxColor[0],
        settings.boxColor[1],
        settings.boxColor[2],
        settings.boxColor[3]
    );

    Overlay::Box(
        x,
        y,
        width,
        height,
        outlineCol,
        1.0f,
        false
    );

    if (settings.fillBox)
    {
        ImU32 fillCol = FloatToImU32(
            settings.fillColor[0],
            settings.fillColor[1],
            settings.fillColor[2],
            settings.fillColor[3]
        );

        Overlay::Box(
            x,
            y,
            width,
            height,
            fillCol,
            2.0f,
            true
        );
    }

    if (settings.cornerBox)
    {
        ImU32 cornerCol = FloatToImU32(
            settings.cornerColor[0],
            settings.cornerColor[1],
            settings.cornerColor[2],
            settings.cornerColor[3]
        );

        float cornerLength = std::clamp(
            width * 0.12f,
            4.0f,
            10.0f
        );

        float cornerOffset = 1.5f;

        DrawCornerBox(
            x,
            y,
            width,
            height,
            1.5f,
            cornerLength,
            cornerOffset,
            cornerCol,
            cornerCol,
            cornerCol,
            cornerCol
        );
    }
}

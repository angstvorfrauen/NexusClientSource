#include "../offsets/client_dll.hpp"
#include "../offsets/offsets.hpp"
#include "health/health.hpp"
#include "../menu/menu.hpp"
#include "bones/bones.hpp"
#include "lines/lines.hpp"
#include "../math/vec.hpp"
#include "../math/color.hpp"
#include "box/box.hpp"
#include "esp.hpp"
#include <vector>
#include "name/name.hpp"
#include "info/info.hpp"

static void DrawSingleESP(uintptr_t pawn, uintptr_t controller, int health, int team,
    uintptr_t entityList,
    const Matrix4x4& viewMatrix, float screenW, float screenH,
    MemoryManager& mem, const EspSettings& s) {
    if (!pawn || health <= 0 || health > 1000) return;
    float boxX = 0, boxY = 0, boxWidth = 0, boxHeight = 0;
    bool boxValid = false;
    uintptr_t gameSceneNode = mem.Read<uintptr_t>(pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_pGameSceneNode);
    if (gameSceneNode) {
        uintptr_t boneArray = mem.Read<uintptr_t>(gameSceneNode + cs2_dumper::schemas::client_dll::CSkeletonInstance::m_modelState + 0x80);
        if (boneArray) {
            BoneJointData head = mem.Read<BoneJointData>(boneArray + (HEAD * sizeof(BoneJointData)));
            BoneJointData root = mem.Read<BoneJointData>(boneArray + (ROOT * sizeof(BoneJointData)));
            Vector2 headScreen, rootScreen;
            if (WorldToScreen(head.Pos, headScreen, viewMatrix, screenW, screenH) &&
                WorldToScreen(root.Pos, rootScreen, viewMatrix, screenW, screenH)) {
                float height = rootScreen.y - headScreen.y;
                if (height > 0.0f) {
                    float topPadding = height * 0.08f;
                    float bottomPadding = height * 0.02f;
                    boxY = headScreen.y - topPadding;
                    boxHeight = height + topPadding + bottomPadding;
                    boxWidth = boxHeight * 0.50f;
                    float centerX = rootScreen.x;
                    boxX = centerX - boxWidth * 0.5f;
                    boxValid = true;
                }
            }
        }
    }
    float infoHeight = 0.0f;
    if (s.money) {
        PlayerInfo info = GetPlayerInfo(controller, pawn, entityList, mem);
        infoHeight = DrawInfoESP(pawn, info, viewMatrix, screenW, screenH, mem, s);
    }
    if (s.health || s.healthNumber) {
        if (boxValid) {
            DrawHealthESP(pawn, health, boxX, boxY, boxWidth, boxHeight,
                viewMatrix, screenW, screenH, mem, s, infoHeight);
        }
    }

    if (s.name && controller)
        DrawNameESP(controller, pawn, viewMatrix, screenW, screenH, mem, s.nameColor);

    if (s.box)
        DrawBoxESP(pawn, viewMatrix, screenW, screenH, mem, s);

    if (s.lines)
        DrawLineESP(pawn, viewMatrix, screenW, screenH, mem, s.linesColor);

    if (s.skeleton) {
        uintptr_t gameSceneNode2 = mem.Read<uintptr_t>(pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_pGameSceneNode);
        if (!gameSceneNode2) return;
        uintptr_t boneArray2 = mem.Read<uintptr_t>(gameSceneNode2 + cs2_dumper::schemas::client_dll::CSkeletonInstance::m_modelState + 0x80);
        if (!boneArray2) return;

        std::vector<BoneJointPos> bonePosList(30);
        unsigned int targetedBones[] = {
            ROOT, PELVIS, SPINE1, SPINE2, CHEST, NECK, HEAD,
            SHOULDER_L, ELBOW_L, HAND_L,
            SHOULDER_R, ELBOW_R, HAND_R,
            HIP_L, KNEE_L, FOOT_HEEL_L,
            HIP_R, KNEE_R, FOOT_HEEL_R
        };
        for (unsigned int id : targetedBones) {
            uintptr_t jointAddr = boneArray2 + (id * sizeof(BoneJointData));
            BoneJointData raw = mem.Read<BoneJointData>(jointAddr);
            Vector2 screen;
            if (WorldToScreen(raw.Pos, screen, viewMatrix, screenW, screenH)) {
                bonePosList[id].Pos = raw.Pos;
                bonePosList[id].ScreenPos = screen;
            }
        }
        Vector2 headPos = bonePosList[HEAD].ScreenPos;
        Vector2 neckPos = bonePosList[NECK].ScreenPos;
        if (headPos.x == 0 && headPos.y == 0) return;
        ImU32 boneColor = FloatToImU32(s.skeletonColor[0], s.skeletonColor[1], s.skeletonColor[2], s.skeletonColor[3]);
        for (auto& link : SKELETON_LINKS) {
            Vector2 p1 = bonePosList[link.a].ScreenPos;
            Vector2 p2 = bonePosList[link.b].ScreenPos;
            if (p1.x != 0 && p1.y != 0 && p2.x != 0 && p2.y != 0) {
                Overlay::Line(p1.x, p1.y, p2.x, p2.y, boneColor, 1.5f);
            }
        }
        Vector2 rootScreen = bonePosList[ROOT].ScreenPos;
        if (rootScreen.x == 0 && rootScreen.y == 0) return;
        float height2 = rootScreen.y - headPos.y;
        float headRadius = height2 * 0.15f;
        if (headRadius < 2.0f) headRadius = 2.0f;
        Overlay::Circle(headPos.x, headPos.y, headRadius, boneColor, 24, 1.5f);
    }
}

void RunEsp(uintptr_t localPawn, int localTeam, uintptr_t entityList,
    const Matrix4x4& viewMatrix, float screenW, float screenH,
    uintptr_t module_base, MemoryManager& mem) {

    const EspSettings& localSettings = g_espSettings[2];
    if (localPawn && (localSettings.box || localSettings.lines ||
        localSettings.skeleton || localSettings.health || localSettings.name || localSettings.money)) {

        uintptr_t localController = mem.Read<uintptr_t>(
            module_base + cs2_dumper::offsets::client_dll::dwLocalPlayerController
        );
        int health = mem.Read<int>(
            localPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iHealth
        );
        int team = mem.Read<int>(
            localPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum
        );

        DrawSingleESP(localPawn, localController, health, team, entityList,
            viewMatrix, screenW, screenH, mem, localSettings);
    }

    for (int i = 1; i < 64; i++) {
        uintptr_t listEntry = mem.Read<uintptr_t>(entityList + (8 * ((i & 0x7FFF) >> 9)) + 16);
        if (!listEntry) continue;
        uintptr_t controller = mem.Read<uintptr_t>(listEntry + 112 * (i & 0x1FF));
        if (controller < 0x1000000 || (controller >> 48) != 0) continue;
        uint32_t pawnHandle = mem.Read<uint32_t>(controller + cs2_dumper::schemas::client_dll::CCSPlayerController::m_hPlayerPawn);
        if (!pawnHandle) continue;
        uintptr_t listEntry2 = mem.Read<uintptr_t>(entityList + (8 * ((pawnHandle & 0x7FFF) >> 9)) + 16);
        if (!listEntry2) continue;
        uintptr_t pawn = mem.Read<uintptr_t>(listEntry2 + 112 * (pawnHandle & 0x1FF));
        if (!pawn || pawn == localPawn) continue;

        int health = mem.Read<int>(pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iHealth);
        int team = mem.Read<int>(pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);
        if (health <= 0 || health > 1000) continue;

        bool isEnemy = (team != localTeam && team != 0);
        bool isTeam = (team == localTeam && team != 0);

        if (isEnemy) {
            const EspSettings& enemySettings = g_espSettings[0];
            if (enemySettings.box || enemySettings.lines || enemySettings.skeleton ||
                enemySettings.health || enemySettings.name || enemySettings.money) {
                DrawSingleESP(pawn, controller, health, team, entityList,
                    viewMatrix, screenW, screenH, mem, enemySettings);
            }
            continue;
        }

        if (isTeam) {
            const EspSettings& teamSettings = g_espSettings[1];
            if (teamSettings.box || teamSettings.lines || teamSettings.skeleton ||
                teamSettings.health || teamSettings.name || teamSettings.money) {
                DrawSingleESP(pawn, controller, health, team, entityList,
                    viewMatrix, screenW, screenH, mem, teamSettings);
            }
            continue;
        }
    }
}

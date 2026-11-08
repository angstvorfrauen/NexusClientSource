#include "../offsets/client_dll.hpp"
#include "../offsets/offsets.hpp"
#include "../math/color.hpp"
#include "../menu/menu.hpp"
#include "../math/math.hpp"
#include "../math/vec.hpp"
#include "aimbot.hpp"
#include <windows.h>
#include <cmath>
#include <chrono>
#include "../visuals/bones/bones.hpp"

static inline Vector3 MulVector3(const Vector3& v, float s) {
    return { v.x * s, v.y * s, v.z * s };
}

static const int boneTargets[] = {
    HEAD,
    NECK,
    SHOULDER_L,
    SHOULDER_R,
    CHEST,
    FOOT_HEEL_L
};

static uintptr_t currentTargetPawn = 0;
static int currentTargetBoneIndex = 0;
static std::chrono::steady_clock::time_point lastSwitchTime = std::chrono::steady_clock::now();

void RunAimbot(uintptr_t localPawn, int localTeam, uintptr_t entityList,
    const Matrix4x4& viewMatrix, float screenW, float screenH,
    uintptr_t module_base, MemoryManager& mem) {

    if (!g_enableAim) {
        currentTargetPawn = 0;
        return;
    }

    float centerX = screenW / 2.0f;
    float centerY = screenH / 2.0f;
    if (g_showAimFov) {
        ImU32 circleColor = FloatToImU32(g_aimFovColor[0], g_aimFovColor[1], g_aimFovColor[2], g_aimFovColor[3]);
        Overlay::Circle(centerX, centerY, g_aimFov, circleColor, 32, 1.0f);
    }

    int targetBoneIndex = boneTargets[g_selectedBone];

    uintptr_t bestPawn = 0;
    float bestScore = -FLT_MAX;
    Vector2 bestScreenPos = { 0, 0 };
    int bestBone = targetBoneIndex;
    bool keepCurrent = false;
    if (currentTargetPawn != 0) {
        int health = mem.Read<int>(currentTargetPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iHealth);
        int team = mem.Read<int>(currentTargetPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);
        if (health > 0 && health <= 1000 && team != localTeam && team != 0) {
            uintptr_t gameSceneNode = mem.Read<uintptr_t>(currentTargetPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_pGameSceneNode);
            if (gameSceneNode) {
                uintptr_t boneArray = mem.Read<uintptr_t>(gameSceneNode + cs2_dumper::schemas::client_dll::CSkeletonInstance::m_modelState + 0x80);
                if (boneArray) {
                    struct BoneJointData { Vector3 Pos; char pad[0x14]; };
                    uintptr_t jointAddr = boneArray + (targetBoneIndex * sizeof(BoneJointData));
                    BoneJointData joint = mem.Read<BoneJointData>(jointAddr);
                    Vector2 screenPos;
                    if (WorldToScreen(joint.Pos, screenPos, viewMatrix, screenW, screenH)) {
                        float dx = screenPos.x - centerX;
                        float dy = screenPos.y - centerY;
                        float dist = sqrtf(dx * dx + dy * dy);
                        if (dist <= g_aimFov) {
                            keepCurrent = true;
                            bestPawn = currentTargetPawn;
                            bestScreenPos = screenPos;
                            bestBone = targetBoneIndex;
                        }
                    }
                }
            }
        }
    }

    if (!keepCurrent) {
        currentTargetPawn = 0;
        bestPawn = 0;
        bestScore = -FLT_MAX;
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
        if (team == localTeam && team != 0) continue;

        uintptr_t gameSceneNode = mem.Read<uintptr_t>(pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_pGameSceneNode);
        if (!gameSceneNode) continue;
        uintptr_t boneArray = mem.Read<uintptr_t>(gameSceneNode + cs2_dumper::schemas::client_dll::CSkeletonInstance::m_modelState + 0x80);
        if (!boneArray) continue;

        struct BoneJointData { Vector3 Pos; char pad[0x14]; };
        uintptr_t jointAddr = boneArray + (targetBoneIndex * sizeof(BoneJointData));
        BoneJointData joint = mem.Read<BoneJointData>(jointAddr);
        Vector2 screenPos;
        if (!WorldToScreen(joint.Pos, screenPos, viewMatrix, screenW, screenH)) continue;

        float dx = screenPos.x - centerX;
        float dy = screenPos.y - centerY;
        float distance = sqrtf(dx * dx + dy * dy);
        if (distance > g_aimFov) continue;

        float score = 0.0f;
        score += (100.0f - health) * g_healthWeight;
        float normDist = distance / g_aimFov;
        score -= normDist * g_distanceWeight * 100.0f;

        if (g_prioritizeThreat) {
            Vector3 velocity = mem.Read<Vector3>(pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_vecVelocity);
            float speed = sqrtf(velocity.x * velocity.x + velocity.y * velocity.y + velocity.z * velocity.z);
            if (speed > 50.0f) score += 20.0f;
        }

        if (g_aimPrediction > 0.0f) {
            Vector3 velocity = mem.Read<Vector3>(pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_vecVelocity);
            Vector3 predictedPos = MulVector3(velocity, g_aimPrediction);
            predictedPos.x += joint.Pos.x;
            predictedPos.y += joint.Pos.y;
            predictedPos.z += joint.Pos.z;
            Vector2 predictedScreen;
            if (WorldToScreen(predictedPos, predictedScreen, viewMatrix, screenW, screenH)) {
                float pdx = predictedScreen.x - centerX;
                float pdy = predictedScreen.y - centerY;
                float predDist = sqrtf(pdx * pdx + pdy * pdy);
                if (predDist < distance) {
                    score += (distance - predDist) * 0.5f;
                }
            }
        }

        if (pawn == currentTargetPawn) {
            score += 15.0f;
        }

        if (score > bestScore) {
            bestScore = score;
            bestPawn = pawn;
            bestScreenPos = screenPos;
            bestBone = targetBoneIndex;
        }
    }

    if (bestPawn != 0) {
        if (currentTargetPawn != 0 && bestPawn != currentTargetPawn) {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastSwitchTime).count();
            if (elapsed >= g_targetSwitchDelay) {
                currentTargetPawn = bestPawn;
                lastSwitchTime = now;
            }
        }
        else if (currentTargetPawn == 0) {
            currentTargetPawn = bestPawn;
            lastSwitchTime = std::chrono::steady_clock::now();
        }
    }
    else {
        currentTargetPawn = 0;
    }

    if (currentTargetPawn != 0) {
        uintptr_t gameSceneNode = mem.Read<uintptr_t>(currentTargetPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_pGameSceneNode);
        if (gameSceneNode) {
            uintptr_t boneArray = mem.Read<uintptr_t>(gameSceneNode + cs2_dumper::schemas::client_dll::CSkeletonInstance::m_modelState + 0x80);
            if (boneArray) {
                struct BoneJointData { Vector3 Pos; char pad[0x14]; };
                uintptr_t jointAddr = boneArray + (bestBone * sizeof(BoneJointData));
                BoneJointData joint = mem.Read<BoneJointData>(jointAddr);
                Vector2 screenPos;
                if (WorldToScreen(joint.Pos, screenPos, viewMatrix, screenW, screenH)) {
                    Overlay::Line(centerX, centerY, screenPos.x, screenPos.y,
                        IM_COL32(255, 0, 0, 255), 1.0f);

                    bool shouldAim = false;
                    if (g_aimbotUseKeybind) {
                        if (GetAsyncKeyState(g_aimbotKey) & 0x8000)
                            shouldAim = true;
                    }
                    else {
                        if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
                            shouldAim = true;
                    }

                    if (shouldAim) {
                        float moveX = (screenPos.x - centerX) / g_aimSmooth;
                        float moveY = (screenPos.y - centerY) / g_aimSmooth;
                        INPUT input = { 0 };
                        input.type = INPUT_MOUSE;
                        input.mi.dwFlags = MOUSEEVENTF_MOVE;
                        input.mi.dx = static_cast<LONG>(moveX);
                        input.mi.dy = static_cast<LONG>(moveY);
                        SendInput(1, &input, sizeof(INPUT));
                    }
                }
            }
        }
    }
}
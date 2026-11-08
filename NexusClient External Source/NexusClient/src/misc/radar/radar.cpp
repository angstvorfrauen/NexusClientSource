
#include "radar.hpp"
#include "../../offsets/client_dll.hpp"

#include <unordered_map>
#include <cstdint>

namespace {
    struct RadarState {
        bool spotted{};
        uint32_t mask0{};
        uint32_t mask1{};
    };

    std::unordered_map<uintptr_t, RadarState> g_originalStates;
    bool g_wasEnabled = false;
    int g_clearFrames = 0;

    constexpr uintptr_t spottedStateOffset =
        cs2_dumper::schemas::client_dll::C_CSPlayerPawn::m_entitySpottedState;

    constexpr uintptr_t spottedOffset = 0x8;
    constexpr uintptr_t spottedMaskOffset = 0xC;
}

static uintptr_t GetPawn(MemoryManager& mem, uintptr_t entityList, int index) {
    uintptr_t listEntry = mem.Read<uintptr_t>(entityList + (8 * ((index & 0x7FFF) >> 9)) + 16);
    if (!listEntry)
        return 0;

    uintptr_t controller = mem.Read<uintptr_t>(listEntry + 112 * (index & 0x1FF));
    if (!controller)
        return 0;

    uint32_t pawnHandle = mem.Read<uint32_t>(controller + cs2_dumper::schemas::client_dll::CCSPlayerController::m_hPlayerPawn);
    if (!pawnHandle)
        return 0;

    uintptr_t pawnListEntry = mem.Read<uintptr_t>(entityList + (8 * ((pawnHandle & 0x7FFF) >> 9)) + 16);
    if (!pawnListEntry)
        return 0;

    return mem.Read<uintptr_t>(pawnListEntry + 112 * (pawnHandle & 0x1FF));
}

static void ClearRadar(MemoryManager& mem, uintptr_t localPawn, uintptr_t entityList) {
    if (!localPawn || !entityList)
        return;

    for (int i = 1; i < 64; ++i) {
        uintptr_t pawn = GetPawn(mem, entityList, i);
        if (!pawn || pawn == localPawn)
            continue;

        int health = mem.Read<int>(pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iHealth);
        if (health <= 0 || health > 1000)
            continue;

        uintptr_t state = pawn + spottedStateOffset;
        mem.Write<bool>(state + spottedOffset, false);
        mem.Write<uint32_t>(state + spottedMaskOffset, 0);
        mem.Write<uint32_t>(state + spottedMaskOffset + 4, 0);
    }
}

void UpdateRadarHack(MemoryManager& mem, uintptr_t localPawn, int localTeam, uintptr_t entityList) {
    if (!localPawn || !entityList)
        return;

    if (!g_radarHackEnabled) {
        if (g_wasEnabled) {
            g_clearFrames = 30;
            g_wasEnabled = false;
        }

        if (g_clearFrames > 0) {
            ClearRadar(mem, localPawn, entityList);
            --g_clearFrames;
        }
        return;
    }

    g_wasEnabled = true;

    for (int i = 1; i < 64; ++i) {
        uintptr_t pawn = GetPawn(mem, entityList, i);
        if (!pawn || pawn == localPawn)
            continue;

        int health = mem.Read<int>(pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iHealth);
        if (health <= 0 || health > 1000)
            continue;

        int team = mem.Read<int>(pawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);
        if (team == 0 || team == localTeam)
            continue;

        uintptr_t state = pawn + spottedStateOffset;

        if (!g_originalStates.contains(pawn)) {
            RadarState original{};
            original.spotted = mem.Read<bool>(state + spottedOffset);
            original.mask0 = mem.Read<uint32_t>(state + spottedMaskOffset);
            original.mask1 = mem.Read<uint32_t>(state + spottedMaskOffset + 4);
            g_originalStates[pawn] = original;
        }

        mem.Write<bool>(state + spottedOffset, true);
        mem.Write<uint32_t>(state + spottedMaskOffset, 0xFFFFFFFF);
        mem.Write<uint32_t>(state + spottedMaskOffset + 4, 0xFFFFFFFF);
    }
}

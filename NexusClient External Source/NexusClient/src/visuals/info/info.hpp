#pragma once
#include <cstdint>
#include <string>
#include "../../math/math.hpp"
#include "../../memory.hpp"
#include "../../menu/menu.hpp"

struct PlayerInfo {
    int money = 0;
};

PlayerInfo GetPlayerInfo(uintptr_t controller, uintptr_t pawn, uintptr_t entityList, MemoryManager& mem);
float DrawInfoESP(uintptr_t pawn, const PlayerInfo& info,
    const Matrix4x4& viewMatrix, float screenW, float screenH,
    MemoryManager& mem, const EspSettings& settings);

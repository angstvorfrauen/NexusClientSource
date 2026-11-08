#pragma once
#include <cstdint>
#include "../../math/math.hpp"
#include "../../memory.hpp"
#include "../../menu/menu.hpp"

void DrawHealthESP(uintptr_t pawn, int health,
    float boxX, float boxY, float boxWidth, float boxHeight,
    const Matrix4x4& viewMatrix,
    float screenW, float screenH,
    MemoryManager& mem,
    const EspSettings& settings,
    float verticalOffset = 0.0f);
#pragma once
#include <cstdint>
#include "../../math/Math.hpp"
#include "../../memory.hpp"
#include "../../menu/menu.hpp"

void DrawBoxESP(uintptr_t pawn, const Matrix4x4& viewMatrix, float screenW, float screenH,
    MemoryManager& mem, const EspSettings& settings);

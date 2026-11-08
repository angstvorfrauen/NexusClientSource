#pragma once
#include <cstdint>
#include "../../math/math.hpp"
#include "../../memory.hpp"

void DrawLineESP(uintptr_t pawn, const Matrix4x4& viewMatrix, float screenW, float screenH,
    MemoryManager& mem, const float color[4]);

#pragma once
#include <cstdint>
#include "../math/math.hpp"
#include "../memory.hpp"

void RunEsp(uintptr_t localPawn, int localTeam, uintptr_t entityList, const Matrix4x4& viewMatrix, float screenW, float screenH, uintptr_t module_base, MemoryManager& mem);

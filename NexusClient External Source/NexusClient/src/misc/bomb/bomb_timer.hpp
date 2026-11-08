#pragma once

#include "../../memory.hpp"
#include <cstdint>

extern bool g_bombTimerEnabled;
extern bool g_bombPlanted;
extern float g_bombDetonateTime;
extern float g_bombTimerRemaining;
extern bool g_bombExploded;

void UpdateBombTimer(MemoryManager& mem, uintptr_t module_base);
void RenderBombTimer();

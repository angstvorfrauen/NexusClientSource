#pragma once

#include "../../memory.hpp"
#include <cstdint>

extern bool g_radarHackEnabled;

void UpdateRadarHack(MemoryManager& mem, uintptr_t localPawn, int localTeam, uintptr_t entityList);

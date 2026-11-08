#pragma once

#include "../../memory.hpp"
#include <cstdint>

extern bool g_antiFlashEnabled;

void UpdateAntiFlash(MemoryManager& mem, uintptr_t local_pawn);

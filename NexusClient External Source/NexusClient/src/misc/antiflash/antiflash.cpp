
#include "../../offsets/client_dll.hpp"
#include "antiflash.hpp"

static bool wasAntiFlashEnabled = false;

void UpdateAntiFlash(MemoryManager& mem,uintptr_t local_pawn){
    if (!local_pawn)return;
    constexpr uintptr_t FLASH_BANG_TIME = 0x1414;
    constexpr uintptr_t FLASH_SCREENSHOT = 0x1418;
    constexpr uintptr_t FLASH_OVERLAY_ALPHA = 0x141C;
    constexpr uintptr_t FLASH_BUILD_UP = 0x1420;
    constexpr uintptr_t FLASH_DSP_CLEARED = 0x1421;
    constexpr uintptr_t FLASH_SCREENSHOT_GRAB = 0x1422;
    constexpr uintptr_t FLASH_MAX_ALPHA = 0x1424;
    constexpr uintptr_t FLASH_DURATION = 0x1428;

    if (!g_antiFlashEnabled && wasAntiFlashEnabled)
    {
        mem.Write<float>(local_pawn + FLASH_BANG_TIME,0.0f);
        mem.Write<float>(local_pawn + FLASH_SCREENSHOT,1.0f);
        mem.Write<float>(local_pawn + FLASH_OVERLAY_ALPHA,0.0f);
        mem.Write<bool>(local_pawn + FLASH_BUILD_UP,false);
        mem.Write<bool>(local_pawn + FLASH_DSP_CLEARED,false);
        mem.Write<bool>(local_pawn + FLASH_SCREENSHOT_GRAB,false);
        mem.Write<float>(local_pawn + FLASH_MAX_ALPHA,255.0f);
        mem.Write<float>(local_pawn + FLASH_DURATION,0.0f);
        wasAntiFlashEnabled = false;
        return;
    }

    if (!g_antiFlashEnabled)return;
    wasAntiFlashEnabled = true;
    mem.Write<float>(local_pawn + FLASH_BANG_TIME,0.0f);
    mem.Write<float>(local_pawn + FLASH_SCREENSHOT,0.0f);
    mem.Write<float>(local_pawn + FLASH_OVERLAY_ALPHA,0.0f);
    mem.Write<float>(local_pawn + FLASH_MAX_ALPHA,0.0f);
    mem.Write<float>(local_pawn + FLASH_DURATION,0.0f);
    mem.Write<bool>(local_pawn + FLASH_BUILD_UP,false);
    mem.Write<bool>(local_pawn + FLASH_DSP_CLEARED,true);
    mem.Write<bool>(local_pawn + FLASH_SCREENSHOT_GRAB,false);

}

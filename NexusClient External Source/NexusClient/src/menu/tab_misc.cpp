#include "tab_misc.hpp"
#include "ui_helpers.hpp"
#include "menu.hpp"

void DrawMiscTab() {
    UI::BeginCard("##MiscUtilities", "Utilities", "quality of life", ImVec2(0, 185));
    UI::StyledCheckbox("Bomb timer", &g_bombTimerEnabled);
    UI::StyledCheckbox("Anti flash", &g_antiFlashEnabled);
    UI::StyledCheckbox("Radar", &g_radarHackEnabled);
    UI::EndCard();
}

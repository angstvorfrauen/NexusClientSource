#pragma once

#include "../imgui/imgui.h"

enum MenuTab {
    TAB_AIMBOT = 0,
    TAB_VISUALS,
    TAB_WORLD,
    TAB_MISC,
    TAB_CONFIG,
    TAB_SETTINGS,
    TAB_COUNT
};

struct EspSettings {
    bool box = false;
    float boxColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool fillBox = false;
    float fillColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool cornerBox = false;
    float cornerColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool lines = false;
    float linesColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool skeleton = false;
    float skeletonColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool name = false;
    float nameColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool health = false;
    bool healthNumber = false;
    float healthNumberColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    bool money = false;
    float moneyColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
};

extern float g_aimFov;
extern float g_aimSmooth;
extern int g_selectedBone;
extern MenuTab g_activeTab;
extern bool g_enableAim;
extern bool g_showAimFov;
extern float g_aimFovColor[4];
extern int g_espTarget;
extern EspSettings g_espSettings[3];
extern bool g_menuOpen;
extern bool g_exitApp;
extern float g_accentColor[4];
extern bool g_aimbotUseKeybind;
extern int g_aimbotKey;
extern bool g_bombTimerEnabled;
extern bool g_cheatReady;
extern float g_menuFadeAlpha;
extern bool g_menuClosing;
extern float g_aimPrediction;
extern float g_targetSwitchDelay;
extern bool g_prioritizeThreat;
extern float g_healthWeight;
extern float g_distanceWeight;
extern bool g_grenadeHelperEnabled;
extern bool g_antiFlashEnabled;
extern bool g_radarHackEnabled;
extern bool g_watermarkEnabled;
extern bool g_waitingForKey;

extern ImTextureID g_logoTexture;

void InitSettings();

class Menu {
public:
    static void Toggle();
    static void Render();
};

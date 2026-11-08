#include "../misc/antiflash/antiflash.hpp"
#include "../misc/bomb/bomb_timer.hpp"
#include "../misc/radar/radar.hpp"
#include "config.hpp"

#include <filesystem>
#include <windows.h>
#include <cstring>
#include <cstdio>
#include <string>

static constexpr const char* CONFIG_DIRECTORY = "C:\\nexusclient\\cfg";

FullConfig::FullConfig()
{
    aimFov = 90.0f;
    aimSmooth = 5.0f;
    selectedBone = 0;
    enableAim = false;
    showAimFov = false;
    aimFovColor[0] = 1.0f; aimFovColor[1] = 1.0f; aimFovColor[2] = 1.0f; aimFovColor[3] = 1.0f;
    aimbotUseKeybind = false;
    aimbotKey = 0x4C;
    espTarget = 0;

    for (int i = 0; i < 3; ++i) {
        espSettings[i].box = false;
        espSettings[i].lines = false;
        espSettings[i].skeleton = false;
        espSettings[i].name = false;
        espSettings[i].health = false;
        espSettings[i].healthNumber = false;
        espSettings[i].money = false;
        espSettings[i].fillBox = false;
        espSettings[i].cornerBox = false;
        for (int c = 0; c < 4; ++c) {
            espSettings[i].boxColor[c] = 1.0f;
            espSettings[i].linesColor[c] = 1.0f;
            espSettings[i].skeletonColor[c] = 1.0f;
            espSettings[i].nameColor[c] = 1.0f;
            espSettings[i].healthNumberColor[c] = 1.0f;
            espSettings[i].moneyColor[c] = 1.0f;
            espSettings[i].fillColor[c] = 1.0f;
            espSettings[i].cornerColor[c] = 1.0f;
        }
        espSettings[i].fillColor[0] = 0.0f;
        espSettings[i].fillColor[1] = 0.0f;
        espSettings[i].fillColor[2] = 0.0f;
        espSettings[i].fillColor[3] = 0.2549f;
    }

    accentColor[0] = 0.43f; accentColor[1] = 0.35f; accentColor[2] = 0.85f; accentColor[3] = 1.0f;
    bombTimerEnabled = false;
    antiFlashEnabled = false;
    radarHackEnabled = false;
    watermarkEnabled = false;
    aimPrediction = 0.08f;
    targetSwitchDelay = 200.0f;
    prioritizeThreat = true;
    healthWeight = 2.0f;
    distanceWeight = 0.1f;
}

void EnsureConfigDirectory() {
    std::error_code error;
    std::filesystem::create_directories(CONFIG_DIRECTORY, error);
}

void SaveConfig(const char* filename, const FullConfig& cfg) {
    if (!filename || !*filename) return;
    EnsureConfigDirectory();
    FILE* f = nullptr;
    if (fopen_s(&f, filename, "wb") != 0 || !f) return;
    fwrite(&cfg, sizeof(FullConfig), 1, f);
    fclose(f);
}

bool LoadConfig(const char* filename, FullConfig& cfg) {
    if (!filename || !*filename) return false;
    FILE* f = nullptr;
    if (fopen_s(&f, filename, "rb") != 0 || !f) return false;
    size_t result = fread(&cfg, sizeof(FullConfig), 1, f);
    fclose(f);
    return result == 1;
}

void ApplyConfig(const FullConfig& cfg) {
    g_aimFov = cfg.aimFov;
    g_aimSmooth = cfg.aimSmooth;
    g_selectedBone = cfg.selectedBone;
    g_enableAim = cfg.enableAim;
    g_showAimFov = cfg.showAimFov;
    memcpy(g_aimFovColor, cfg.aimFovColor, sizeof(g_aimFovColor));
    g_aimbotUseKeybind = cfg.aimbotUseKeybind;
    g_aimbotKey = cfg.aimbotKey;
    g_espTarget = cfg.espTarget;
    memcpy(g_espSettings, cfg.espSettings, sizeof(g_espSettings));
    memcpy(g_accentColor, cfg.accentColor, sizeof(g_accentColor));
    g_bombTimerEnabled = cfg.bombTimerEnabled;
    g_antiFlashEnabled = cfg.antiFlashEnabled;
    g_radarHackEnabled = cfg.radarHackEnabled;
    g_watermarkEnabled = cfg.watermarkEnabled;
    g_aimPrediction = cfg.aimPrediction;
    g_targetSwitchDelay = cfg.targetSwitchDelay;
    g_prioritizeThreat = cfg.prioritizeThreat;
    g_healthWeight = cfg.healthWeight;
    g_distanceWeight = cfg.distanceWeight;
}

void LoadCurrentConfigFromGlobals(FullConfig& cfg) {
    cfg.aimFov = g_aimFov;
    cfg.aimSmooth = g_aimSmooth;
    cfg.selectedBone = g_selectedBone;
    cfg.enableAim = g_enableAim;
    cfg.showAimFov = g_showAimFov;
    memcpy(cfg.aimFovColor, g_aimFovColor, sizeof(g_aimFovColor));
    cfg.aimbotUseKeybind = g_aimbotUseKeybind;
    cfg.aimbotKey = g_aimbotKey;
    cfg.espTarget = g_espTarget;
    memcpy(cfg.espSettings, g_espSettings, sizeof(g_espSettings));
    memcpy(cfg.accentColor, g_accentColor, sizeof(g_accentColor));
    cfg.bombTimerEnabled = g_bombTimerEnabled;
    cfg.antiFlashEnabled = g_antiFlashEnabled;
    cfg.radarHackEnabled = g_radarHackEnabled;
    cfg.watermarkEnabled = g_watermarkEnabled;
    cfg.aimPrediction = g_aimPrediction;
    cfg.targetSwitchDelay = g_targetSwitchDelay;
    cfg.prioritizeThreat = g_prioritizeThreat;
    cfg.healthWeight = g_healthWeight;
    cfg.distanceWeight = g_distanceWeight;
}
#pragma once

#include "../menu/menu.hpp"
#include "../settings/watermark/watermark.hpp"

struct FullConfig
{
    float aimFov;
    float aimSmooth;
    int selectedBone;
    bool enableAim;
    bool showAimFov;
    float aimFovColor[4];
    bool aimbotUseKeybind;
    int aimbotKey;
    int espTarget;
    EspSettings espSettings[3];
    float accentColor[4];
    bool bombTimerEnabled;
    bool antiFlashEnabled;
    bool radarHackEnabled;
    bool watermarkEnabled;
    float aimPrediction;
    float targetSwitchDelay;
    bool prioritizeThreat;
    float healthWeight;
    float distanceWeight;

    FullConfig();
};

void SaveConfig(const char* filename, const FullConfig& cfg);
bool LoadConfig(const char* filename, FullConfig& cfg);
void ApplyConfig(const FullConfig& cfg);
void LoadCurrentConfigFromGlobals(FullConfig& cfg);
void EnsureConfigDirectory();
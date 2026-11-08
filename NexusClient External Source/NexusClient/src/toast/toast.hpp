#pragma once
#include "../imgui/imgui.h"
#include <string>
#include <vector>

struct Toast {
    std::string message;
    float startTime;
    float duration;
    float progress;
    bool active;
    float slideOffset;
    float fadeAlpha;

    Toast(const char* msg, float dur = 5.0f)
        : message(msg), duration(dur), active(true), slideOffset(0.0f), fadeAlpha(1.0f) {
        startTime = static_cast<float>(ImGui::GetTime());
        progress = 0.0f;
    }

    void Update();
};

extern std::vector<Toast> g_toasts;
void RenderToasts();

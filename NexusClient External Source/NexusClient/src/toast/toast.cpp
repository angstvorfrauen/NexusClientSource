#include "toast.hpp"
#include "../menu/menu.hpp"
#include <algorithm>

std::vector<Toast> g_toasts;

void Toast::Update() {
    if (!active)
        return;

    float now = static_cast<float>(ImGui::GetTime());
    float elapsed = now - startTime;

    progress = std::min(elapsed / duration, 1.0f);

    const float slideInDuration = 0.35f;
    if (elapsed < slideInDuration) {
        float t = elapsed / slideInDuration;

        t = 1.0f - powf(1.0f - t, 3.0f);
        slideOffset = t;
    }
    else {
        slideOffset = 1.0f;
    }

    if (progress >= 1.0f) {
        float fadeTime = now - (startTime + duration);
        const float fadeDuration = 0.5f;
        if (fadeTime >= fadeDuration) {
            active = false;
        }
        else {
            fadeAlpha = 1.0f - (fadeTime / fadeDuration);
        }
    }
    else {
        fadeAlpha = 1.0f;
    }
}

void RenderToasts() {
    if (g_toasts.empty())
        return;

    ImGuiIO& io = ImGui::GetIO();
    ImDrawList* draw = ImGui::GetForegroundDrawList();

    for (auto& toast : g_toasts)
        toast.Update();

    g_toasts.erase(
        std::remove_if(
            g_toasts.begin(),
            g_toasts.end(),
            [](const Toast& toast) {
                return !toast.active;
            }
        ),
        g_toasts.end()
    );

    if (g_toasts.empty())
        return;

    float screenW = io.DisplaySize.x;
    float screenH = io.DisplaySize.y;

    const float scale = std::clamp(screenH / 1080.0f, 0.85f, 1.25f);

    const float toastWidth = 300.0f * scale;
    const float toastHeight = 70.0f * scale;
    const float padding = 18.0f * scale;
    const float spacing = 10.0f * scale;
    const float rounding = 10.0f * scale;

    float yOffset = 0.0f;

    for (auto it = g_toasts.rbegin(); it != g_toasts.rend(); ++it) {
        Toast& toast = *it;

        if (!toast.active)
            continue;

        float targetX = screenW - toastWidth - padding;
        float startX = screenW + 20.0f * scale;

        float currentX =
            startX + (targetX - startX) * toast.slideOffset;

        float y =
            screenH -
            toastHeight -
            padding -
            yOffset;

        float alpha = toast.fadeAlpha;

        ImU32 bgColor = ImGui::GetColorU32(
            ImVec4(0.018f, 0.018f, 0.022f, 0.96f * alpha)
        );

        ImU32 borderColor = ImGui::GetColorU32(
            ImVec4(0.12f, 0.12f, 0.15f, 0.85f * alpha)
        );

        ImU32 textColor = ImGui::GetColorU32(
            ImVec4(0.92f, 0.92f, 0.95f, alpha)
        );

        ImVec2 min(currentX, y);
        ImVec2 max(
            currentX + toastWidth,
            y + toastHeight
        );

        draw->AddRectFilled(min, max, bgColor, rounding);
        draw->AddRect(min, max, borderColor, rounding, 0, 1.0f * scale);

        float textX = currentX + 16.0f * scale;
        float textY = y + 16.0f * scale;

        draw->AddText(
            ImVec2(textX, textY),
            textColor,
            toast.message.c_str()
        );

        float barX = currentX + 14.0f * scale;
        float barY = y + toastHeight - 10.0f * scale;

        float barWidth = toastWidth - 28.0f * scale;
        float barHeight = 3.0f * scale;

        ImU32 barBackground = ImGui::GetColorU32(
            ImVec4(0.07f, 0.07f, 0.085f, 0.8f * alpha)
        );

        draw->AddRectFilled(
            ImVec2(barX, barY),
            ImVec2(barX + barWidth, barY + barHeight),
            barBackground,
            2.0f * scale
        );

        float remaining = 1.0f - toast.progress;
        ImU32 accentColor = ImGui::GetColorU32(
            ImVec4(g_accentColor[0], g_accentColor[1], g_accentColor[2], alpha)
        );

        draw->AddRectFilled(
            ImVec2(barX, barY),
            ImVec2(barX + barWidth * remaining, barY + barHeight),
            accentColor,
            2.0f * scale
        );

        yOffset += toastHeight + spacing;
    }
}

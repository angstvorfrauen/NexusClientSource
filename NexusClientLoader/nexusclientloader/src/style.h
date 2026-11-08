#pragma once

#include <windows.h>

void InitTheme();
void CleanupTheme();

void CenterWindowOnScreen(
    HWND hwnd,
    int width,
    int height
);

void ApplyRoundedChildRegion(
    HWND hwnd,
    int width,
    int height,
    int radius
);

void CloseWindowAnimated(HWND hwnd);
void DrawCleanGradientGlowLine(HDC hdc, const RECT& rc, double phase);

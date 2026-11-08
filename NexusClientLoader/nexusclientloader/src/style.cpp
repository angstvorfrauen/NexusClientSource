#include <cmath>
#include "common.h"

namespace
{
    HFONT CreateUiFont(const wchar_t* family, LONG height, LONG weight)
    {
        return CreateFontW(
            height, 0, 0, 0, weight,
            FALSE, FALSE, FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            family);
    }

    HFONT CreateFallbackFont(LONG height, LONG weight)
    {
        return CreateUiFont(L"Segoe UI", height, weight);
    }
}

void InitTheme()
{
    g_fontRegular = CreateUiFont(L"Segoe UI Variable Text", -16, FW_NORMAL);
    g_fontBold = CreateUiFont(L"Segoe UI Variable Text", -16, FW_SEMIBOLD);
    g_fontTitle = CreateUiFont(L"Segoe UI Variable Display", -25, FW_SEMIBOLD);
    g_fontSmall = CreateUiFont(L"Segoe UI Variable Text", -13, FW_NORMAL);

    if (!g_fontRegular) g_fontRegular = CreateFallbackFont(-16, FW_NORMAL);
    if (!g_fontBold)    g_fontBold = CreateFallbackFont(-16, FW_SEMIBOLD);
    if (!g_fontTitle)   g_fontTitle = CreateFallbackFont(-25, FW_SEMIBOLD);
    if (!g_fontSmall)   g_fontSmall = CreateFallbackFont(-13, FW_NORMAL);

    g_bgBrush = CreateSolidBrush(g_bgColor);
    g_editBrush = CreateSolidBrush(g_editBgColor);
}

void CleanupTheme()
{
    if (g_fontRegular) { DeleteObject(g_fontRegular); g_fontRegular = nullptr; }
    if (g_fontBold) { DeleteObject(g_fontBold);    g_fontBold = nullptr; }
    if (g_fontTitle) { DeleteObject(g_fontTitle);   g_fontTitle = nullptr; }
    if (g_fontSmall) { DeleteObject(g_fontSmall);   g_fontSmall = nullptr; }
    if (g_bgBrush) { DeleteObject(g_bgBrush);     g_bgBrush = nullptr; }
    if (g_editBrush) { DeleteObject(g_editBrush);   g_editBrush = nullptr; }
}

void CenterWindowOnScreen(HWND hwnd, int width, int height)
{
    if (!hwnd) return;

    const int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    SetWindowPos(
        hwnd, nullptr,
        (screenWidth - width) / 2,
        (screenHeight - height) / 2,
        width, height,
        SWP_NOZORDER | SWP_NOACTIVATE);
}

void ApplyDarkTitleBar(HWND hwnd)
{
    if (!hwnd) return;

    HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
    if (!dwm) return;

    auto fn = reinterpret_cast<DwmSetWindowAttributeFn>(
        GetProcAddress(dwm, "DwmSetWindowAttribute"));

    if (fn)
    {
        BOOL dark = TRUE;
        fn(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &dark, sizeof(dark));

        COLORREF caption = g_bgColor;
        COLORREF text = g_textColor;
        fn(hwnd, DWMWA_CAPTION_COLOR, &caption, sizeof(caption));
        fn(hwnd, DWMWA_TEXT_COLOR, &text, sizeof(text));
    }

    FreeLibrary(dwm);
}

void ApplyRoundedWindowCorners(HWND hwnd)
{
    if (!hwnd) return;

    HMODULE dwm = LoadLibraryW(L"dwmapi.dll");
    if (!dwm) return;

    auto fn = reinterpret_cast<DwmSetWindowAttributeFn>(
        GetProcAddress(dwm, "DwmSetWindowAttribute"));

    if (fn)
    {
        DWORD preference = DWMWCP_ROUND;
        fn(hwnd, DWMWA_WINDOW_CORNER_PREFERENCE,
            &preference, sizeof(preference));
    }

    FreeLibrary(dwm);
}

void ApplyRoundedChildRegion(HWND hwnd, int width, int height, int radius)
{
    if (!hwnd || width <= 0 || height <= 0) return;

    HRGN region = CreateRoundRectRgn(
        0, 0, width + 1, height + 1, radius, radius);

    if (!region) return;

    if (SetWindowRgn(hwnd, region, TRUE) == 0)
        DeleteObject(region);
}

void CloseWindowAnimated(HWND hwnd)
{
    if (!hwnd || !IsWindow(hwnd)) return;

    AnimateWindow(hwnd, 140, AW_HIDE | AW_BLEND);
    DestroyWindow(hwnd);
}


void DrawCleanGradientGlowLine(HDC hdc, const RECT& rc, double phase)
{
    if (!hdc || rc.right <= rc.left)
        return;

    const int y = rc.top;
    const int left = rc.left;
    const int right = rc.right;
    const int width = right - left;

    Gdiplus::Graphics graphics(hdc);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics.SetCompositingMode(Gdiplus::CompositingModeSourceOver);

    const int segments = 96;

    auto hsv = [](double h, double s, double v) -> Gdiplus::Color
    {
        h = h - floor(h);
        double r = 0.0, g = 0.0, b = 0.0;
        double i = floor(h * 6.0);
        double f = h * 6.0 - i;
        double p = v * (1.0 - s);
        double q = v * (1.0 - f * s);
        double t = v * (1.0 - (1.0 - f) * s);

        switch (static_cast<int>(i) % 6)
        {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        default:r = v; g = p; b = q; break;
        }

        return Gdiplus::Color(
            255,
            static_cast<BYTE>(r * 255.0),
            static_cast<BYTE>(g * 255.0),
            static_cast<BYTE>(b * 255.0));
    };
    for (int pass = 0; pass < 4; ++pass)
    {
        const BYTE alpha = static_cast<BYTE>(10 - pass * 2);
        const REAL thickness = static_cast<REAL>(6.0 - pass * 1.25);

        for (int i = 0; i < segments; ++i)
        {
            const double a = static_cast<double>(i) / segments;
            const double b = static_cast<double>(i + 1) / segments;
            const double hueA = phase + a * 0.72;
            const double hueB = phase + b * 0.72;

            Gdiplus::Color ca = hsv(hueA, 0.62, 0.72);
            Gdiplus::Color cb = hsv(hueB, 0.62, 0.72);

            ca = Gdiplus::Color(alpha, ca.GetRed(), ca.GetGreen(), ca.GetBlue());
            cb = Gdiplus::Color(alpha, cb.GetRed(), cb.GetGreen(), cb.GetBlue());

            Gdiplus::LinearGradientBrush brush(
                Gdiplus::Point(left + static_cast<int>(width * a), y),
                Gdiplus::Point(left + static_cast<int>(width * b) + 1, y),
                ca, cb);

            Gdiplus::Pen pen(&brush, thickness);
            graphics.DrawLine(
                &pen,
                static_cast<REAL>(left + width * a),
                static_cast<REAL>(y),
                static_cast<REAL>(left + width * b),
                static_cast<REAL>(y));
        }
    }

    for (int i = 0; i < segments; ++i)
    {
        const double a = static_cast<double>(i) / segments;
        const double b = static_cast<double>(i + 1) / segments;

        Gdiplus::Color ca = hsv(phase + a * 0.72, 0.68, 0.88);
        Gdiplus::Color cb = hsv(phase + b * 0.72, 0.68, 0.88);

        Gdiplus::LinearGradientBrush brush(
            Gdiplus::Point(left + static_cast<int>(width * a), y),
            Gdiplus::Point(left + static_cast<int>(width * b) + 1, y),
            Gdiplus::Color(210, ca.GetRed(), ca.GetGreen(), ca.GetBlue()),
            Gdiplus::Color(210, cb.GetRed(), cb.GetGreen(), cb.GetBlue()));

        Gdiplus::Pen pen(&brush, 1.15f);
        graphics.DrawLine(
            &pen,
            static_cast<REAL>(left + width * a),
            static_cast<REAL>(y),
            static_cast<REAL>(left + width * b),
            static_cast<REAL>(y));
    }
}

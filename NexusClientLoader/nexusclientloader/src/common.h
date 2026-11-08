#pragma once

#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0A00
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <windowsx.h>
#include <winhttp.h>
#include <commctrl.h>
#include <shellscalingapi.h>
#include <commdlg.h>
#include <string>
#include <vector>
#include <random>
#include <thread>
#include <atomic>
#include <fstream>
#include <sstream>
#include <new>
#include <memory>
#include <cmath>
#include <gdiplus.h>
#include <wincrypt.h>
#include <objidl.h>
#include <winreg.h>

#include "strings.h"

using namespace Gdiplus;

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "shcore.lib")
#pragma comment(lib, "comdlg32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "advapi32.lib")

typedef HRESULT(WINAPI* DwmSetWindowAttributeFn)(HWND, DWORD, LPCVOID, DWORD);

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif
#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif
#ifndef DWMWA_TEXT_COLOR
#define DWMWA_TEXT_COLOR 36
#endif
#ifndef DWMWA_WINDOW_CORNER_PREFERENCE
#define DWMWA_WINDOW_CORNER_PREFERENCE 33
#endif
#ifndef DWMWCP_ROUND
#define DWMWCP_ROUND 2
#endif

inline constexpr UINT WM_APP_VERSION_READY = WM_APP + 1;
inline constexpr int WINDOW_WIDTH = 720;
inline constexpr int WINDOW_HEIGHT = 520;
inline constexpr int EDIT_ROUNDING = 10;
inline constexpr int BUTTON_ROUNDING = 10;
inline constexpr int EDIT_HEIGHT = 48;
inline constexpr int BUTTON_HEIGHT = 44;
inline constexpr int EDIT_PADDING_X = 14;

extern HWND g_loginWindow;
extern HWND g_usernameEdit;
extern HWND g_passwordEdit;
extern HWND g_loginButton;
extern HWND g_statusLabel;
extern HWND g_titleLabel;
extern HWND g_subtitleLabel;
extern HWND g_userLabel;
extern HWND g_passLabel;
extern HWND g_saveButton;
extern bool g_saveLogin;

extern HWND g_mainWindow;
extern HWND g_mainUsernameLabel;
extern HWND g_startButton;
extern HWND g_versionLabel;

extern std::wstring g_loggedInUsername;
extern std::wstring g_authToken;
extern std::wstring g_downloadedExePath;
extern std::wstring g_appVersion;
extern std::wstring g_subscriptionExpiresAt;
extern std::string g_profilePictureDataUrl;

extern std::unique_ptr<Gdiplus::Bitmap> g_profileBitmap;
extern ULONG_PTR g_gdiplusToken;

extern HFONT g_fontRegular;
extern HFONT g_fontBold;
extern HFONT g_fontTitle;
extern HFONT g_fontSmall;
extern HBRUSH g_bgBrush;
extern HBRUSH g_editBrush;

extern COLORREF g_bgColor;
extern COLORREF g_editBgColor;
extern COLORREF g_editBorder;
extern COLORREF g_textColor;
extern COLORREF g_subTextColor;
extern COLORREF g_labelColor;
extern COLORREF g_accentColor;
extern COLORREF g_accentHover;
extern COLORREF g_accentPress;
extern COLORREF g_accent2Color;
extern COLORREF g_dangerColor;
extern COLORREF g_disabledColor;
extern COLORREF g_statusColor;

extern bool g_loginButtonHover;
extern bool g_loginButtonPressed;
extern bool g_loginCloseHover;
extern bool g_startButtonHover;
extern bool g_startButtonPressed;
extern bool g_startupClosing;
extern bool g_restartingFromLogout;
extern bool g_profileMenuOpen;
extern bool g_profileMenuHoverWebsite;
extern bool g_profileMenuHoverLogout;

// Core
std::wstring GetMachineHwid();
std::wstring RandomTitleW(std::size_t length = 32);
void InitTheme();
void CleanupTheme();
void CenterWindowOnScreen(HWND hwnd, int width, int height);
void ApplyDarkTitleBar(HWND hwnd);
void ApplyRoundedWindowCorners(HWND hwnd);
void ApplyRoundedChildRegion(HWND hwnd, int width, int height, int radius);
void CloseWindowAnimated(HWND hwnd);
inline void AnimateLoginWindow(HWND hwnd, int durationMs = 180)
{
    if (!hwnd || !IsWindow(hwnd)) return;
    if (durationMs < 1) durationMs = 1;

    ShowWindow(hwnd, SW_SHOWNOACTIVATE);
    AnimateWindow(hwnd, durationMs, AW_BLEND);
    InvalidateRect(hwnd, nullptr, FALSE);
}

inline void AnimateWindowGrow(HWND hwnd, int targetWidth, int targetHeight, int durationMs = 260)
{
    if (!hwnd || !IsWindow(hwnd)) return;
    if (durationMs < 1) durationMs = 1;

    RECT target{};
    GetWindowRect(hwnd, &target);
    const int centerX = (target.left + target.right) / 2;
    const int centerY = (target.top + target.bottom) / 2;
    const int initialWidth = max(1, static_cast<int>(targetWidth * 0.82));
    const int initialHeight = max(1, static_cast<int>(targetHeight * 0.82));

    SetWindowPos(hwnd, nullptr, centerX - initialWidth / 2, centerY - initialHeight / 2,
        initialWidth, initialHeight, SWP_NOACTIVATE | SWP_NOZORDER);
    ShowWindow(hwnd, SW_SHOWNOACTIVATE);

    const ULONGLONG started = GetTickCount64();
    for (;;)
    {
        const double elapsed = static_cast<double>(GetTickCount64() - started);
        double t = elapsed / static_cast<double>(durationMs);
        if (t > 1.0) t = 1.0;

        // Smooth ease-out cubic.
        const double eased = 1.0 - std::pow(1.0 - t, 3.0);
        const int width = initialWidth + static_cast<int>((targetWidth - initialWidth) * eased);
        const int height = initialHeight + static_cast<int>((targetHeight - initialHeight) * eased);

        SetWindowPos(hwnd, nullptr, centerX - width / 2, centerY - height / 2,
            width, height, SWP_NOACTIVATE | SWP_NOZORDER);
        ApplyRoundedChildRegion(hwnd, width, height, 22);
        RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);

        if (t >= 1.0) break;
        Sleep(16);
    }

    SetWindowPos(hwnd, nullptr, centerX - targetWidth / 2, centerY - targetHeight / 2,
        targetWidth, targetHeight, SWP_NOACTIVATE | SWP_NOZORDER);
    ApplyRoundedChildRegion(hwnd, targetWidth, targetHeight, 22);
    RedrawWindow(hwnd, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN);
}

// String / HTTP helpers
std::string WideToUtf8(const std::wstring& text);
std::wstring Utf8ToWide(const std::string& text);
std::wstring EscapeJson(const std::wstring& text);
std::string GetJsonString(const std::string& json, const std::string& key);
std::wstring GetText(HWND hwnd);
bool QueryHttpStatus(HINTERNET request, DWORD& statusCode);
void CloseWinHttp(HINTERNET request, HINTERNET connection, HINTERNET session);

// Download / version
bool IsValidWindowsPE(const std::wstring& path);
bool ReadHttpResponse(const wchar_t* userAgent, const wchar_t* apiPath, std::string& response);
std::wstring GetAppDataFolder();
bool DownloadFile(const std::wstring& localPath);
std::wstring NormalizeVersion(std::wstring version);
std::wstring FetchVersionFromServer();

// Profile
bool Base64ToBytes(const std::string& input, std::vector<BYTE>& output);
void ClearProfileBitmap();
bool LoadProfileBitmap(const std::string& dataUrl);
void DrawProfileAvatar(HDC hdc, const RECT& bounds);
inline std::wstring FormatSubscriptionRemaining(const std::wstring& expiresAt)
{
    if (expiresAt.empty()) return L"No active subscription";
    if (expiresAt.rfind(L"9999-12-31", 0) == 0) return L"Subscription: Lifetime";

    SYSTEMTIME st{};
    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
    if (swscanf_s(expiresAt.c_str(), L"%4d-%2d-%2dT%2d:%2d:%2d",
        &year, &month, &day, &hour, &minute, &second) < 3)
        return L"Subscription active";

    st.wYear = static_cast<WORD>(year);
    st.wMonth = static_cast<WORD>(month);
    st.wDay = static_cast<WORD>(day);
    st.wHour = static_cast<WORD>(hour);
    st.wMinute = static_cast<WORD>(minute);
    st.wSecond = static_cast<WORD>(second);

    FILETIME ft{};
    if (!SystemTimeToFileTime(&st, &ft)) return L"Subscription active";

    ULARGE_INTEGER end{};
    end.LowPart = ft.dwLowDateTime;
    end.HighPart = ft.dwHighDateTime;

    FILETIME nowFt{};
    GetSystemTimeAsFileTime(&nowFt);
    ULARGE_INTEGER now{};
    now.LowPart = nowFt.dwLowDateTime;
    now.HighPart = nowFt.dwHighDateTime;

    if (end.QuadPart <= now.QuadPart) return L"Subscription expired";

    const unsigned long long seconds = (end.QuadPart - now.QuadPart) / 10000000ULL;
    const unsigned long long days = seconds / 86400ULL;
    const unsigned long long hours = (seconds % 86400ULL) / 3600ULL;

    wchar_t text[96]{};
    if (days >= 1)
    {
        swprintf_s(text, L"Subscription: %llu %s left", days, days == 1 ? L"day" : L"days");
    }
    else
    {
        swprintf_s(text, L"Subscription: %llu %s left", hours, hours == 1 ? L"hour" : L"hours");
    }
    return text;
}

// Auth
std::wstring GetSessionFilePath();
bool ProtectToken(const std::wstring& token, std::vector<BYTE>& protectedData);
bool UnprotectToken(const std::vector<BYTE>& protectedData, std::wstring& token);
bool SaveAuthToken();
bool LoadAuthToken();
void ClearAuthToken();

struct LoaderSessionResult {
    bool networkError = false;
    bool success = false;
    std::string response;
    DWORD lastError = 0;
};

LoaderSessionResult LoaderLoginAPI(const std::wstring& username,
    const std::wstring& password,
    const std::wstring& hwid);
bool ValidateSavedSession();

struct LoginResult {
    bool networkError = false;
    bool success = false;
    std::string response;
    std::wstring username;
    std::wstring token;
    std::string profilePicture;
    std::wstring subscriptionExpiresAt;
    DWORD lastError = 0;
};

LoginResult LoginAPI(const std::wstring& username, const std::wstring& password);

// Login UI
void DrawLoginChrome(HDC hdc, const RECT& rc, bool hover);
void DrawCleanGradientGlowLine(HDC hdc, const RECT& rc, double phase);
bool IsLoginClosePoint(const RECT& rc, POINT pt);
void SetStatus(const wchar_t* text, COLORREF color = RGB(137, 148, 170));
void CenterEditText(HWND edit);
void PaintEditBorder(HWND hwnd);
LRESULT CALLBACK EditSubclassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam,
    UINT_PTR subclassId, DWORD_PTR refData);
void PaintLoginButton(HWND hwnd, HDC hdc);
void PaintSaveButton(HWND hwnd, HDC hdc);
LRESULT CALLBACK SaveButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
bool RegisterSaveButtonClass(HINSTANCE instance);
LRESULT CALLBACK LoginButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
bool RegisterLoginButtonClass(HINSTANCE instance);
void PerformLogin();
LRESULT CALLBACK LoginWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

// Main UI
void PaintStartButton(HWND hwnd, HDC hdc);
LRESULT CALLBACK StartButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
bool RegisterStartButtonClass(HINSTANCE instance);
void DrawMainGrid(HDC hdc, const RECT& rc);
void DrawPanel(HDC hdc, RECT rc);
LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void OpenMainWindow();
void RunStartupCheck(HWND startupWindow);

// Window creation
bool CreateLoginWindow(HINSTANCE instance);

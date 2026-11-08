#include "common.h"
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")

static bool g_externalSelected = true;
static bool g_internalSelected = false;
static bool g_mainCloseHover = false;
static bool g_externalLoading = true;
static int g_spinnerFrame = 0;

static bool IsLifetimeUser()
{
    return _wcsicmp(g_loggedInUsername.c_str(), L"YOUR_USERNAME") == 0;
}

static bool IsSubscriptionActive()
{
    if (IsLifetimeUser()) return true;
    if (g_subscriptionExpiresAt.empty())
        return false;

    int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
    if (swscanf_s(
        g_subscriptionExpiresAt.c_str(),
        L"%4d-%2d-%2dT%2d:%2d:%2d",
        &year, &month, &day, &hour, &minute, &second) < 3)
        return false;

    SYSTEMTIME st{};
    st.wYear = static_cast<WORD>(year);
    st.wMonth = static_cast<WORD>(month);
    st.wDay = static_cast<WORD>(day);
    st.wHour = static_cast<WORD>(hour);
    st.wMinute = static_cast<WORD>(minute);
    st.wSecond = static_cast<WORD>(second);

    FILETIME expiryFt{};
    if (!SystemTimeToFileTime(&st, &expiryFt))
        return false;

    FILETIME nowFt{};
    GetSystemTimeAsFileTime(&nowFt);

    ULARGE_INTEGER expiry{}, now{};
    expiry.LowPart = expiryFt.dwLowDateTime;
    expiry.HighPart = expiryFt.dwHighDateTime;
    now.LowPart = nowFt.dwLowDateTime;
    now.HighPart = nowFt.dwHighDateTime;

    return expiry.QuadPart > now.QuadPart;
}

static RECT ProfileAvatarRect(const RECT& rc)
{
    return RECT{ 28, rc.bottom - 64, 68, rc.bottom - 24 };
}

static RECT ProfileMenuRect(const RECT& rc)
{
    return RECT{ 20, rc.bottom - 156, 218, rc.bottom - 72 };
}

static RECT ProfileWebsiteRect(const RECT& rc)
{
    RECT menu = ProfileMenuRect(rc);
    return RECT{ menu.left + 10, menu.top + 10, menu.right - 10, menu.top + 46 };
}

static RECT ProfileLogoutRect(const RECT& rc)
{
    RECT menu = ProfileMenuRect(rc);
    return RECT{ menu.left + 10, menu.top + 48, menu.right - 10, menu.bottom - 10 };
}

static void DrawRoundedFill(HDC hdc, const RECT& rc, COLORREF color, int radius = 12);
static void DrawRoundedBorder(HDC hdc, const RECT& rc, COLORREF color, int width = 1, int radius = 12);

static void DrawProfileMenu(HDC hdc, const RECT& rc)
{
    if (!g_profileMenuOpen) return;

    RECT menu = ProfileMenuRect(rc);
    DrawRoundedFill(hdc, menu, RGB(30, 30, 35), 12);
    DrawRoundedBorder(hdc, menu, RGB(58, 58, 67), 1, 12);

    RECT website = ProfileWebsiteRect(rc);
    RECT logout = ProfileLogoutRect(rc);

    if (g_profileMenuHoverWebsite)
        DrawRoundedFill(hdc, website, RGB(43, 43, 49), 8);

    if (g_profileMenuHoverLogout)
        DrawRoundedFill(hdc, logout, RGB(43, 43, 49), 8);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(232, 232, 236));

    HFONT old = reinterpret_cast<HFONT>(SelectObject(hdc, g_fontRegular));
    RECT text = website;
    text.left += 12;
    DrawTextW(hdc, L"Website", -1, &text,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    text = logout;
    text.left += 12;
    DrawTextW(hdc, L"Logout", -1, &text,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, old);
}

static void RestartApplication()
{
    wchar_t modulePath[MAX_PATH]{};
    const DWORD length = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    if (length == 0 || length >= MAX_PATH)
        return;

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};
    std::wstring commandLine = L"\"" + std::wstring(modulePath) + L"\"";

    if (!CreateProcessW(
        nullptr, commandLine.data(), nullptr, nullptr, FALSE, 0,
        nullptr, nullptr, &si, &pi))
    {
        return;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    if (g_mainWindow && IsWindow(g_mainWindow))
        DestroyWindow(g_mainWindow);

    if (g_loginWindow && IsWindow(g_loginWindow))
        DestroyWindow(g_loginWindow);

    PostQuitMessage(0);
}

static void LogoutToLogin(HWND mainWindow)
{
    ClearAuthToken();
    g_loggedInUsername.clear();
    g_subscriptionExpiresAt.clear();
    g_profilePictureDataUrl.clear();
    ClearProfileBitmap();
    g_profileMenuOpen = false;
    g_profileMenuHoverWebsite = false;
    g_profileMenuHoverLogout = false;
    g_saveLogin = false;

    RestartApplication();
}
static bool IsMainClosePoint(const RECT& rc, POINT pt)
{
    RECT close{ rc.right - 60, 0, rc.right, 44 };
    return PtInRect(&close, pt) != FALSE;
}

static RECT ExternalCardRect(const RECT& rc)
{
    return RECT{ 28, 142, rc.right - 28, 252 };
}

static RECT InternalCardRect(const RECT& rc)
{
    return RECT{ 28, 266, rc.right - 28, 376 };
}

static void DrawRoundedFill(HDC hdc, const RECT& rc, COLORREF color, int radius)
{
    HBRUSH brush = CreateSolidBrush(color);
    HRGN region = CreateRoundRectRgn(rc.left, rc.top, rc.right + 1, rc.bottom + 1, radius, radius);
    FillRgn(hdc, region, brush);
    DeleteObject(region);
    DeleteObject(brush);
}

static void DrawRoundedBorder(HDC hdc, const RECT& rc, COLORREF color, int width, int radius)
{
    HPEN pen = CreatePen(PS_SOLID, width, color);
    HPEN oldPen = reinterpret_cast<HPEN>(SelectObject(hdc, pen));
    HBRUSH oldBrush = reinterpret_cast<HBRUSH>(SelectObject(hdc, GetStockObject(NULL_BRUSH)));
    RoundRect(hdc, rc.left, rc.top, rc.right, rc.bottom, radius, radius);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

static void DrawTopBar(HDC hdc, const RECT& rc, bool hover)
{
    RECT bar{ 0, 0, rc.right, 44 };
    DrawRoundedFill(hdc, bar, RGB(27, 27, 31), 18);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(235, 235, 238));
    HFONT old = reinterpret_cast<HFONT>(SelectObject(hdc, g_fontBold));
    RECT title{ 20, 0, 220, 44 };
    DrawTextW(hdc, L"NexusClient", -1, &title, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, old);
    RECT close{ rc.right - 60, 0, rc.right, 44 };

    HPEN pen = CreatePen(PS_SOLID, hover ? 3 : 2,
        hover ? RGB(255, 105, 105) : RGB(205, 205, 210));
    HPEN oldPen = reinterpret_cast<HPEN>(SelectObject(hdc, pen));
    const int cx = rc.right - 28;
    MoveToEx(hdc, cx - 6, 15, nullptr); LineTo(hdc, cx + 6, 27);
    MoveToEx(hdc, cx + 6, 15, nullptr); LineTo(hdc, cx - 6, 27);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

static void DrawModeCard(HDC hdc, const RECT& rc, bool selected, bool available,
    const wchar_t* title, const wchar_t* subtitle)
{
    const COLORREF fill = selected ? RGB(35, 35, 40) : RGB(29, 29, 33);
    const COLORREF border = selected ? RGB(76, 76, 86) : RGB(48, 48, 55);
    DrawRoundedFill(hdc, rc, fill, 14);
    DrawRoundedBorder(hdc, rc, border, selected ? 1 : 1, 14);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(235, 235, 239));
    HFONT old = reinterpret_cast<HFONT>(SelectObject(hdc, g_fontBold));
    RECT titleRc{ rc.left + 18, rc.top + 16, rc.right - 90, rc.top + 45 };
    DrawTextW(hdc, title, -1, &titleRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, old);

    SetTextColor(hdc, available ? RGB(135, 135, 145) : RGB(110, 110, 118));
    old = reinterpret_cast<HFONT>(SelectObject(hdc, g_fontSmall));
    RECT subRc{ rc.left + 18, rc.top + 48, rc.right - 100, rc.top + 76 };
    DrawTextW(hdc, subtitle, -1, &subRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, old);

    if (!available)
    {
        SetTextColor(hdc, RGB(112, 112, 120));
        old = reinterpret_cast<HFONT>(SelectObject(hdc, g_fontSmall));
        RECT unavailable{ rc.right - 112, rc.top + 34, rc.right - 18, rc.top + 72 };
        DrawTextW(hdc, L"Not Available", -1, &unavailable, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, old);
    }
    else if (selected)
    {
        const int cx = rc.right - 34;
        const int cy = rc.top + 55;
        if (g_externalLoading)
        {
            Graphics graphics(hdc);
            graphics.SetSmoothingMode(SmoothingModeAntiAlias);
            Pen spinnerPen(Color(255, 145, 125, 255), 2.0f);
            const float startAngle = static_cast<float>(g_spinnerFrame * 20);
            graphics.DrawArc(&spinnerPen,
                static_cast<REAL>(cx - 8), static_cast<REAL>(cy - 8),
                16.0f, 16.0f, startAngle, 250.0f);
        }
        else
        {
            HPEN checkPen = CreatePen(PS_SOLID, 2, g_accentColor);
            HPEN oldCheckPen = reinterpret_cast<HPEN>(SelectObject(hdc, checkPen));
            MoveToEx(hdc, cx - 8, cy, nullptr);
            LineTo(hdc, cx - 2, cy + 6);
            LineTo(hdc, cx + 9, cy - 7);
            SelectObject(hdc, oldCheckPen);
            DeleteObject(checkPen);
        }
    }
}

void PaintStartButton(HWND hwnd, HDC hdc)
{
    RECT rc{};
    GetClientRect(hwnd, &rc);

    COLORREF fill = RGB(38, 38, 44);
    if (!IsWindowEnabled(hwnd)) fill = RGB(30, 30, 34);
    else if (g_startButtonPressed) fill = RGB(49, 49, 56);
    else if (g_startButtonHover) fill = RGB(44, 44, 51);
    HBRUSH baseBrush = CreateSolidBrush(RGB(22, 22, 25));
    FillRect(hdc, &rc, baseBrush);
    DeleteObject(baseBrush);

    DrawRoundedFill(hdc, rc, fill, 11);
    DrawRoundedBorder(hdc, rc, RGB(59, 59, 67), 1, 11);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, IsWindowEnabled(hwnd) ? RGB(235, 235, 239) : RGB(105, 105, 112));
    HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(hdc, g_fontBold));
    const wchar_t* buttonText = IsSubscriptionActive()
        ? START_BUTTON_TEXT
        : L"SUBSCRIPTION REQUIRED";
    DrawTextW(hdc, buttonText, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(hdc, oldFont);
}

LRESULT CALLBACK StartButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        PaintStartButton(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND: return 1;
    case WM_MOUSEMOVE:
        if (!g_startButtonHover) { g_startButtonHover = true; InvalidateRect(hwnd, nullptr, FALSE); }
        { TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, hwnd, 0 }; TrackMouseEvent(&tme); }
        return 0;
    case WM_MOUSELEAVE:
        g_startButtonHover = false; g_startButtonPressed = false; InvalidateRect(hwnd, nullptr, FALSE); return 0;
    case WM_LBUTTONDOWN:
        if (IsWindowEnabled(hwnd)) { g_startButtonPressed = true; SetCapture(hwnd); InvalidateRect(hwnd, nullptr, FALSE); }
        return 0;
    case WM_LBUTTONUP:
        if (g_startButtonPressed)
        {
            g_startButtonPressed = false;
            if (GetCapture() == hwnd) ReleaseCapture();
            InvalidateRect(hwnd, nullptr, FALSE);
            POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            RECT rc{}; GetClientRect(hwnd, &rc);
            if (PtInRect(&rc, pt))
                SendMessageW(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(1004, BN_CLICKED), reinterpret_cast<LPARAM>(hwnd));
        }
        return 0;
    case WM_ENABLE:
        g_startButtonPressed = false; InvalidateRect(hwnd, nullptr, TRUE); return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

bool RegisterStartButtonClass(HINSTANCE instance)
{
    static bool registered = false;
    if (registered) return true;
    WNDCLASSW wc{};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = StartButtonProc;
    wc.hInstance = instance;
    wc.lpszClassName = L"NexusStartButton";
    wc.hCursor = LoadCursorW(nullptr, IDC_HAND);
    if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) return false;
    registered = true;
    return true;
}

void DrawMainGrid(HDC hdc, const RECT& rc)
{
    HBRUSH bg = CreateSolidBrush(RGB(22, 22, 25));
    FillRect(hdc, &rc, bg);
    DeleteObject(bg);
}

void DrawPanel(HDC hdc, RECT rc)
{
    DrawRoundedFill(hdc, rc, RGB(28, 28, 32), 14);
    DrawRoundedBorder(hdc, rc, RGB(47, 47, 54), 1, 14);
}

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_ERASEBKGND: return 1;
    case WM_NCHITTEST:
    {
        LRESULT hit = DefWindowProcW(hwnd, message, wParam, lParam);
        if (hit == HTCLIENT)
        {
            POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &pt);
            RECT rc{}; GetClientRect(hwnd, &rc);
            if (pt.y < 44 && !IsMainClosePoint(rc, pt)) return HTCAPTION;
        }
        return hit;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC paintDc = BeginPaint(hwnd, &ps);
        RECT rc{}; GetClientRect(hwnd, &rc);

        HDC hdc = CreateCompatibleDC(paintDc);
        HBITMAP bitmap = CreateCompatibleBitmap(paintDc, rc.right, rc.bottom);
        HBITMAP oldBitmap = reinterpret_cast<HBITMAP>(SelectObject(hdc, bitmap));

        DrawMainGrid(hdc, rc);
        DrawTopBar(hdc, rc, g_mainCloseHover);

        RECT glowLine{ 0, 44, rc.right, 45 };
        const double glowPhase = static_cast<double>(GetTickCount64() % 12000ULL) / 12000.0;
        DrawCleanGradientGlowLine(hdc, glowLine, glowPhase);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(238, 238, 241));
        HFONT old = reinterpret_cast<HFONT>(SelectObject(hdc, g_fontTitle));
        RECT heading{ 28, 66, rc.right - 28, 98 };
        DrawTextW(hdc, L"Select your Version", -1, &heading, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, old);

        SetTextColor(hdc, RGB(132, 132, 141));
        old = reinterpret_cast<HFONT>(SelectObject(hdc, g_fontSmall));
        RECT sub{ 29, 98, rc.right - 28, 124 };
        DrawTextW(hdc, L"Select the version you wanna use for CS2", -1, &sub, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, old);

        RECT external = ExternalCardRect(rc);
        RECT internal = InternalCardRect(rc);
        DrawModeCard(hdc, external, g_externalSelected, true, L"CS2 External", L"For legit players just a overlay on the screen");
        DrawModeCard(hdc, internal, g_internalSelected, false, L"CS2 Internal", L"Reading client.dll and overrite it good to rage");

        RECT avatar{ 28, rc.bottom - 60, 68, rc.bottom - 20 };
        DrawProfileAvatar(hdc, avatar);

        SetTextColor(hdc, RGB(235, 235, 239));
        old = reinterpret_cast<HFONT>(SelectObject(hdc, g_fontBold));
        RECT profileName{ 80, rc.bottom - 60, 300, rc.bottom - 37 };
        DrawTextW(hdc, g_loggedInUsername.c_str(), -1, &profileName,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(hdc, old);

        SetTextColor(hdc, RGB(132, 132, 141));
        old = reinterpret_cast<HFONT>(SelectObject(hdc, g_fontSmall));
        std::wstring subscriptionText = FormatSubscriptionRemaining(g_subscriptionExpiresAt);
        RECT profileSub{ 80, rc.bottom - 38, 330, rc.bottom - 18 };
        DrawTextW(hdc, subscriptionText.c_str(), -1, &profileSub,
            DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        SelectObject(hdc, old);

        DrawProfileMenu(hdc, rc);

        BitBlt(paintDc, 0, 0, rc.right, rc.bottom, hdc, 0, 0, SRCCOPY);
        SelectObject(hdc, oldBitmap);
        DeleteObject(bitmap);
        DeleteDC(hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_MOUSEMOVE:
    {
        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RECT rc{}; GetClientRect(hwnd, &rc);
        bool hover = IsMainClosePoint(rc, pt);

        RECT websiteRect = ProfileWebsiteRect(rc);
        RECT logoutRect = ProfileLogoutRect(rc);

        bool websiteHover = g_profileMenuOpen && PtInRect(&websiteRect, pt) != FALSE;
        bool logoutHover = g_profileMenuOpen && PtInRect(&logoutRect, pt) != FALSE;

        if (hover != g_mainCloseHover ||
            websiteHover != g_profileMenuHoverWebsite ||
            logoutHover != g_profileMenuHoverLogout)
        {
            g_mainCloseHover = hover;
            g_profileMenuHoverWebsite = websiteHover;
            g_profileMenuHoverLogout = logoutHover;
            RECT redraw{ 0, 0, rc.right, 48 };
            InvalidateRect(hwnd, &redraw, FALSE);
            if (g_profileMenuOpen)
            {
                RECT menu = ProfileMenuRect(rc);
                InvalidateRect(hwnd, &menu, FALSE);
            }
        }

        TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tme);
        return 0;
    }
    case WM_MOUSELEAVE:
        g_mainCloseHover = false; InvalidateRect(hwnd, nullptr, FALSE); return 0;

    case WM_LBUTTONDOWN:
    {
        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RECT rc{}; GetClientRect(hwnd, &rc);
        if (IsMainClosePoint(rc, pt)) { SetCapture(hwnd); return 0; }
        break;
    }
    case WM_LBUTTONUP:
    {
        if (GetCapture() == hwnd) ReleaseCapture();
        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RECT rc{}; GetClientRect(hwnd, &rc);
        if (IsMainClosePoint(rc, pt)) { CloseWindowAnimated(hwnd); return 0; }

        RECT avatar = ProfileAvatarRect(rc);
        if (PtInRect(&avatar, pt))
        {
            g_profileMenuOpen = !g_profileMenuOpen;
            g_profileMenuHoverWebsite = false;
            g_profileMenuHoverLogout = false;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }

        if (g_profileMenuOpen)
        {
            RECT website = ProfileWebsiteRect(rc);
            RECT logout = ProfileLogoutRect(rc);
            RECT menu = ProfileMenuRect(rc);

            if (PtInRect(&website, pt))
            {
                ShellExecuteW(hwnd, L"open", L"https://xeuka.net", nullptr, nullptr, SW_SHOWNORMAL);
                g_profileMenuOpen = false;
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }

            if (PtInRect(&logout, pt))
            {
                LogoutToLogin(hwnd);
                return 0;
            }

            if (!PtInRect(&menu, pt))
            {
                g_profileMenuOpen = false;
                InvalidateRect(hwnd, nullptr, FALSE);
                return 0;
            }

            return 0;
        }

        RECT external = ExternalCardRect(rc);
        RECT internal = InternalCardRect(rc);
        if (PtInRect(&external, pt))
        {
            g_externalSelected = true;
            g_internalSelected = false;
            g_externalLoading = true;
            g_spinnerFrame = 0;
            EnableWindow(g_startButton, FALSE);
            SetTimer(hwnd, 2, 70, nullptr);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        if (PtInRect(&internal, pt))
        {
            g_externalSelected = false;
            g_internalSelected = true;
            g_externalLoading = false;
            KillTimer(hwnd, 2);
            EnableWindow(g_startButton, FALSE);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        return 0;
    }

    case WM_TIMER:
        if (wParam == 3)
        {
            RECT client{};
            GetClientRect(hwnd, &client);
            RECT glowRect{ 0, 42, client.right, 47 };
            InvalidateRect(hwnd, &glowRect, FALSE);
            return 0;
        }

        if (wParam == 2)
        {
            ++g_spinnerFrame;
            if (g_spinnerFrame >= 18)
            {
                g_externalLoading = false;
                KillTimer(hwnd, 2);
                if (g_externalSelected) EnableWindow(g_startButton, IsSubscriptionActive() ? TRUE : FALSE);
            }

            RECT spinnerRect{ WINDOW_WIDTH - 110, 142, WINDOW_WIDTH - 28, 252 };
            InvalidateRect(hwnd, &spinnerRect, FALSE);
            return 0;
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == 1004)
        {
            if (!g_externalSelected || g_externalLoading) return 0;

            if (!IsSubscriptionActive())
            {
                MessageBoxW(
                    hwnd,
                    L"You need an active subscription to start this build.",
                    L"NexusClient",
                    MB_OK | MB_ICONWARNING);
                EnableWindow(g_startButton, FALSE);
                return 0;
            }

            if (g_downloadedExePath.empty())
            {
                std::wstring folder = GetAppDataFolder();
                if (!folder.empty()) g_downloadedExePath = folder + L"\\" + EXE_FILENAME;
            }
            if (g_downloadedExePath.empty())
            {
                MessageBoxW(hwnd, L"AppData folder not found.", L"NexusClient", MB_OK | MB_ICONERROR);
                return 0;
            }

            EnableWindow(g_startButton, FALSE);
            DeleteFileW(g_downloadedExePath.c_str());
            DeleteFileW((g_downloadedExePath + L".download").c_str());

            if (!DownloadFile(g_downloadedExePath))
            {
                DeleteFileW(g_downloadedExePath.c_str());
                DeleteFileW((g_downloadedExePath + L".download").c_str());
                EnableWindow(g_startButton, TRUE);
                MessageBoxW(hwnd, L"Error while downloading.", L"NexusClient", MB_OK | MB_ICONERROR);
                return 0;
            }

            STARTUPINFOW si{}; si.cb = sizeof(si);
            PROCESS_INFORMATION pi{};
            if (!CreateProcessW(g_downloadedExePath.c_str(), nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi))
            {
                wchar_t messageText[256]{};
                swprintf_s(messageText, L"Error while starting: %lu", GetLastError());
                EnableWindow(g_startButton, TRUE);
                MessageBoxW(hwnd, messageText, L"NexusClient", MB_OK | MB_ICONERROR);
                return 0;
            }
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            EnableWindow(g_startButton, TRUE);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;

    case WM_CTLCOLORSTATIC:
    {
        HDC dc = reinterpret_cast<HDC>(wParam);
        SetTextColor(dc, g_textColor);
        SetBkColor(dc, g_bgColor);
        SetBkMode(dc, TRANSPARENT);
        return reinterpret_cast<INT_PTR>(g_bgBrush);
    }
    case WM_CLOSE: CloseWindowAnimated(hwnd); return 0;
    case WM_DESTROY:
        KillTimer(hwnd, 2);
        KillTimer(hwnd, 3);
        ClearProfileBitmap();
        if (g_restartingFromLogout)
            g_restartingFromLogout = false;
        else
            PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

void OpenMainWindow()
{
    const wchar_t CLASS_NAME[] = L"LoginAppMainWindow";
    HINSTANCE instance = GetModuleHandleW(nullptr);
    RegisterStartButtonClass(instance);

    WNDCLASSW wc{};
    wc.lpfnWndProc = MainWindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = g_bgBrush;
    RegisterClassW(&wc);

    g_mainWindow = CreateWindowExW(
        0, CLASS_NAME, L"NexusClient",
        WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
        nullptr, nullptr, instance, nullptr);
    if (!g_mainWindow) return;

    ApplyRoundedChildRegion(g_mainWindow, WINDOW_WIDTH, WINDOW_HEIGHT, 22);

    g_mainUsernameLabel = CreateWindowW(
        L"STATIC", L"", WS_CHILD | SS_LEFT | SS_CENTERIMAGE,
        28, WINDOW_HEIGHT - 34, 220, 22, g_mainWindow, nullptr, instance, nullptr);
    SendMessageW(g_mainUsernameLabel, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontSmall), TRUE);

    g_startButton = CreateWindowW(
        L"NexusStartButton", L"", WS_VISIBLE | WS_CHILD | WS_TABSTOP,
        WINDOW_WIDTH - 222, WINDOW_HEIGHT - 62, 194, 44,
        g_mainWindow, reinterpret_cast<HMENU>(1004), instance, nullptr);
    if (!g_startButton) return;
    ApplyRoundedChildRegion(g_startButton, 194, 44, 10);
    EnableWindow(g_startButton, FALSE);

    CenterWindowOnScreen(g_mainWindow, WINDOW_WIDTH, WINDOW_HEIGHT);
    SetTimer(g_mainWindow, 2, 70, nullptr);
    SetTimer(g_mainWindow, 3, 35, nullptr);
    AnimateWindowGrow(g_mainWindow, WINDOW_WIDTH, WINDOW_HEIGHT, 280);
    UpdateWindow(g_mainWindow);
    RedrawWindow(g_startButton, nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE);


}

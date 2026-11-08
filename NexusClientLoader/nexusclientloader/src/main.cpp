#include "common.h"

namespace
{
    constexpr UINT WM_APP_STARTUP_DONE = WM_APP + 20;
    constexpr int STARTUP_WIDTH = 420;
    constexpr int STARTUP_HEIGHT = 220;

    bool g_startupCloseHover = false;
    int g_startupFrame = 0;
    ULONGLONG g_startupStarted = 0;
    ULONGLONG g_sessionCheckedAt = 0;
    bool g_sessionCheckFinished = false;
    bool g_sessionValid = false;
    constexpr DWORD STARTUP_MIN_MS = 5000;
    constexpr int STARTUP_TARGET_WIDTH = 420;
    constexpr int STARTUP_TARGET_HEIGHT = 220;
    constexpr int STARTUP_INITIAL_WIDTH = 330;
    constexpr int STARTUP_INITIAL_HEIGHT = 170;

    bool IsStartupClosePoint(const RECT& rc, POINT pt)
    {
        RECT close{ rc.right - 52, 0, rc.right, 44 };
        return PtInRect(&close, pt) != FALSE;
    }

    void DrawStartup(HDC hdc, const RECT& rc)
    {
        HBRUSH bg = CreateSolidBrush(g_bgColor);
        FillRect(hdc, &rc, bg);
        DeleteObject(bg);

        RECT top{ 0, 0, rc.right, 44 };
        HBRUSH topBrush = CreateSolidBrush(RGB(27, 27, 31));
        HRGN topRegion = CreateRoundRectRgn(0, 0, rc.right + 1, 45, 18, 18);
        FillRgn(hdc, topRegion, topBrush);
        DeleteObject(topRegion);
        DeleteObject(topBrush);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, g_textColor);
        HFONT old = reinterpret_cast<HFONT>(SelectObject(hdc, g_fontBold));
        RECT title{ 20, 0, 250, 44 };
        DrawTextW(hdc, L"NexusClient", -1, &title, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, old);

        RECT close{ rc.right - 52, 0, rc.right, 44 };
        if (g_startupCloseHover)
        {
            HBRUSH hb = CreateSolidBrush(RGB(43, 43, 48));
            HRGN hr = CreateRoundRectRgn(close.left, close.top, close.right + 1, close.bottom + 1, 14, 14);
            FillRgn(hdc, hr, hb);
            DeleteObject(hr);
            DeleteObject(hb);
        }

        HPEN pen = CreatePen(PS_SOLID, 1, g_startupCloseHover ? RGB(240, 125, 125) : RGB(155, 155, 164));
        HPEN oldPen = reinterpret_cast<HPEN>(SelectObject(hdc, pen));
        const int cx = rc.right - 26;
        MoveToEx(hdc, cx - 4, 17, nullptr); LineTo(hdc, cx + 4, 25);
        MoveToEx(hdc, cx + 4, 17, nullptr); LineTo(hdc, cx - 4, 25);
        SelectObject(hdc, oldPen);
        DeleteObject(pen);

        SetTextColor(hdc, RGB(238, 238, 241));
        old = reinterpret_cast<HFONT>(SelectObject(hdc, g_fontTitle));
        RECT heading{ 28, 72, rc.right - 28, 105 };
        DrawTextW(hdc, L"Starting NexusClient", -1, &heading, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, old);

        SetTextColor(hdc, RGB(132, 132, 141));
        old = reinterpret_cast<HFONT>(SelectObject(hdc, g_fontSmall));
        RECT sub{ 29, 105, rc.right - 28, 130 };
        DrawTextW(hdc, L"Checking your session...", -1, &sub, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, old);

        RECT bar{ 28, 150, rc.right - 28, 158 };
        HBRUSH barBg = CreateSolidBrush(RGB(37, 37, 42));
        HRGN barRegion = CreateRoundRectRgn(bar.left, bar.top, bar.right + 1, bar.bottom + 1, 8, 8);
        FillRgn(hdc, barRegion, barBg);
        DeleteObject(barRegion);
        DeleteObject(barBg);

        const int segmentWidth = 90;
        const int innerLeft = bar.left + 1;
        const int innerRight = bar.right - 1;
        const int travel = (innerRight - innerLeft) - segmentWidth;
        const double phase = (static_cast<double>(g_startupFrame % 240) / 239.0) * 2.0 * 3.141592653589793;
        const double eased = (1.0 - std::cos(phase)) * 0.5;
        const int x = innerLeft + static_cast<int>(travel * eased);
        RECT segment{ x, bar.top + 1, x + segmentWidth, bar.bottom - 1 };
        HBRUSH segmentBrush = CreateSolidBrush(g_accentColor);
        HRGN clipRegion = CreateRoundRectRgn(bar.left, bar.top, bar.right + 1, bar.bottom + 1, 8, 8);
        SelectClipRgn(hdc, clipRegion);
        FillRect(hdc, &segment, segmentBrush);
        SelectClipRgn(hdc, nullptr);
        DeleteObject(clipRegion);
        DeleteObject(segmentBrush);

        RECT status{ 28, 178, rc.right - 28, 202 };
        SetTextColor(hdc, RGB(105, 105, 114));
        old = reinterpret_cast<HFONT>(SelectObject(hdc, g_fontSmall));
        DrawTextW(hdc, L"Please wait...", -1, &status, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, old);
    }

    LRESULT CALLBACK StartupWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_ERASEBKGND:
            return 1;

        case WM_NCHITTEST:
        {
            LRESULT hit = DefWindowProcW(hwnd, message, wParam, lParam);
            if (hit == HTCLIENT)
            {
                POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                ScreenToClient(hwnd, &pt);
                RECT rc{}; GetClientRect(hwnd, &rc);
                if (pt.y < 44 && !IsStartupClosePoint(rc, pt)) return HTCAPTION;
            }
            return hit;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps{};
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc{}; GetClientRect(hwnd, &rc);
            DrawStartup(hdc, rc);
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_MOUSEMOVE:
        {
            POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            RECT rc{}; GetClientRect(hwnd, &rc);
            bool hover = IsStartupClosePoint(rc, pt);
            if (hover != g_startupCloseHover)
            {
                g_startupCloseHover = hover;
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, hwnd, 0 };
            TrackMouseEvent(&tme);
            return 0;
        }

        case WM_MOUSELEAVE:
            g_startupCloseHover = false;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;

        case WM_LBUTTONDOWN:
        {
            POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            RECT rc{}; GetClientRect(hwnd, &rc);
            if (IsStartupClosePoint(rc, pt)) { SetCapture(hwnd); return 0; }
            break;
        }

        case WM_LBUTTONUP:
        {
            if (GetCapture() == hwnd) ReleaseCapture();
            POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            RECT rc{}; GetClientRect(hwnd, &rc);
            if (IsStartupClosePoint(rc, pt))
            {
                g_startupClosing = true;
                DestroyWindow(hwnd);
                return 0;
            }
            return 0;
        }

        case WM_TIMER:
            if (wParam == 1)
            {
                ++g_startupFrame;

                const ULONGLONG now = GetTickCount64();
                const double elapsed = static_cast<double>(now - g_startupStarted);
                const double growT = (elapsed < 900.0) ? (elapsed / 900.0) : 1.0;
                const double eased = 1.0 - std::pow(1.0 - growT, 3.0);

                const int width = STARTUP_INITIAL_WIDTH + static_cast<int>((STARTUP_TARGET_WIDTH - STARTUP_INITIAL_WIDTH) * eased);
                const int height = STARTUP_INITIAL_HEIGHT + static_cast<int>((STARTUP_TARGET_HEIGHT - STARTUP_INITIAL_HEIGHT) * eased);
                if (width != STARTUP_INITIAL_WIDTH || height != STARTUP_INITIAL_HEIGHT)
                {
                    RECT wr{};
                    GetWindowRect(hwnd, &wr);
                    const int cx = (wr.left + wr.right) / 2;
                    const int cy = (wr.top + wr.bottom) / 2;
                    SetWindowPos(hwnd, nullptr, cx - width / 2, cy - height / 2, width, height, SWP_NOACTIVATE | SWP_NOZORDER);
                    ApplyRoundedChildRegion(hwnd, width, height, 22);
                }

                InvalidateRect(hwnd, nullptr, FALSE);

                if (g_sessionCheckFinished && (now - g_startupStarted) >= STARTUP_MIN_MS)
                    PostMessageW(hwnd, WM_APP_STARTUP_DONE, g_sessionValid ? 1 : 0, 0);
                return 0;
            }
            break;

        case WM_APP_STARTUP_DONE:
        {
            const bool valid = wParam != 0;
            KillTimer(hwnd, 1);
            if (g_startupClosing) return 0;

            ShowWindow(hwnd, SW_HIDE);
            DestroyWindow(hwnd);

            if (valid)
            {
                g_downloadedExePath = GetAppDataFolder();
                if (!g_downloadedExePath.empty())
                    g_downloadedExePath += L"\\" + std::wstring(EXE_FILENAME);
                OpenMainWindow();
            }
            else
            {
                ClearAuthToken();
                if (g_loginWindow)
                {
                    ShowWindow(g_loginWindow, SW_SHOW);
                    AnimateLoginWindow(g_loginWindow, 180);
                    SetFocus(g_usernameEdit);
                }
            }
            return 0;
        }

        case WM_CLOSE:
            g_startupClosing = true;
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            KillTimer(hwnd, 1);
            if (g_startupClosing) PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    HWND CreateStartupWindow(HINSTANCE instance)
    {
        const wchar_t CLASS_NAME[] = L"NexusClientStartupWindow";
        WNDCLASSW wc{};
        wc.lpfnWndProc = StartupWindowProc;
        wc.hInstance = instance;
        wc.lpszClassName = CLASS_NAME;
        wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        wc.hbrBackground = g_bgBrush;
        wc.style = CS_HREDRAW | CS_VREDRAW;
        RegisterClassW(&wc);

        HWND hwnd = CreateWindowExW(
            WS_EX_COMPOSITED,
            CLASS_NAME, L"NexusClient",
            WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
            CW_USEDEFAULT, CW_USEDEFAULT,
            STARTUP_INITIAL_WIDTH, STARTUP_INITIAL_HEIGHT,
            nullptr, nullptr, instance, nullptr);
        if (!hwnd) return nullptr;

        ApplyRoundedChildRegion(hwnd, STARTUP_INITIAL_WIDTH, STARTUP_INITIAL_HEIGHT, 22);
        CenterWindowOnScreen(hwnd, STARTUP_INITIAL_WIDTH, STARTUP_INITIAL_HEIGHT);
        g_startupStarted = GetTickCount64();
        SetTimer(hwnd, 1, 16, nullptr);
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
        return hwnd;
    }
}

void RunStartupCheck(HWND startupWindow)
{
    std::thread([startupWindow]()
    {
        bool valid = false;
        if (LoadAuthToken())
            valid = ValidateSavedSession();

        g_sessionValid = valid;
        g_sessionCheckFinished = true;
        g_sessionCheckedAt = GetTickCount64();

        if (!IsWindow(startupWindow) || g_startupClosing)
            return;
        PostMessageW(startupWindow, WM_NULL, 0, 0);
    }).detach();
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int)
{
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    INITCOMMONCONTROLSEX icc{};
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icc);

    InitTheme();

    GdiplusStartupInput gdiplusInput;
    if (GdiplusStartup(&g_gdiplusToken, &gdiplusInput, nullptr) != Gdiplus::Ok)
    {
        CleanupTheme();
        return 1;
    }

    if (!CreateLoginWindow(hInstance))
    {
        MessageBoxW(nullptr, L"Could not create the login window.", L"NexusClient", MB_OK | MB_ICONERROR);
        GdiplusShutdown(g_gdiplusToken);
        CleanupTheme();
        return 1;
    }

    ShowWindow(g_loginWindow, SW_HIDE);

    HWND startupWindow = CreateStartupWindow(hInstance);
    if (!startupWindow)
    {
        ShowWindow(g_loginWindow, SW_SHOW);
        SetFocus(g_usernameEdit);
    }
    else
    {
        RunStartupCheck(startupWindow);
    }

    MSG msg{};
    while (GetMessageW(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    GdiplusShutdown(g_gdiplusToken);
    CleanupTheme();
    return 0;
}

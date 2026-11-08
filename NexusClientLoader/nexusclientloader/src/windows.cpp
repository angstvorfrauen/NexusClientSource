#include "common.h"

bool g_loginCloseHover = false;

bool IsLoginClosePoint(const RECT& rc, POINT pt)
{
    RECT close{ rc.right - 60, 0, rc.right, 48 };
    return PtInRect(&close, pt) != FALSE;
}

void DrawLoginChrome(HDC hdc, const RECT& rc, bool hover)
{
    RECT bar{ 0, 0, rc.right, 48 };
    HBRUSH brush = CreateSolidBrush(RGB(26, 26, 30));
    FillRect(hdc, &bar, brush);
    DeleteObject(brush);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(235, 235, 238));
    HFONT old = reinterpret_cast<HFONT>(SelectObject(hdc, g_fontBold));
    RECT title{ 22, 0, 300, 48 };
    DrawTextW(hdc, L"NexusClient", -1, &title, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    SelectObject(hdc, old);
    RECT close{ rc.right - 60, 0, rc.right, 48 };

    RECT glowLine{ 0, 47, rc.right, 48 };
    const double glowPhase = static_cast<double>(GetTickCount64() % 12000ULL) / 12000.0;
    DrawCleanGradientGlowLine(hdc, glowLine, glowPhase);

    HPEN pen = CreatePen(PS_SOLID, hover ? 3 : 2,
        hover ? RGB(255, 105, 105) : RGB(205, 205, 210));
    HPEN oldPen = reinterpret_cast<HPEN>(SelectObject(hdc, pen));
    int cx = rc.right - 28;
    MoveToEx(hdc, cx - 6, 17, nullptr); LineTo(hdc, cx + 6, 29);
    MoveToEx(hdc, cx + 6, 17, nullptr); LineTo(hdc, cx - 6, 29);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
}

bool CreateLoginWindow(HINSTANCE instance)
{
    const wchar_t CLASS_NAME[] = L"LoginAppLoginWindow";

    if (!RegisterLoginButtonClass(instance) || !RegisterSaveButtonClass(instance))
        return false;

    WNDCLASSW wc{};
    wc.lpfnWndProc = LoginWindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = g_bgBrush;

    if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return false;

    g_loginWindow = CreateWindowExW(
        0, CLASS_NAME, L"",
        WS_POPUP | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        nullptr, nullptr, instance, nullptr);

    if (!g_loginWindow)
        return false;

    ApplyRoundedChildRegion(g_loginWindow, WINDOW_WIDTH, WINDOW_HEIGHT, 20);

    const int marginX = 55;
    const int fieldWidth = WINDOW_WIDTH - (marginX * 2);

    g_titleLabel = CreateWindowW(
        L"STATIC", LOGIN_WINDOW_TITLE,
        WS_VISIBLE | WS_CHILD,
        marginX, 74, fieldWidth, 36,
        g_loginWindow, nullptr, instance, nullptr);
    SendMessageW(g_titleLabel, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontTitle), TRUE);

    g_subtitleLabel = CreateWindowW(
        L"STATIC", LOGIN_SUBTITLE,
        WS_VISIBLE | WS_CHILD,
        marginX, 112, fieldWidth, 22,
        g_loginWindow, nullptr, instance, nullptr);
    SendMessageW(g_subtitleLabel, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontSmall), TRUE);

    g_userLabel = CreateWindowW(
        L"STATIC", LOGIN_USERNAME_LABEL,
        WS_VISIBLE | WS_CHILD,
        marginX, 143, fieldWidth, 20,
        g_loginWindow, nullptr, instance, nullptr);
    SendMessageW(g_userLabel, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontSmall), TRUE);

    g_usernameEdit = CreateWindowExW(
        0, L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_MULTILINE | ES_AUTOHSCROLL | ES_LEFT | ES_NOHIDESEL | ES_WANTRETURN,
        marginX + 1, 168, fieldWidth - 2, EDIT_HEIGHT - 2,
        g_loginWindow, reinterpret_cast<HMENU>(1001), instance, nullptr);
    SendMessageW(g_usernameEdit, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontRegular), TRUE);

    g_passLabel = CreateWindowW(
        L"STATIC", LOGIN_PASSWORD_LABEL,
        WS_VISIBLE | WS_CHILD,
        marginX, 219, fieldWidth, 20,
        g_loginWindow, nullptr, instance, nullptr);
    SendMessageW(g_passLabel, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontSmall), TRUE);

    g_passwordEdit = CreateWindowExW(
        0, L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_MULTILINE | ES_PASSWORD | ES_AUTOHSCROLL | ES_LEFT | ES_NOHIDESEL | ES_WANTRETURN,
        marginX + 1, 244, fieldWidth - 2, EDIT_HEIGHT - 2,
        g_loginWindow, reinterpret_cast<HMENU>(1002), instance, nullptr);
    SendMessageW(g_passwordEdit, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontRegular), TRUE);
    SendMessageW(g_passwordEdit, EM_SETPASSWORDCHAR, L'*', 0);

    SendMessageW(g_usernameEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(EDIT_PADDING_X, EDIT_PADDING_X));
    SendMessageW(g_passwordEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(EDIT_PADDING_X, EDIT_PADDING_X));

    SetWindowSubclass(g_usernameEdit, EditSubclassProc, 1, 0);
    SetWindowSubclass(g_passwordEdit, EditSubclassProc, 2, 0);
    ApplyRoundedChildRegion(g_usernameEdit, fieldWidth, EDIT_HEIGHT, EDIT_ROUNDING);
    ApplyRoundedChildRegion(g_passwordEdit, fieldWidth, EDIT_HEIGHT, EDIT_ROUNDING);
    CenterEditText(g_usernameEdit);
    CenterEditText(g_passwordEdit);

    g_saveButton = CreateWindowExW(
        0, L"NexusSaveButton", L"",
        WS_VISIBLE | WS_CHILD | WS_TABSTOP,
        marginX, 304, 150, 30,
        g_loginWindow, reinterpret_cast<HMENU>(1005), instance, nullptr);
    ApplyRoundedChildRegion(g_saveButton, 150, 30, 8);

    g_loginButton = CreateWindowExW(
        0, L"LoginAppCustomButton", L"",
        WS_VISIBLE | WS_CHILD | WS_TABSTOP,
        WINDOW_WIDTH - marginX - 190, 302, 190, BUTTON_HEIGHT,
        g_loginWindow, reinterpret_cast<HMENU>(1003), instance, nullptr);
    if (!g_loginButton) return false;
    ApplyRoundedChildRegion(g_loginButton, 190, BUTTON_HEIGHT, 8);

    g_statusLabel = CreateWindowW(
        L"STATIC", L"",
        WS_VISIBLE | WS_CHILD | SS_CENTER | SS_CENTERIMAGE,
        marginX, 355, fieldWidth, 30,
        g_loginWindow, nullptr, instance, nullptr);
    SendMessageW(g_statusLabel, WM_SETFONT, reinterpret_cast<WPARAM>(g_fontSmall), TRUE);

    SendMessageW(g_usernameEdit, WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS), 0);
    SendMessageW(g_passwordEdit, WM_CHANGEUISTATE, MAKEWPARAM(UIS_SET, UISF_HIDEFOCUS), 0);

    CenterWindowOnScreen(g_loginWindow, WINDOW_WIDTH, WINDOW_HEIGHT);
    ShowWindow(g_loginWindow, SW_HIDE);
    InvalidateRect(g_loginWindow, nullptr, FALSE);
    CenterEditText(g_usernameEdit);
    CenterEditText(g_passwordEdit);

    SetTimer(g_loginWindow, 3, 35, nullptr);
    return true;
}

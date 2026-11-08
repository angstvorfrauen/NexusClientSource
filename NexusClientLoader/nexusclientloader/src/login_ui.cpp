#include "common.h"

void SetStatus(const wchar_t* text, COLORREF color)
{
    if (!g_statusLabel) return;
    g_statusColor = color;
    SetWindowTextW(g_statusLabel, text);
    InvalidateRect(g_statusLabel, nullptr, TRUE);
    UpdateWindow(g_statusLabel);
}

void CenterEditText(HWND edit)
{
    if (!edit) return;

    RECT rc{};
    GetClientRect(edit, &rc);

    HFONT font = reinterpret_cast<HFONT>(
        SendMessageW(edit, WM_GETFONT, 0, 0));
    if (!font) font = g_fontRegular;

    HDC hdc = GetDC(edit);
    if (!hdc) return;

    HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(hdc, font));
    TEXTMETRICW tm{};
    GetTextMetricsW(hdc, &tm);
    SelectObject(hdc, oldFont);
    ReleaseDC(edit, hdc);

    const int editHeight = rc.bottom - rc.top;
    const int textHeight = tm.tmHeight;
    int top = (editHeight - textHeight) / 2;
    if (top < 1) top = 1;

    RECT textRect{};
    textRect.left = EDIT_PADDING_X;
    textRect.right = rc.right - EDIT_PADDING_X;
    textRect.top = top;
    textRect.bottom = editHeight;

    SendMessageW(edit, EM_SETRECT, 0,
        reinterpret_cast<LPARAM>(&textRect));
}

void PaintEditBorder(HWND hwnd)
{
    RECT rc{};
    GetClientRect(hwnd, &rc);

    HDC hdc = GetDC(hwnd);
    if (!hdc) return;

    COLORREF border = (GetFocus() == hwnd)
        ? g_accentColor
        : g_editBorder;

    HPEN pen = CreatePen(PS_SOLID, 1, border);
    HPEN oldPen = reinterpret_cast<HPEN>(SelectObject(hdc, pen));
    HBRUSH oldBrush = reinterpret_cast<HBRUSH>(
        SelectObject(hdc, GetStockObject(NULL_BRUSH)));

    RoundRect(hdc, 1, 1, rc.right - 2, rc.bottom - 2,
        EDIT_ROUNDING, EDIT_ROUNDING);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);
    ReleaseDC(hwnd, hdc);
}

LRESULT CALLBACK EditSubclassProc(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    UINT_PTR subclassId,
    DWORD_PTR)
{
    switch (message)
    {
    case WM_KEYDOWN:
    {
        if (wParam == VK_RETURN)
        {
            if (hwnd == g_usernameEdit)
            {
                SetFocus(g_passwordEdit);
                return 0;
            }
            if (hwnd == g_passwordEdit)
            {
                PerformLogin();
                return 0;
            }
        }
        LRESULT result = DefSubclassProc(hwnd, message, wParam, lParam);
        InvalidateRect(hwnd, nullptr, FALSE);
        return result;
    }

    case WM_CHAR:
    {
        if (wParam == VK_RETURN) return 0;
        LRESULT result = DefSubclassProc(hwnd, message, wParam, lParam);
        InvalidateRect(hwnd, nullptr, FALSE);
        UpdateWindow(hwnd);
        return result;
    }

    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MOUSEMOVE:
    {
        LRESULT result = DefSubclassProc(hwnd, message, wParam, lParam);
        if (message != WM_MOUSEMOVE || (wParam & MK_LBUTTON))
            InvalidateRect(hwnd, nullptr, FALSE);
        return result;
    }

    case WM_SETFONT:
    {
        LRESULT result = DefSubclassProc(hwnd, message, wParam, lParam);
        CenterEditText(hwnd);
        InvalidateRect(hwnd, nullptr, TRUE);
        return result;
    }

    case WM_SIZE:
    {
        LRESULT result = DefSubclassProc(hwnd, message, wParam, lParam);
        CenterEditText(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);
        return result;
    }

    case WM_SETFOCUS:
    case WM_KILLFOCUS:
    {
        LRESULT result = DefSubclassProc(hwnd, message, wParam, lParam);
        CenterEditText(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);
        return result;
    }

    case WM_PAINT:
    {
        LRESULT result = DefSubclassProc(hwnd, message, wParam, lParam);
        PaintEditBorder(hwnd);
        return result;
    }

    case WM_NCPAINT:
    {
        LRESULT result = DefSubclassProc(hwnd, message, wParam, lParam);
        PaintEditBorder(hwnd);
        return result;
    }

    case WM_NCDESTROY:
        RemoveWindowSubclass(hwnd, EditSubclassProc, subclassId);
        break;
    }

    return DefSubclassProc(hwnd, message, wParam, lParam);
}

void PaintLoginButton(HWND hwnd, HDC hdc)
{
    RECT rc{};
    GetClientRect(hwnd, &rc);

    bool disabled = !IsWindowEnabled(hwnd);

    COLORREF fill = RGB(36, 36, 41);
    if (disabled)
        fill = g_disabledColor;
    else if (g_loginButtonPressed)
        fill = RGB(48, 48, 54);
    else if (g_loginButtonHover)
        fill = RGB(43, 43, 49);
    HBRUSH baseBrush = CreateSolidBrush(g_bgColor);
    FillRect(hdc, &rc, baseBrush);
    DeleteObject(baseBrush);

    HBRUSH brush = CreateSolidBrush(fill);
    HRGN region = CreateRoundRectRgn(
        rc.left, rc.top, rc.right + 1, rc.bottom + 1,
        BUTTON_ROUNDING, BUTTON_ROUNDING);
    FillRgn(hdc, region, brush);
    DeleteObject(region);
    DeleteObject(brush);

    HPEN pen = CreatePen(PS_SOLID, 1, RGB(58, 58, 65));
    HPEN oldPen = reinterpret_cast<HPEN>(SelectObject(hdc, pen));
    HBRUSH oldBrush = reinterpret_cast<HBRUSH>(SelectObject(hdc, GetStockObject(NULL_BRUSH)));
    RoundRect(hdc, 0, 0, rc.right - 1, rc.bottom - 1, BUTTON_ROUNDING, BUTTON_ROUNDING);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(pen);

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(230, 230, 235));
    HFONT oldFont = reinterpret_cast<HFONT>(
        SelectObject(hdc, g_fontRegular));
    DrawTextW(hdc, L"Login", -1, &rc,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(hdc, oldFont);
}

LRESULT CALLBACK LoginButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        PaintLoginButton(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_MOUSEMOVE:
    {
        if (!g_loginButtonHover)
        {
            g_loginButtonHover = true;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        TRACKMOUSEEVENT tme{};
        tme.cbSize = sizeof(tme);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = hwnd;
        TrackMouseEvent(&tme);
        return 0;
    }

    case WM_MOUSELEAVE:
        g_loginButtonHover = false;
        g_loginButtonPressed = false;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;

    case WM_LBUTTONDOWN:
        if (IsWindowEnabled(hwnd))
        {
            g_loginButtonPressed = true;
            SetCapture(hwnd);
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;

    case WM_LBUTTONUP:
        if (g_loginButtonPressed)
        {
            bool wasPressed = g_loginButtonPressed;
            g_loginButtonPressed = false;
            if (GetCapture() == hwnd) ReleaseCapture();
            InvalidateRect(hwnd, nullptr, FALSE);
            if (wasPressed)
            {
                POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                RECT rc{};
                GetClientRect(hwnd, &rc);
                if (PtInRect(&rc, pt))
                    SendMessageW(GetParent(hwnd), WM_COMMAND, MAKEWPARAM(1003, BN_CLICKED), reinterpret_cast<LPARAM>(hwnd));
            }
            return 0;
        }
        break;

    case WM_KEYDOWN:
        if (IsWindowEnabled(hwnd) &&
            (wParam == VK_RETURN || wParam == VK_SPACE))
        {
            g_loginButtonPressed = true;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;

    case WM_KEYUP:
        if (wParam == VK_RETURN || wParam == VK_SPACE)
        {
            bool pressed = g_loginButtonPressed;
            g_loginButtonPressed = false;
            InvalidateRect(hwnd, nullptr, FALSE);
            if (pressed && IsWindowEnabled(hwnd))
                PerformLogin();
            return 0;
        }
        break;

    case WM_ENABLE:
        g_loginButtonPressed = false;
        InvalidateRect(hwnd, nullptr, TRUE);
        UpdateWindow(hwnd);
        return 0;

    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        if (message == WM_KILLFOCUS) g_loginButtonPressed = false;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

bool RegisterLoginButtonClass(HINSTANCE instance)
{
    static bool registered = false;
    if (registered) return true;

    WNDCLASSW wc{};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = LoginButtonProc;
    wc.hInstance = instance;
    wc.lpszClassName = L"LoginAppCustomButton";
    wc.hCursor = LoadCursorW(nullptr, IDC_HAND);
    wc.hbrBackground = nullptr;

    if (!RegisterClassW(&wc))
    {
        if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return false;
    }

    registered = true;
    return true;
}


void PaintSaveButton(HWND hwnd, HDC hdc)
{
    RECT rc{};
    GetClientRect(hwnd, &rc);

    HBRUSH background = CreateSolidBrush(g_bgColor);
    FillRect(hdc, &rc, background);
    DeleteObject(background);

    const int boxSize = 18;
    const int boxTop = (rc.bottom - boxSize) / 2;
    const int boxLeft = 2;

    POINT cursor{};
    GetCursorPos(&cursor);
    ScreenToClient(hwnd, &cursor);
    bool hover = PtInRect(&rc, cursor) != FALSE;

    COLORREF boxFill = g_saveLogin ? g_accentColor : (hover ? RGB(40, 40, 46) : RGB(31, 31, 35));
    COLORREF boxBorder = g_saveLogin ? g_accentColor : (hover ? RGB(88, 88, 98) : RGB(68, 68, 76));

    HBRUSH brush = CreateSolidBrush(boxFill);
    HPEN pen = CreatePen(PS_SOLID, 1, boxBorder);
    HPEN oldPen = reinterpret_cast<HPEN>(SelectObject(hdc, pen));
    HBRUSH oldBrush = reinterpret_cast<HBRUSH>(SelectObject(hdc, brush));
    RoundRect(hdc, boxLeft, boxTop, boxLeft + boxSize, boxTop + boxSize,
        5, 5);
    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);
    DeleteObject(brush);
    DeleteObject(pen);

    if (g_saveLogin)
    {
        HPEN checkPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
        HPEN oldCheckPen = reinterpret_cast<HPEN>(SelectObject(hdc, checkPen));
        MoveToEx(hdc, boxLeft + 4, boxTop + 9, nullptr);
        LineTo(hdc, boxLeft + 8, boxTop + 13);
        LineTo(hdc, boxLeft + 15, boxTop + 5);
        SelectObject(hdc, oldCheckPen);
        DeleteObject(checkPen);
    }

    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(205, 205, 212));
    HFONT oldFont = reinterpret_cast<HFONT>(SelectObject(hdc, g_fontSmall));
    RECT text{ boxLeft + boxSize + 9, 0, rc.right, rc.bottom };
    DrawTextW(hdc, L"Remember me", -1, &text,
        DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
    SelectObject(hdc, oldFont);
}

LRESULT CALLBACK SaveButtonProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps{};
        HDC hdc = BeginPaint(hwnd, &ps);
        PaintSaveButton(hwnd, hdc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_MOUSEMOVE:
    {
        TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tme);
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    }
    case WM_MOUSELEAVE:
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_LBUTTONUP:
        g_saveLogin = true;
        SetFocus(hwnd);
        InvalidateRect(hwnd, nullptr, FALSE);
        UpdateWindow(hwnd);
        return 0;
    case WM_KEYUP:
        if (wParam == VK_SPACE || wParam == VK_RETURN)
        {
            g_saveLogin = true;
            InvalidateRect(hwnd, nullptr, FALSE);
            return 0;
        }
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

bool RegisterSaveButtonClass(HINSTANCE instance)
{
    static bool registered = false;
    if (registered) return true;

    WNDCLASSW wc{};
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = SaveButtonProc;
    wc.hInstance = instance;
    wc.lpszClassName = L"NexusSaveButton";
    wc.hCursor = LoadCursorW(nullptr, IDC_HAND);
    wc.hbrBackground = nullptr;

    if (!RegisterClassW(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        return false;

    registered = true;
    return true;
}

void PerformLogin()
{
    g_saveLogin = true;

    if (!g_usernameEdit || !g_passwordEdit || !g_loginButton || !IsWindowEnabled(g_loginButton))
        return;

    std::wstring username = GetText(g_usernameEdit);
    std::wstring password = GetText(g_passwordEdit);

    if (username.empty())
    {
        MessageBoxW(g_loginWindow, L"Please enter your username.", L"NexusClient", MB_OK | MB_ICONERROR);
        SetFocus(g_usernameEdit);
        return;
    }

    if (password.empty())
    {
        MessageBoxW(g_loginWindow, L"Please enter your password.", L"NexusClient", MB_OK | MB_ICONERROR);
        SetFocus(g_passwordEdit);
        return;
    }

    EnableWindow(g_loginButton, FALSE);

    LoginResult result = LoginAPI(username, password);

    if (result.networkError)
    {
        wchar_t msg[160];
        swprintf_s(msg, L"The server could not be reached.\nError code: %lu", result.lastError);
        MessageBoxW(g_loginWindow, msg, L"NexusClient", MB_OK | MB_ICONERROR);
        EnableWindow(g_loginButton, TRUE);
        SetFocus(g_passwordEdit);
        return;
    }

    if (result.success)
    {
        g_loggedInUsername =
            result.username.empty() ? username : result.username;
        g_authToken = result.token;
        g_profilePictureDataUrl = result.profilePicture;
        g_subscriptionExpiresAt = result.subscriptionExpiresAt;
        if (_wcsicmp(g_loggedInUsername.c_str(), L"YOUR_USERNAME") == 0)
            g_subscriptionExpiresAt = L"9999-12-31T23:59:59Z";

        if (g_saveLogin)
        {
            if (!SaveAuthToken())
            {
                MessageBoxW(g_loginWindow, L"Could not save the session. Please try again.", L"NexusClient", MB_OK | MB_ICONWARNING);
            }
        }
        else
        {
            ClearAuthToken();
        }
        LoadProfileBitmap(g_profilePictureDataUrl);

        std::wstring folder = GetAppDataFolder();

        if (folder.empty())
        {
            MessageBoxW(g_loginWindow, L"The AppData folder is unavailable.", L"NexusClient", MB_OK | MB_ICONERROR);
            EnableWindow(g_loginButton, TRUE);
            return;
        }

        g_downloadedExePath = folder + L"\\" + EXE_FILENAME;

        DeleteFileW(g_downloadedExePath.c_str());
        DeleteFileW((g_downloadedExePath + L".download").c_str());

        ShowWindow(g_loginWindow, SW_HIDE);
        OpenMainWindow();
        return;
    }

    MessageBoxW(g_loginWindow, L"Username or password is incorrect.", L"NexusClient", MB_OK | MB_ICONERROR);
    EnableWindow(g_loginButton, TRUE);
    SetFocus(g_passwordEdit);
}

LRESULT CALLBACK LoginWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_ERASEBKGND:
    {
        HDC hdc = reinterpret_cast<HDC>(wParam);
        RECT rect{};
        GetClientRect(hwnd, &rect);
        HBRUSH bg = CreateSolidBrush(g_bgColor);
        FillRect(hdc, &rect, bg);
        DeleteObject(bg);
        return 1;
    }
    case WM_NCHITTEST:
    {
        LRESULT hit = DefWindowProcW(hwnd, message, wParam, lParam);
        if (hit == HTCLIENT)
        {
            POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ScreenToClient(hwnd, &pt);
            RECT rc{}; GetClientRect(hwnd, &rc);
            if (pt.y < 48 && !IsLoginClosePoint(rc, pt)) return HTCAPTION;
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

        HBRUSH bg = CreateSolidBrush(g_bgColor);
        FillRect(hdc, &rc, bg);
        DeleteObject(bg);

        DrawLoginChrome(hdc, rc, g_loginCloseHover);

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
        bool hover = IsLoginClosePoint(rc, pt);
        if (hover != g_loginCloseHover)
        {
            g_loginCloseHover = hover;
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        TRACKMOUSEEVENT tme{ sizeof(tme), TME_LEAVE, hwnd, 0 };
        TrackMouseEvent(&tme);
        return 0;
    }
    case WM_MOUSELEAVE:
        g_loginCloseHover = false;
        InvalidateRect(hwnd, nullptr, FALSE);
        return 0;
    case WM_LBUTTONDOWN:
    {
        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RECT rc{}; GetClientRect(hwnd, &rc);
        if (IsLoginClosePoint(rc, pt)) { SetCapture(hwnd); return 0; }
        break;
    }
    case WM_LBUTTONUP:
    {
        if (GetCapture() == hwnd) ReleaseCapture();
        POINT pt{ GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
        RECT rc{}; GetClientRect(hwnd, &rc);
        if (IsLoginClosePoint(rc, pt))
        {
            DestroyWindow(hwnd);
            return 0;
        }
        break;
    }
    case WM_CTLCOLORSTATIC:
    {
        HDC hdcStatic = reinterpret_cast<HDC>(wParam);
        HWND ctrl = reinterpret_cast<HWND>(lParam);

        if (ctrl == g_statusLabel)
            SetTextColor(hdcStatic, g_statusColor);
        else if (ctrl == g_subtitleLabel)
            SetTextColor(hdcStatic, g_subTextColor);
        else if (ctrl == g_userLabel || ctrl == g_passLabel)
            SetTextColor(hdcStatic, g_labelColor);
        else
            SetTextColor(hdcStatic, g_textColor);

        SetBkColor(hdcStatic, g_bgColor);
        SetBkMode(hdcStatic, TRANSPARENT);
        return reinterpret_cast<INT_PTR>(g_bgBrush);
    }
    case WM_CTLCOLOREDIT:
    {
        HDC hdcEdit = reinterpret_cast<HDC>(wParam);
        SetTextColor(hdcEdit, g_textColor);
        SetBkColor(hdcEdit, g_editBgColor);
        SetBkMode(hdcEdit, OPAQUE);
        return reinterpret_cast<INT_PTR>(g_editBrush);
    }
    case WM_TIMER:
        if (wParam == 3)
        {
            RECT client{};
            GetClientRect(hwnd, &client);
            RECT glowRect{ 0, 45, client.right, 50 };
            InvalidateRect(hwnd, &glowRect, FALSE);
            return 0;
        }
        break;
    case WM_ACTIVATE:
        if (g_loginButton) InvalidateRect(g_loginButton, nullptr, FALSE);
        if (g_saveButton) InvalidateRect(g_saveButton, nullptr, FALSE);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wParam) == 1003)
        {
            PerformLogin();
            return 0;
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, 3);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, message, wParam, lParam);
}

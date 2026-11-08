#include "common.h"

std::wstring g_serverHost = L"xeuka.net";

HWND g_loginWindow = nullptr;
HWND g_usernameEdit = nullptr;
HWND g_passwordEdit = nullptr;
HWND g_loginButton = nullptr;
HWND g_statusLabel = nullptr;
HWND g_titleLabel = nullptr;
HWND g_subtitleLabel = nullptr;
HWND g_userLabel = nullptr;
HWND g_passLabel = nullptr;
HWND g_saveButton = nullptr;
bool g_saveLogin = true;

HWND g_mainWindow = nullptr;
HWND g_mainUsernameLabel = nullptr;
HWND g_startButton = nullptr;
HWND g_versionLabel = nullptr;

std::wstring g_loggedInUsername;
std::wstring g_authToken;
std::wstring g_downloadedExePath;
std::wstring g_appVersion = APP_VERSION;
std::wstring g_subscriptionExpiresAt;
std::string g_profilePictureDataUrl;

std::unique_ptr<Gdiplus::Bitmap> g_profileBitmap;
ULONG_PTR g_gdiplusToken = 0;

HFONT g_fontRegular = nullptr;
HFONT g_fontBold = nullptr;
HFONT g_fontTitle = nullptr;
HFONT g_fontSmall = nullptr;
HBRUSH g_bgBrush = nullptr;
HBRUSH g_editBrush = nullptr;

COLORREF g_bgColor = RGB(22, 22, 25);
COLORREF g_editBgColor = RGB(31, 31, 35);
COLORREF g_editBorder = RGB(52, 52, 58);
COLORREF g_textColor = RGB(235, 235, 238);
COLORREF g_subTextColor = RGB(145, 145, 152);
COLORREF g_labelColor = RGB(185, 185, 192);
COLORREF g_accentColor = RGB(145, 125, 255);
COLORREF g_accentHover = RGB(160, 142, 255);
COLORREF g_accentPress = RGB(120, 102, 225);
COLORREF g_accent2Color = RGB(150, 235, 180);
COLORREF g_dangerColor = RGB(255, 110, 110);
COLORREF g_disabledColor = RGB(45, 45, 50);
COLORREF g_statusColor = g_subTextColor;

bool g_loginButtonHover = false;
bool g_loginButtonPressed = false;
bool g_startButtonHover = false;
bool g_startButtonPressed = false;
bool g_startupClosing = false;
bool g_restartingFromLogout = false;
bool g_profileMenuOpen = false;
bool g_profileMenuHoverWebsite = false;
bool g_profileMenuHoverLogout = false;

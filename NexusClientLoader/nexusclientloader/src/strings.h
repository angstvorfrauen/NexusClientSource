#pragma once
#include <string>

extern std::wstring g_serverHost;

inline constexpr unsigned short SERVER_PORT = 443;
inline constexpr wchar_t LOGIN_API[] = L"/api/login";
inline constexpr wchar_t VERSION_API[] = L"/api/version";
inline constexpr wchar_t DOWNLOAD_API[] = L"/download/client";
inline constexpr wchar_t LOADER_LOGIN_API[] = L"/api/loader/login";
inline constexpr wchar_t LOADER_SESSION_API[] = L"/api/loader/session";
inline constexpr wchar_t LOADER_SESSION_FILE[] = L"session.dat";

inline constexpr wchar_t CHEAT_FILENAME[] = L"n234g90gsbghds.exe";
inline constexpr wchar_t APP_DATA_FOLDER[] = L"NexusClient";
inline constexpr wchar_t EXE_FILENAME[] = L"nexusclient-beta.exe";

inline constexpr wchar_t APP_NAME[] = L"NexusClient";
inline constexpr wchar_t APP_VERSION[] = L"1.0.0";

inline constexpr wchar_t LOGIN_WINDOW_TITLE[] = L"Login";
inline constexpr wchar_t LOGIN_SUBTITLE[] = L"Enter your account to use our cheat!";
inline constexpr wchar_t LOGIN_USERNAME_LABEL[] = L"Username";
inline constexpr wchar_t LOGIN_PASSWORD_LABEL[] = L"Password";
inline constexpr wchar_t START_BUTTON_TEXT[] = L"START";
inline constexpr wchar_t SAVE_BUTTON_TEXT[] = L"Save";

inline constexpr wchar_t STATUS_LOGIN_CHECK[] = L"Check Login...";
inline constexpr wchar_t STATUS_LOGIN_SUCCESS[] = L"Login successful!";
inline constexpr wchar_t STATUS_LOGIN_INVALID[] = L"Username or password is wrong.";
inline constexpr wchar_t STATUS_LOGIN_EMPTY[] = L"Username or password is invalid!";
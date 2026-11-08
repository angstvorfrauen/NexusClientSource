#include "common.h"

std::wstring GetSessionFilePath()
{
    std::wstring folder = GetAppDataFolder();
    if (folder.empty()) return L"";
    return folder + L"\\" + LOADER_SESSION_FILE;
}

bool ProtectToken(const std::wstring& token, std::vector<BYTE>& protectedData)
{
    protectedData.clear();
    if (token.empty()) return false;

    DATA_BLOB input{};
    input.pbData = reinterpret_cast<BYTE*>(const_cast<wchar_t*>(token.data()));
    input.cbData = static_cast<DWORD>((token.size() + 1) * sizeof(wchar_t));

    DATA_BLOB output{};
    if (!CryptProtectData(&input, L"NexusClient loader session",
        nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output))
        return false;

    protectedData.assign(output.pbData, output.pbData + output.cbData);
    LocalFree(output.pbData);
    return true;
}

bool UnprotectToken(const std::vector<BYTE>& protectedData, std::wstring& token)
{
    token.clear();
    if (protectedData.empty()) return false;

    DATA_BLOB input{};
    input.pbData = const_cast<BYTE*>(protectedData.data());
    input.cbData = static_cast<DWORD>(protectedData.size());

    DATA_BLOB output{};
    if (!CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr,
        CRYPTPROTECT_UI_FORBIDDEN, &output))
        return false;

    const wchar_t* value = reinterpret_cast<const wchar_t*>(output.pbData);
    if (value) token = value;
    LocalFree(output.pbData);
    return !token.empty();
}

bool SaveAuthToken()
{
    const std::wstring path = GetSessionFilePath();
    if (path.empty() || g_authToken.empty()) return false;

    std::vector<BYTE> protectedData;
    if (!ProtectToken(g_authToken, protectedData)) return false;

    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_HIDDEN, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;

    DWORD written = 0;
    BOOL ok = WriteFile(file, protectedData.data(),
        static_cast<DWORD>(protectedData.size()), &written, nullptr);
    CloseHandle(file);
    return ok && written == protectedData.size();
}

bool LoadAuthToken()
{
    const std::wstring path = GetSessionFilePath();
    if (path.empty()) return false;

    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ,
        nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_HIDDEN, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;

    DWORD size = GetFileSize(file, nullptr);
    if (size == INVALID_FILE_SIZE || size == 0 || size > 64 * 1024)
    {
        CloseHandle(file);
        return false;
    }

    std::vector<BYTE> data(size);
    DWORD read = 0;
    BOOL ok = ReadFile(file, data.data(), size, &read, nullptr);
    CloseHandle(file);

    if (!ok || read != size) return false;

    return UnprotectToken(data, g_authToken);
}

void ClearAuthToken()
{
    g_authToken.clear();
    const std::wstring path = GetSessionFilePath();
    if (!path.empty()) DeleteFileW(path.c_str());
}

LoaderSessionResult LoaderLoginAPI(const std::wstring& username,
    const std::wstring& password,
    const std::wstring& hwid)
{
    LoaderSessionResult result;

    std::wstring body =
        L"{\"username\":\"" + EscapeJson(username) +
        L"\",\"password\":\"" + EscapeJson(password) +
        L"\",\"hwid\":\"" + EscapeJson(hwid) + L"\"}";
    std::string bodyUtf8 = WideToUtf8(body);

    HINTERNET session = WinHttpOpen(L"NexusClientLoader/1.0",
        WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) { result.networkError = true; result.lastError = GetLastError(); return result; }

    HINTERNET connection = WinHttpConnect(session, g_serverHost.c_str(), SERVER_PORT, 0);
    if (!connection) {
        result.networkError = true; result.lastError = GetLastError();
        WinHttpCloseHandle(session); return result;
    }

    HINTERNET request = WinHttpOpenRequest(connection, L"POST",
        LOADER_LOGIN_API, nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!request) {
        result.networkError = true; result.lastError = GetLastError();
        WinHttpCloseHandle(connection); WinHttpCloseHandle(session); return result;
    }

    WinHttpSetTimeouts(request, 10000, 10000, 15000, 15000);
    std::wstring headers =
        L"Content-Type: application/json\r\n"
        L"Accept: application/json\r\n"
        L"X-Nexus-HWID: " + hwid + L"\r\n";

    BOOL sent = WinHttpSendRequest(request, headers.c_str(), static_cast<DWORD>(-1L),
        bodyUtf8.data(), static_cast<DWORD>(bodyUtf8.size()),
        static_cast<DWORD>(bodyUtf8.size()), 0);

    if (!sent || !WinHttpReceiveResponse(request, nullptr)) {
        result.networkError = true; result.lastError = GetLastError();
        WinHttpCloseHandle(request); WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session); return result;
    }

    DWORD status = 0;
    QueryHttpStatus(request, status);

    DWORD available = 0;
    while (WinHttpQueryDataAvailable(request, &available) && available > 0) {
        std::string buffer(available, '\0');
        DWORD got = 0;
        if (!WinHttpReadData(request, buffer.data(), available, &got) || got == 0) break;
        result.response.append(buffer.data(), got);
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    result.success = (status == 200 &&
        (result.response.find("\"success\":true") != std::string::npos ||
            result.response.find("\"success\": true") != std::string::npos));

    return result;
}

bool ValidateSavedSession()
{
    if (g_authToken.empty()) return false;

    HINTERNET session = WinHttpOpen(L"NexusClientLoader/1.0",
        WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) return false;

    HINTERNET connection = WinHttpConnect(session, g_serverHost.c_str(), SERVER_PORT, 0);
    if (!connection) { WinHttpCloseHandle(session); return false; }

    HINTERNET request = WinHttpOpenRequest(connection, L"GET",
        LOADER_SESSION_API, nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!request) {
        WinHttpCloseHandle(connection); WinHttpCloseHandle(session); return false;
    }

    std::wstring headers = L"Accept: application/json\r\nAuthorization: Bearer " + g_authToken + L"\r\nX-Nexus-HWID: " + GetMachineHwid() + L"\r\n";
    bool ok = WinHttpSendRequest(request, headers.c_str(), static_cast<DWORD>(-1L),
        WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
        WinHttpReceiveResponse(request, nullptr);

    DWORD status = 0;
    if (ok) QueryHttpStatus(request, status);

    std::string response;
    DWORD available = 0;
    while (ok && WinHttpQueryDataAvailable(request, &available) && available > 0) {
        std::string buffer(available, '\0');
        DWORD got = 0;
        if (!WinHttpReadData(request, buffer.data(), available, &got) || got == 0) break;
        response.append(buffer.data(), got);
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);

    if (status != 200 ||
        response.find("\"authenticated\":true") == std::string::npos)
        return false;

    g_loggedInUsername = Utf8ToWide(GetJsonString(response, "username"));
    g_profilePictureDataUrl = GetJsonString(response, "avatar");
    g_subscriptionExpiresAt = Utf8ToWide(GetJsonString(response, "expiresAt"));
    if (_wcsicmp(g_loggedInUsername.c_str(), L"YOUR_USERNAME") == 0)
        g_subscriptionExpiresAt = L"9999-12-31T23:59:59Z";
    LoadProfileBitmap(g_profilePictureDataUrl);
    return !g_loggedInUsername.empty();
}

LoginResult LoginAPI(const std::wstring& username, const std::wstring& password)
{
    LoginResult result;
    std::wstring hwid = GetMachineHwid();
    LoaderSessionResult api = LoaderLoginAPI(username, password, hwid);

    result.networkError = api.networkError;
    result.lastError = api.lastError;
    result.response = api.response;

    if (api.networkError) return result;
    if (!api.success) return result;

    result.success = true;
    result.username = Utf8ToWide(GetJsonString(api.response, "username"));
    result.token = Utf8ToWide(GetJsonString(api.response, "token"));
    result.profilePicture = GetJsonString(api.response, "avatar");
    result.subscriptionExpiresAt = Utf8ToWide(GetJsonString(api.response, "expiresAt"));
    return result;
}


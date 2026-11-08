#include "common.h"

std::string WideToUtf8(const std::wstring& text)
{
    if (text.empty()) return {};

    int size = WideCharToMultiByte(CP_UTF8, 0, text.data(),
        static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);

    if (size <= 0) return {};

    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(),
        static_cast<int>(text.size()), result.data(), size, nullptr, nullptr);
    return result;
}

std::wstring Utf8ToWide(const std::string& text)
{
    if (text.empty()) return {};

    int size = MultiByteToWideChar(CP_UTF8, 0, text.data(),
        static_cast<int>(text.size()), nullptr, 0);

    if (size <= 0) return {};

    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(),
        static_cast<int>(text.size()), result.data(), size);
    return result;
}

std::wstring EscapeJson(const std::wstring& text)
{
    std::wstring result;
    for (wchar_t c : text)
    {
        switch (c)
        {
        case L'\\': result += L"\\\\"; break;
        case L'"':  result += L"\\\""; break;
        case L'\r': result += L"\\r"; break;
        case L'\n': result += L"\\n"; break;
        case L'\t': result += L"\\t"; break;
        default: result += c; break;
        }
    }
    return result;
}

std::string GetJsonString(const std::string& json, const std::string& key)
{
    const std::string needle = "\"" + key + "\"";
    size_t p = json.find(needle);
    if (p == std::string::npos) return {};

    p = json.find(':', p + needle.size());
    if (p == std::string::npos) return {};

    p = json.find('"', p + 1);
    if (p == std::string::npos) return {};

    ++p;
    std::string value;
    while (p < json.size())
    {
        if (json[p] == '"' && (p == 0 || json[p - 1] != '\\'))
            break;

        if (json[p] == '\\' && p + 1 < json.size())
        {
            ++p;
            switch (json[p])
            {
            case '"': value += '"'; break;
            case '\\': value += '\\'; break;
            case 'n': value += '\n'; break;
            case 'r': value += '\r'; break;
            case 't': value += '\t'; break;
            default: value += json[p]; break;
            }
        }
        else
        {
            value += json[p];
        }
        ++p;
    }
    return value;
}

std::wstring GetText(HWND hwnd)
{
    if (!hwnd) return L"";

    int length = GetWindowTextLengthW(hwnd);
    if (length <= 0) return L"";

    std::wstring text(static_cast<size_t>(length) + 1, L'\0');
    int copied = GetWindowTextW(hwnd, text.data(), length + 1);

    if (copied <= 0) return L"";

    text.resize(copied);
    return text;
}

bool QueryHttpStatus(HINTERNET request, DWORD& statusCode)
{
    statusCode = 0;
    DWORD size = sizeof(statusCode);

    return WinHttpQueryHeaders(
        request,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        NULL,
        &statusCode,
        &size,
        NULL) != FALSE;
}

void CloseWinHttp(HINTERNET request, HINTERNET connection, HINTERNET session)
{
    if (request)    WinHttpCloseHandle(request);
    if (connection) WinHttpCloseHandle(connection);
    if (session)    WinHttpCloseHandle(session);
}


std::wstring GetMachineHwid()
{
    HKEY key = nullptr;
    if (RegOpenKeyExW(
        HKEY_LOCAL_MACHINE,
        L"SOFTWARE\\Microsoft\\Cryptography",
        0,
        KEY_READ | KEY_WOW64_64KEY,
        &key) != ERROR_SUCCESS)
    {
        if (RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Microsoft\\Cryptography",
            0,
            KEY_READ,
            &key) != ERROR_SUCCESS)
        {
            return L"";
        }
    }

    wchar_t buffer[256]{};
    DWORD type = 0;
    DWORD size = sizeof(buffer);

    LONG result = RegQueryValueExW(
        key,
        L"MachineGuid",
        nullptr,
        &type,
        reinterpret_cast<LPBYTE>(buffer),
        &size);

    RegCloseKey(key);

    if (result != ERROR_SUCCESS ||
        (type != REG_SZ && type != REG_EXPAND_SZ) ||
        buffer[0] == L'\0')
    {
        return L"";
    }

    return std::wstring(buffer);
}

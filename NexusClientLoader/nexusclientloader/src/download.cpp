#include "common.h"

bool IsValidWindowsPE(const std::wstring& path)
{
    HANDLE file = CreateFileW(
        path.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (file == INVALID_HANDLE_VALUE)
        return false;

    IMAGE_DOS_HEADER dos{};
    DWORD read = 0;

    if (!ReadFile(file, &dos, sizeof(dos), &read, NULL) ||
        read != sizeof(dos) ||
        dos.e_magic != IMAGE_DOS_SIGNATURE)
    {
        CloseHandle(file);
        return false;
    }

    if (dos.e_lfanew < sizeof(IMAGE_DOS_HEADER) ||
        dos.e_lfanew > 16 * 1024 * 1024)
    {
        CloseHandle(file);
        return false;
    }

    LARGE_INTEGER offset{};
    offset.QuadPart = dos.e_lfanew;

    if (!SetFilePointerEx(file, offset, nullptr, FILE_BEGIN))
    {
        CloseHandle(file);
        return false;
    }

    DWORD signature = 0;

    if (!ReadFile(file, &signature, sizeof(signature), &read, NULL) ||
        read != sizeof(signature) ||
        signature != IMAGE_NT_SIGNATURE)
    {
        CloseHandle(file);
        return false;
    }

    IMAGE_FILE_HEADER fileHeader{};

    if (!ReadFile(file, &fileHeader, sizeof(fileHeader), &read, NULL) ||
        read != sizeof(fileHeader))
    {
        CloseHandle(file);
        return false;
    }

    return fileHeader.Machine == IMAGE_FILE_MACHINE_I386 ||
        fileHeader.Machine == IMAGE_FILE_MACHINE_AMD64 ||
        fileHeader.Machine == IMAGE_FILE_MACHINE_ARM64;
}

bool ReadHttpResponse(
    const wchar_t* userAgent,
    const wchar_t* apiPath,
    std::string& response)
{
    response.clear();

    HINTERNET session = WinHttpOpen(
        userAgent,
        WINHTTP_ACCESS_TYPE_NO_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    if (!session)
        return false;

    HINTERNET connection =
        WinHttpConnect(session, g_serverHost.c_str(), SERVER_PORT, 0);

    if (!connection)
    {
        CloseWinHttp(nullptr, nullptr, session);
        return false;
    }

    HINTERNET request = WinHttpOpenRequest(
        connection,
        L"GET",
        apiPath,
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);

    if (!request)
    {
        CloseWinHttp(nullptr, connection, session);
        return false;
    }

    WinHttpSetTimeouts(request, 10000, 10000, 15000, 15000);

    if (!WinHttpSendRequest(
        request,
        WINHTTP_NO_ADDITIONAL_HEADERS,
        0,
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0) ||
        !WinHttpReceiveResponse(request, nullptr))
    {
        CloseWinHttp(request, connection, session);
        return false;
    }

    DWORD statusCode = 0;

    if (!QueryHttpStatus(request, statusCode) || statusCode != 200)
    {
        CloseWinHttp(request, connection, session);
        return false;
    }

    DWORD available = 0;

    while (WinHttpQueryDataAvailable(request, &available))
    {
        if (available == 0)
            break;

        std::vector<char> buffer(available);
        DWORD downloaded = 0;

        if (!WinHttpReadData(
            request,
            buffer.data(),
            available,
            &downloaded))
        {
            CloseWinHttp(request, connection, session);
            return false;
        }

        if (downloaded == 0)
            break;

        response.append(buffer.data(), downloaded);
    }

    CloseWinHttp(request, connection, session);
    return true;
}

std::wstring GetAppDataFolder()
{
    wchar_t* local = _wgetenv(L"APPDATA");
    if (!local) return L"";
    std::wstring folder = local;
    folder += L"\\";
    folder += APP_DATA_FOLDER;
    CreateDirectoryW(folder.c_str(), nullptr);
    return folder;
}

bool DownloadFile(const std::wstring& localPath)
{
    HINTERNET session = WinHttpOpen(
        L"NexusClientLoader/1.0",
        WINHTTP_ACCESS_TYPE_NO_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    if (!session)
        return false;

    HINTERNET connection =
        WinHttpConnect(session, g_serverHost.c_str(), SERVER_PORT, 0);

    if (!connection)
    {
        CloseWinHttp(nullptr, nullptr, session);
        return false;
    }

    HINTERNET request = WinHttpOpenRequest(
        connection,
        L"GET",
        DOWNLOAD_API,
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);

    if (!request)
    {
        CloseWinHttp(nullptr, connection, session);
        return false;
    }

    WinHttpSetTimeouts(request, 10000, 10000, 30000, 30000);

    std::wstring headers =
        L"Accept: application/octet-stream\r\n"
        L"Cache-Control: no-cache\r\n";

    if (!g_authToken.empty())
    {
        headers += L"Authorization: Bearer ";
        headers += g_authToken;
        headers += L"\r\n";
    }

    if (!WinHttpSendRequest(
        request,
        headers.c_str(),
        static_cast<DWORD>(-1L),
        WINHTTP_NO_REQUEST_DATA,
        0,
        0,
        0) ||
        !WinHttpReceiveResponse(request, nullptr))
    {
        CloseWinHttp(request, connection, session);
        return false;
    }

    DWORD statusCode = 0;
    if (!QueryHttpStatus(request, statusCode))
    {
        CloseWinHttp(request, connection, session);
        return false;
    }

    if (statusCode != 200)
    {
        wchar_t error[160]{};
        swprintf_s(
            error,
            L"Download HTTP Status: %lu",
            statusCode);

        MessageBoxW(
            g_loginWindow,
            error,
            L"NexusClient Download",
            MB_OK | MB_ICONERROR);

        CloseWinHttp(request, connection, session);
        return false;
    }

    ULONGLONG expectedSize = 0;
    bool hasExpectedSize = false;

    DWORD contentLength = 0;
    DWORD contentLengthSize = sizeof(contentLength);

    if (WinHttpQueryHeaders(
        request,
        WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
        WINHTTP_HEADER_NAME_BY_INDEX,
        &contentLength,
        &contentLengthSize,
        WINHTTP_NO_HEADER_INDEX))
    {
        expectedSize = contentLength;
        hasExpectedSize = true;
    }

    const std::wstring tempPath = localPath + L".download";
    DeleteFileW(tempPath.c_str());

    HANDLE file = CreateFileW(
        tempPath.c_str(),
        GENERIC_WRITE,
        0,
        nullptr,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr);

    if (file == INVALID_HANDLE_VALUE)
    {
        CloseWinHttp(request, connection, session);
        return false;
    }

    bool success = true;
    ULONGLONG totalDownloaded = 0;

    for (;;)
    {
        DWORD available = 0;

        if (!WinHttpQueryDataAvailable(request, &available))
        {
            success = false;
            break;
        }

        if (available == 0)
            break;

        std::vector<BYTE> buffer(available);
        DWORD downloaded = 0;

        if (!WinHttpReadData(
            request,
            buffer.data(),
            available,
            &downloaded))
        {
            success = false;
            break;
        }

        if (downloaded == 0)
            break;

        DWORD written = 0;

        if (!WriteFile(
            file,
            buffer.data(),
            downloaded,
            &written,
            nullptr) ||
            written != downloaded)
        {
            success = false;
            break;
        }

        totalDownloaded += downloaded;
    }

    FlushFileBuffers(file);
    CloseHandle(file);
    CloseWinHttp(request, connection, session);

    if (!success || totalDownloaded == 0)
    {
        DeleteFileW(tempPath.c_str());
        return false;
    }

    if (hasExpectedSize &&
        expectedSize != 0 &&
        totalDownloaded != expectedSize)
    {
        DeleteFileW(tempPath.c_str());
        return false;
    }

    if (!IsValidWindowsPE(tempPath))
    {
        DeleteFileW(tempPath.c_str());
        return false;
    }

    DeleteFileW(localPath.c_str());

    if (!MoveFileW(
        tempPath.c_str(),
        localPath.c_str()))
    {
        DeleteFileW(tempPath.c_str());
        return false;
    }

    return IsValidWindowsPE(localPath);
}


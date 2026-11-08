#include "common.h"

std::wstring NormalizeVersion(std::wstring version)
{
    while (!version.empty() &&
        (version.front() == L' ' ||
            version.front() == L'\r' ||
            version.front() == L'\n' ||
            version.front() == L'\t'))
    {
        version.erase(version.begin());
    }

    while (!version.empty() &&
        (version.back() == L' ' ||
            version.back() == L'\r' ||
            version.back() == L'\n' ||
            version.back() == L'\t'))
    {
        version.pop_back();
    }

    if (!version.empty() &&
        (version.front() == L'v' || version.front() == L'V'))
    {
        version.erase(version.begin());
    }

    return version.empty() ? L"0.0.0" : version;
}

std::wstring FetchVersionFromServer()
{
    std::string response;

    if (!ReadHttpResponse(
        L"NexusClientLoader/1.0",
        VERSION_API,
        response))
    {
        return L"0.0.0";
    }

    std::string version = GetJsonString(response, "version");

    if (version.empty())
        return L"0.0.0";

    return NormalizeVersion(Utf8ToWide(version));
}
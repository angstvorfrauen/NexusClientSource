#include "common.h"

bool Base64ToBytes(const std::string& input, std::vector<BYTE>& output)
{
    output.clear();
    if (input.empty()) return false;

    DWORD required = 0;
    if (!CryptStringToBinaryA(input.c_str(), 0, CRYPT_STRING_BASE64,
        nullptr, &required, nullptr, nullptr))
        return false;

    output.resize(required);
    if (!CryptStringToBinaryA(input.c_str(), 0, CRYPT_STRING_BASE64,
        output.data(), &required, nullptr, nullptr))
    {
        output.clear();
        return false;
    }

    output.resize(required);
    return true;
}

void ClearProfileBitmap()
{
    g_profileBitmap.reset();
}

bool LoadProfileBitmap(const std::string& dataUrl)
{
    ClearProfileBitmap();
    if (dataUrl.empty()) return false;

    const size_t comma = dataUrl.find(',');
    if (comma == std::string::npos) return false;

    const std::string payload = dataUrl.substr(comma + 1);
    std::vector<BYTE> bytes;
    if (!Base64ToBytes(payload, bytes) || bytes.empty()) return false;

    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes.size());
    if (!memory) return false;

    void* dst = GlobalLock(memory);
    if (!dst)
    {
        GlobalFree(memory);
        return false;
    }
    memcpy(dst, bytes.data(), bytes.size());
    GlobalUnlock(memory);

    IStream* stream = nullptr;
    if (FAILED(CreateStreamOnHGlobal(memory, TRUE, &stream)) || !stream)
    {
        GlobalFree(memory);
        return false;
    }

    std::unique_ptr<Gdiplus::Bitmap> bitmap(Gdiplus::Bitmap::FromStream(stream));
    stream->Release();

    if (!bitmap || bitmap->GetLastStatus() != Gdiplus::Ok)
        return false;

    g_profileBitmap = std::move(bitmap);
    return true;
}

void DrawProfileAvatar(HDC hdc, const RECT& bounds)
{
    const int size = min(bounds.right - bounds.left, bounds.bottom - bounds.top);
    const int x = bounds.left;
    const int y = bounds.top;

    Graphics graphics(hdc);
    graphics.SetSmoothingMode(SmoothingModeAntiAlias);
    graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    graphics.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    SolidBrush bg(Color(255, 17, 17, 17));
    graphics.FillEllipse(&bg, x, y, size, size);

    Pen border(Color(255, 48, 48, 48), 1.0f);
    graphics.DrawEllipse(&border, x, y, size - 1, size - 1);

    if (g_profileBitmap)
    {
        GraphicsPath clip;
        clip.AddEllipse(static_cast<REAL>(x), static_cast<REAL>(y),
            static_cast<REAL>(size), static_cast<REAL>(size));
        graphics.SetClip(&clip);
        Gdiplus::Image* image = static_cast<Gdiplus::Image*>(g_profileBitmap.get());
        graphics.DrawImage(image, static_cast<INT>(x), static_cast<INT>(y),
            static_cast<INT>(size), static_cast<INT>(size));
        graphics.ResetClip();
        graphics.DrawEllipse(&border, x, y, size - 1, size - 1);
    }
    else
    {
        wchar_t initial = g_loggedInUsername.empty() ? L'N' : g_loggedInUsername.front();
        wchar_t text[2] = { initial, L'\0' };
        HFONT old = reinterpret_cast<HFONT>(SelectObject(hdc, g_fontTitle));
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, g_accentColor);
        RECT r{ x, y, x + size, y + size };
        DrawTextW(hdc, text, 1, &r, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hdc, old);
    }
}
#define _CRT_SECURE_NO_WARNINGS
#include <d3d10_1.h>
#include <d3d11.h>
#include <dxgi.h>
#include <windows.h>
#include <filesystem>
#include <string>
#include <algorithm>
#include <vector>
#include <cmath>

#include "../imgui/imgui_internal.h"
#include "../misc/antiflash/antiflash.hpp"
#include "../misc/bomb/bomb_timer.hpp"
#include "../misc/radar/radar.hpp"
#include "../config/config.hpp"
#include "../toast/toast.hpp"
#include "menu.hpp"
#include "menu_style.hpp"
#include "ui_helpers.hpp"
#include "tab_aimbot.hpp"
#include "tab_visuals.hpp"
#include "tab_world.hpp"
#include "tab_misc.hpp"
#include "tab_config.hpp"
#include "tab_settings.hpp"
#include "../settings/watermark/watermark.hpp"

extern ID3D11Device* g_pd3dDevice;
ImTextureID g_logoTexture = (ImTextureID)0;

static std::string GetModuleDirectory() {
    HMODULE module = nullptr;
    if (!GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&GetModuleDirectory),
            &module)) {
        return {};
    }

    char path[MAX_PATH]{};
    const DWORD len = GetModuleFileNameA(module, path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) return {};

    std::string full(path, len);
    const size_t slash = full.find_last_of("\\/");
    return slash == std::string::npos ? std::string{} : full.substr(0, slash);
}

static bool LoadLogoTextureFromFile(const std::string& path) {
    if (!g_pd3dDevice || g_logoTexture || path.empty()) return false;

    FILE* f = nullptr;
    if (fopen_s(&f, path.c_str(), "rb") != 0 || !f) return false;

    BITMAPFILEHEADER bf{};
    BITMAPINFOHEADER bi{};
    const bool validHeader =
        fread(&bf, sizeof(bf), 1, f) == 1 &&
        fread(&bi, sizeof(bi), 1, f) == 1 &&
        bf.bfType == 0x4D42 &&
        bi.biPlanes == 1 &&
        bi.biBitCount == 32 &&
        bi.biCompression == BI_RGB;

    if (!validHeader) {
        fclose(f);
        return false;
    }

    const int width = bi.biWidth;
    const int height = bi.biHeight < 0 ? -bi.biHeight : bi.biHeight;
    if (width <= 0 || height <= 0 || width > 2048 || height > 2048) {
        fclose(f);
        return false;
    }

    const size_t pixelBytes = static_cast<size_t>(width) * static_cast<size_t>(height) * 4u;
    if (pixelBytes == 0 || pixelBytes > 32u * 1024u * 1024u) {
        fclose(f);
        return false;
    }

    if (fseek(f, static_cast<long>(bf.bfOffBits), SEEK_SET) != 0) {
        fclose(f);
        return false;
    }

    std::vector<unsigned char> pixels(pixelBytes);
    if (fread(pixels.data(), 1, pixelBytes, f) != pixelBytes) {
        fclose(f);
        return false;
    }
    fclose(f);

    std::vector<unsigned char> rgba(pixelBytes);
    for (int y = 0; y < height; ++y) {
        const int srcY = (bi.biHeight > 0) ? (height - 1 - y) : y;
        for (int x = 0; x < width; ++x) {
            const size_t src = (static_cast<size_t>(srcY) * width + x) * 4u;
            const size_t dst = (static_cast<size_t>(y) * width + x) * 4u;
            rgba[dst + 0] = pixels[src + 2];
            rgba[dst + 1] = pixels[src + 1];
            rgba[dst + 2] = pixels[src + 0];
            rgba[dst + 3] = pixels[src + 3];
        }
    }

    D3D11_TEXTURE2D_DESC desc{};
    desc.Width = static_cast<UINT>(width);
    desc.Height = static_cast<UINT>(height);
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

    D3D11_SUBRESOURCE_DATA init{};
    init.pSysMem = rgba.data();
    init.SysMemPitch = static_cast<UINT>(width * 4);

    ID3D11Texture2D* texture = nullptr;
    HRESULT hr = g_pd3dDevice->CreateTexture2D(&desc, &init, &texture);
    if (FAILED(hr) || !texture) return false;

    ID3D11ShaderResourceView* srv = nullptr;
    hr = g_pd3dDevice->CreateShaderResourceView(texture, nullptr, &srv);
    texture->Release();
    if (FAILED(hr) || !srv) return false;

    g_logoTexture = reinterpret_cast<ImTextureID>(srv);
    return true;
}

static void LoadLogoTexture() {
    if (!g_pd3dDevice || g_logoTexture) return;

    std::vector<std::string> candidates;
    const auto add = [&candidates](const std::string& p) {
        if (!p.empty()) candidates.push_back(p);
    };

    char cwd[MAX_PATH]{};
    const DWORD cwdLen = GetCurrentDirectoryA(MAX_PATH, cwd);
    if (cwdLen > 0 && cwdLen < MAX_PATH) {
        const std::string base(cwd, cwdLen);
        add(base + "\\nc_icon.bmp");
        add(base + "\\nc.bmp");
        add(base + "\\menu\\nc_icon.bmp");
        add(base + "\\menu\\nc.bmp");
    }

    char exe[MAX_PATH]{};
    const DWORD exeLen = GetModuleFileNameA(nullptr, exe, MAX_PATH);
    if (exeLen > 0 && exeLen < MAX_PATH) {
        std::string base(exe, exeLen);
        const size_t slash = base.find_last_of("\\/");
        if (slash != std::string::npos) {
            base.resize(slash);
            add(base + "\\nc_icon.bmp");
            add(base + "\\nc.bmp");
            add(base + "\\menu\\nc_icon.bmp");
            add(base + "\\menu\\nc.bmp");
        }
    }

    const std::string moduleDir = GetModuleDirectory();
    add(moduleDir + "\\nc_icon.bmp");
    add(moduleDir + "\\nc.bmp");
    add(moduleDir + "\\menu\\nc_icon.bmp");
    add(moduleDir + "\\menu\\nc.bmp");

    for (const std::string& path : candidates) {
        if (LoadLogoTextureFromFile(path)) return;
    }
}

float g_aimFov = 90.0f;
float g_aimSmooth = 5.0f;
int g_selectedBone = 0;
bool g_enableAim = false;
bool g_showAimFov = false;
float g_aimFovColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
int g_espTarget = 0;
EspSettings g_espSettings[3];
bool g_menuOpen = false;
bool g_exitApp = false;
float g_accentColor[4] = { 0.90f, 0.05f, 0.05f, 1.00f };
bool g_aimbotUseKeybind = false;
int g_aimbotKey = 0x46;
bool g_waitingForKey = false;
MenuTab g_activeTab = TAB_AIMBOT;
bool g_aimlockEnabled = false;
bool g_silentAimEnabled = false;
bool g_cheatReady = false;
float g_menuFadeAlpha = 0.0f;
bool g_menuClosing = false;
static bool g_menuFadeInProgress = false;
float g_aimPrediction = 0.08f;
float g_targetSwitchDelay = 200.0f;
bool g_prioritizeThreat = true;
float g_healthWeight = 2.0f;
float g_distanceWeight = 0.1f;
bool g_bombTimerEnabled = false;
bool g_antiFlashEnabled = false;
bool g_radarHackEnabled = false;
bool g_watermarkEnabled = false;

void InitSettings() {
    for (int target = 0; target < 3; ++target) {
        g_espSettings[target].box = false;
        g_espSettings[target].name = false;
        g_espSettings[target].lines = false;
        g_espSettings[target].health = false;
        g_espSettings[target].skeleton = false;
        g_espSettings[target].healthNumber = false;
        g_espSettings[target].money = false;
        g_espSettings[target].fillBox = false;
        g_espSettings[target].cornerBox = false;
        for (int i = 0; i < 4; ++i) {
            g_espSettings[target].boxColor[i] = 1.0f;
            g_espSettings[target].nameColor[i] = 1.0f;
            g_espSettings[target].linesColor[i] = 1.0f;
            g_espSettings[target].skeletonColor[i] = 1.0f;
            g_espSettings[target].healthNumberColor[i] = 1.0f;
            g_espSettings[target].moneyColor[i] = 1.0f;
            g_espSettings[target].cornerColor[i] = 1.0f;
        }
        g_espSettings[target].fillColor[0] = 0.0f;
        g_espSettings[target].fillColor[1] = 0.0f;
        g_espSettings[target].fillColor[2] = 0.0f;
        g_espSettings[target].fillColor[3] = 0.2549f;
    }
    LoadLogoTexture();
}

void Menu::Toggle() {
    if (!g_menuOpen) {
        g_menuOpen = true;
        g_menuClosing = false;
        g_menuFadeAlpha = 0.0f;
        g_menuFadeInProgress = true;
    } else if (!g_menuClosing) {
        g_menuClosing = true;
        g_menuFadeInProgress = false;
    }
}

static const char* TabLabel(int i) {
    static const char* labels[TAB_COUNT] = { "Aimbot", "Visuals", "World", "Misc", "Config", "Settings" };
    return labels[i];
}

static const char* TabDescription(int i) {
    static const char* descriptions[TAB_COUNT] = {
        "Targeting and precision controls",
        "Entity rendering and information",
        "World and environment options",
        "Quality-of-life utilities",
        "Profiles and configuration",
        "Interface and overlay preferences"
    };
    return descriptions[i];
}

static void DrawNavIcon(ImDrawList* dl, const ImVec2& p, int tab, ImU32 color) {
    const float x = p.x, y = p.y;
    if (tab == TAB_AIMBOT) {
        dl->AddCircle(ImVec2(x + 10, y + 10), 7.0f, color, 24, 1.6f);
        dl->AddCircleFilled(ImVec2(x + 10, y + 10), 2.4f, color);
        dl->AddLine(ImVec2(x + 10, y), ImVec2(x + 10, y + 4), color, 1.5f);
        dl->AddLine(ImVec2(x + 10, y + 16), ImVec2(x + 10, y + 20), color, 1.5f);
        dl->AddLine(ImVec2(x, y + 10), ImVec2(x + 4, y + 10), color, 1.5f);
        dl->AddLine(ImVec2(x + 16, y + 10), ImVec2(x + 20, y + 10), color, 1.5f);
    } else if (tab == TAB_VISUALS) {
        dl->AddRect(ImVec2(x + 2, y + 3), ImVec2(x + 18, y + 17), color, 3.0f, 0, 1.6f);
        dl->AddLine(ImVec2(x + 5, y + 14), ImVec2(x + 9, y + 10), color, 1.5f);
        dl->AddLine(ImVec2(x + 9, y + 10), ImVec2(x + 12, y + 12), color, 1.5f);
        dl->AddLine(ImVec2(x + 12, y + 12), ImVec2(x + 16, y + 7), color, 1.5f);
    } else if (tab == TAB_WORLD) {
        dl->AddCircle(ImVec2(x + 10, y + 10), 8.0f, color, 24, 1.5f);
        dl->AddLine(ImVec2(x + 2, y + 10), ImVec2(x + 18, y + 10), color, 1.2f);
        dl->AddLine(ImVec2(x + 10, y + 2), ImVec2(x + 10, y + 18), color, 1.2f);
    } else if (tab == TAB_MISC) {
        dl->AddRect(ImVec2(x + 4, y + 2), ImVec2(x + 16, y + 18), color, 2.0f, 0, 1.5f);
        dl->AddCircleFilled(ImVec2(x + 10, y + 6), 1.5f, color);
        dl->AddLine(ImVec2(x + 7, y + 11), ImVec2(x + 13, y + 11), color, 1.5f);
        dl->AddLine(ImVec2(x + 7, y + 14), ImVec2(x + 13, y + 14), color, 1.5f);
    } else if (tab == TAB_CONFIG) {
        dl->AddRect(ImVec2(x + 3, y + 4), ImVec2(x + 17, y + 16), color, 2.0f, 0, 1.5f);
        dl->AddLine(ImVec2(x + 6, y + 8), ImVec2(x + 14, y + 8), color, 1.3f);
        dl->AddLine(ImVec2(x + 6, y + 12), ImVec2(x + 12, y + 12), color, 1.3f);
    } else {
        dl->AddCircle(ImVec2(x + 10, y + 10), 7.0f, color, 24, 1.5f);
        dl->AddCircleFilled(ImVec2(x + 10, y + 10), 2.2f, color);
        for (int i = 0; i < 8; ++i) {
            const float a = (3.14159265f * 2.0f * i) / 8.0f;
            dl->AddLine(ImVec2(x + 10 + std::cos(a) * 8.5f, y + 10 + std::sin(a) * 8.5f),
                        ImVec2(x + 10 + std::cos(a) * 10.0f, y + 10 + std::sin(a) * 10.0f), color, 1.3f);
        }
    }
}

void Menu::Render() {
    const float speed = 8.0f;
    if (g_menuClosing) {
        g_menuFadeAlpha -= ImGui::GetIO().DeltaTime * speed;
        if (g_menuFadeAlpha <= 0.0f) {
            g_menuFadeAlpha = 0.0f;
            g_menuClosing = false;
            g_menuOpen = false;
            g_waitingForKey = false;
            return;
        }
    }
    if (!g_menuOpen) return;
    if (!g_logoTexture) LoadLogoTexture();
    if (g_menuFadeInProgress) {
        g_menuFadeAlpha += ImGui::GetIO().DeltaTime * speed;
        if (g_menuFadeAlpha >= 1.0f) {
            g_menuFadeAlpha = 1.0f;
            g_menuFadeInProgress = false;
        }
    }

    MenuStyle::ApplyDarkTheme();
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_CheckMark] = ImVec4(g_accentColor[0], g_accentColor[1], g_accentColor[2], 1.0f);
    style.Colors[ImGuiCol_SliderGrab] = ImVec4(g_accentColor[0], g_accentColor[1], g_accentColor[2], 0.88f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(g_accentColor[0], g_accentColor[1], g_accentColor[2], 1.0f);

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_menuFadeAlpha);
    ImGui::SetNextWindowSize(ImVec2(1120.0f, 720.0f), ImGuiCond_Always);
    ImGui::SetNextWindowPos(ImVec2(80.0f, 80.0f), ImGuiCond_FirstUseEver);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 12.0f);
    ImGui::Begin("##NexusClient", nullptr,
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
    ImGui::PopStyleVar(1);

    const ImVec2 windowPos = ImGui::GetWindowPos();
    const ImVec2 windowSize = ImGui::GetWindowSize();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    const ImVec4 accent = ImVec4(g_accentColor[0], g_accentColor[1], g_accentColor[2], 1.0f);
    const ImU32 accentCol = ImGui::ColorConvertFloat4ToU32(accent);
    const ImU32 borderCol = ImGui::GetColorU32(ImVec4(0.105f, 0.111f, 0.128f, 1.0f));
    const ImU32 mutedCol = ImGui::GetColorU32(ImVec4(0.52f, 0.54f, 0.58f, 1.0f));
    const ImU32 textCol = ImGui::GetColorU32(ImGuiCol_Text);

    dl->AddRectFilled(windowPos, ImVec2(windowPos.x + windowSize.x, windowPos.y + 1.5f), accentCol, 12.0f);

    const float sidebarW = 214.0f;
    const float contentX = sidebarW;
    dl->AddRectFilled(ImVec2(windowPos.x + sidebarW, windowPos.y),
                      ImVec2(windowPos.x + sidebarW + 1.0f, windowPos.y + windowSize.y), borderCol);

    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::BeginChild("##Sidebar", ImVec2(sidebarW, windowSize.y), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImGui::SetCursorPos(ImVec2(20, 20));
    if (g_logoTexture) {
        ImGui::Image(g_logoTexture, ImVec2(38, 38));
        ImGui::SameLine(0, 11);
    }
    ImGui::BeginGroup();
    ImGui::Text("NEXUS");
    ImGui::TextDisabled("CLIENT");
    ImGui::EndGroup();

    ImGui::SetCursorPos(ImVec2(18, 84));
    ImGui::TextDisabled("MENU");
    ImGui::SetCursorPos(ImVec2(16, 108));

    for (int i = 0; i < TAB_COUNT; ++i) {
        const bool selected = g_activeTab == static_cast<MenuTab>(i);
        ImGui::PushID(i);
        const ImVec2 p = ImGui::GetCursorScreenPos();
        const float w = ImGui::GetContentRegionAvail().x;
        const float h = 43.0f;
        if (ImGui::InvisibleButton("##tab", ImVec2(w, h)))
            g_activeTab = static_cast<MenuTab>(i);
        const bool hovered = ImGui::IsItemHovered();

        if (selected) {
            dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h),
                ImGui::ColorConvertFloat4ToU32(ImVec4(accent.x, accent.y, accent.z, 0.105f)), 8.0f);
            dl->AddRectFilled(ImVec2(p.x, p.y + 7), ImVec2(p.x + 3, p.y + h - 7), accentCol, 2.0f);
        } else if (hovered) {
            dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h),
                ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 0.025f)), 8.0f);
        }

        const ImU32 iconCol = selected ? accentCol : mutedCol;
        const ImU32 labelCol = selected ? textCol : ImGui::GetColorU32(ImVec4(0.66f, 0.68f, 0.72f, 1.0f));
        DrawNavIcon(dl, ImVec2(p.x + 16, p.y + 11), i, iconCol);
        dl->AddText(ImVec2(p.x + 49, p.y + 12), labelCol, TabLabel(i));
        ImGui::PopID();
    }

    const float footerY = windowSize.y - 76.0f;
    dl->AddLine(ImVec2(windowPos.x + 18, windowPos.y + footerY - 12),
                ImVec2(windowPos.x + sidebarW - 18, windowPos.y + footerY - 12), borderCol, 1.0f);
    ImGui::SetCursorPos(ImVec2(20, footerY));
    ImGui::TextDisabled("STATUS");
    ImGui::SameLine(0, 8);
    ImGui::TextColored(ImVec4(0.45f, 0.86f, 0.57f, 1.0f), "READY");
    ImGui::EndChild();

    ImGui::SameLine(0, 0);
    ImGui::BeginChild("##Content", ImVec2(0, windowSize.y), false,
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImGui::SetCursorPos(ImVec2(28, 23));
    ImGui::BeginGroup();
    ImGui::Text("%s", TabLabel(g_activeTab));
    ImGui::TextDisabled("%s", TabDescription(g_activeTab));
    ImGui::EndGroup();

    const float headerLineY = 77.0f;
    dl->AddLine(ImVec2(windowPos.x + contentX + 28, windowPos.y + headerLineY),
                ImVec2(windowPos.x + windowSize.x - 28, windowPos.y + headerLineY), borderCol, 1.0f);

    ImGui::SetCursorPos(ImVec2(28, 92));
    ImGui::BeginChild("##Page", ImVec2(0, 0), false);
    switch (g_activeTab) {
    case TAB_AIMBOT:   DrawAimbotTab(); break;
    case TAB_VISUALS:  DrawVisualsTab(); break;
    case TAB_WORLD:    DrawWorldTab(); break;
    case TAB_MISC:     DrawMiscTab(); break;
    case TAB_CONFIG:   DrawConfigTab(); break;
    case TAB_SETTINGS: DrawSettingsTab(); break;
    default:           g_activeTab = TAB_AIMBOT; DrawAimbotTab(); break;
    }
    ImGui::EndChild();
    ImGui::EndChild();
    ImGui::End();
    ImGui::PopStyleVar();
}

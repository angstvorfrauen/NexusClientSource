#include "settings/watermark/watermark.hpp"
#include "misc/antiflash/antiflash.hpp"
#include "misc/bomb/bomb_timer.hpp"
#include "offsets/client_dll.hpp"
#include "misc/radar/radar.hpp"
#include "offsets/offsets.hpp"
#include "config/config.hpp"
#include "aimbot/aimbot.hpp"
#include "visuals/esp.hpp"
#include "toast/toast.hpp"
#include "math/math.hpp"
#include "menu/menu.hpp"
#include "math/vec.hpp"
#include "console.hpp"
#include "memory.hpp"
#include "menu/menu_style.hpp"
#include <windows.h>
#include <dwmapi.h>
#include <d3d11.h>
#include <thread>

#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui.h"

ID3D11Device* g_pd3dDevice = nullptr;
ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
IDXGISwapChain* g_pSwapChain = nullptr;
ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
bool g_Running = true;
MemoryManager mem;
uintptr_t module_base = 0;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

void KeyHandler() {
    while (g_Running) {
        if (GetAsyncKeyState(VK_INSERT) & 1)
            Menu::Toggle();
        if (GetAsyncKeyState(VK_END) & 1)
            g_Running = false;
        if (g_exitApp)
            g_Running = false;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void HackGame() {
    int localTeam = 0;
    float screenW = (float)GetSystemMetrics(SM_CXSCREEN);
    float screenH = (float)GetSystemMetrics(SM_CYSCREEN);
    Matrix4x4 viewMatrix = mem.Read<Matrix4x4>(module_base + cs2_dumper::offsets::client_dll::dwViewMatrix);
    uintptr_t entityList = mem.Read<uintptr_t>(module_base + cs2_dumper::offsets::client_dll::dwEntityList);
    uintptr_t localPawn = mem.Read<uintptr_t>(module_base + cs2_dumper::offsets::client_dll::dwLocalPlayerPawn);
    if (localPawn) localTeam = mem.Read<int>(localPawn + cs2_dumper::schemas::client_dll::C_BaseEntity::m_iTeamNum);
    RunAimbot(localPawn, localTeam, entityList, viewMatrix, screenW, screenH, module_base, mem);
    RunEsp(localPawn, localTeam, entityList, viewMatrix, screenW, screenH, module_base, mem);
    UpdateBombTimer(mem, module_base);
    UpdateAntiFlash(mem, localPawn);
    UpdateRadarHack(mem, localPawn, localTeam, entityList);
}

int main() {
    Logger::Initialize("null");
    InitSettings();
    while (!mem.Attach(L"cs2.exe")) {
        std::cout << "[-] cs2.exe not found! (start the game to inject)...\n";
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
    std::cout << "[+] injection successful!\n";
    EnsureConfigDirectory();
    FullConfig defaultCfg;
    if (LoadConfig("C:\\nexusclient\\cfg\\default.cfg", defaultCfg)) { ApplyConfig(defaultCfg); }
    module_base = mem.GetModuleBaseAddress(L"client.dll");
    std::cout << "[+] found cs2.exe...\n";
    std::cout << "[+] client.dll base: 0x" << std::hex << module_base << std::endl;
    std::cout << "[+] https://t.me/rocketclient2\n";
    std::cout << "[+] https://discord.gg/j5CcQ5H7PY join for updates!\n";
    std::cout << "[+] version 0.0.8-beta\n";

    HINSTANCE hAppInstance = GetModuleHandle(NULL);
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, hAppInstance, nullptr, nullptr, nullptr, nullptr, L"ExternalMenuClass", nullptr };
    RegisterClassExW(&wc);
    HWND hwnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED,
        wc.lpszClassName, L"null",
        WS_POPUP,
        0, 0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        nullptr, nullptr, wc.hInstance, nullptr
    );

    std::thread titleThread([hwnd]() {
        while (g_Running) {
            std::string title = Logger::RandomTitle(32);
            SetConsoleTitleA(title.c_str());
            std::wstring wtitle(title.begin(), title.end());
            if (IsWindow(hwnd))
                SetWindowTextW(hwnd, wtitle.c_str());
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        });

    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 255, LWA_ALPHA);
    MARGINS margins = { -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    if (!CreateDeviceD3D(hwnd)) {
        g_Running = false;
        if (titleThread.joinable())
            titleThread.join();
        CleanupDeviceD3D();
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    MenuStyle::ApplyDarkTheme();

    io.Fonts->Clear();
    char windowsDir[MAX_PATH]{};
    GetWindowsDirectoryA(windowsDir, MAX_PATH);
    std::string fontPath = std::string(windowsDir) + "\\Fonts\\segoeuib.ttf";
    ImFontConfig fontConfig{};
    fontConfig.OversampleH = 3;
    fontConfig.OversampleV = 2;
    fontConfig.PixelSnapH = true;
    ImFont* mainFont = io.Fonts->AddFontFromFileTTF(fontPath.c_str(), 17.0f, &fontConfig);
    if (!mainFont) mainFont = io.Fonts->AddFontDefault();

    ImFontConfig pixelConfig{};
    pixelConfig.OversampleH = 1;
    pixelConfig.OversampleV = 1;
    pixelConfig.PixelSnapH = true;
    std::string pixelFontPath = std::string(windowsDir) + "\\Fonts\\consola.ttf";
    ImFont* pixelFont = io.Fonts->AddFontFromFileTTF(pixelFontPath.c_str(), 13.0f, &pixelConfig);
    if (!pixelFont) {
        pixelFont = io.Fonts->AddFontDefault(&pixelConfig);
        if (pixelFont) pixelFont->Scale = 0.8f;
    }

    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    static bool toastShown = false;
    if (!toastShown) {
        g_toasts.emplace_back("NexusClient successfully injected.", 3.0f);
        toastShown = true;
    }
    std::thread hotkeyThread(KeyHandler);

    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (g_Running) {
        while (PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) g_Running = false;
        }
        if (!g_Running) break;

        LONG style = GetWindowLong(hwnd, GWL_EXSTYLE);
        if (g_menuOpen || g_menuClosing) style &= ~WS_EX_TRANSPARENT;
        else style |= WS_EX_TRANSPARENT;
        SetWindowLong(hwnd, GWL_EXSTYLE, style);

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        HackGame();

        if (g_menuOpen || g_menuClosing) {
            ImDrawList* draw = ImGui::GetBackgroundDrawList();
            int alpha = (int)(150 * g_menuFadeAlpha);
            ImU32 dimColor = IM_COL32(0, 0, 0, alpha);
            draw->AddRectFilled(ImVec2(0, 0), ImGui::GetIO().DisplaySize, dimColor);
        }

        Menu::Render();
        RenderToasts();
        RenderBombTimer();
        RenderWatermark();
        const float clear_color_with_alpha[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        g_pSwapChain->Present(1, 0);
    }

    g_Running = false;
    if (hotkeyThread.joinable())
        hotkeyThread.join();
    if (titleThread.joinable())
        titleThread.join();

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    CleanupDeviceD3D();
    DestroyWindow(hwnd);
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}

bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags,
        featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
        &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED)
        res = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags,
            featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain,
            &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK) return false;
    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) return true;
    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_KEYMENU) return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}
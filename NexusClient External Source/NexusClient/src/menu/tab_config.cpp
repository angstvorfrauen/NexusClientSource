#include "tab_config.hpp"
#include "menu.hpp"
#include "ui_helpers.hpp"
#include "../config/config.hpp"
#include "../toast/toast.hpp"
#include <windows.h>
#include <filesystem>
#include <string>

static void EnsureConfigDirectory() {
    std::filesystem::create_directories("C:\\nexusclient\\cfg");
}

void DrawConfigTab() {
    static constexpr const char* CONFIG_DIRECTORY = "C:\\nexusclient\\cfg";
    EnsureConfigDirectory();
    static char configName[32] = "";

    UI::BeginCard("##ConfigManager", "Config manager", "profiles", ImVec2(0, 215));
    ImGui::TextDisabled("Profile name");
    ImGui::SetNextItemWidth(-1);
    ImGui::InputText("##ConfigName", configName, sizeof(configName));
    ImGui::Spacing();

    if (UI::AccentButton("Save", ImVec2(92, 32))) {
        if (!configName[0]) g_toasts.emplace_back("Enter a config name");
        else {
            char path[MAX_PATH]{};
            sprintf_s(path, "%s\\%s.cfg", CONFIG_DIRECTORY, configName);
            FullConfig cfg;
            LoadCurrentConfigFromGlobals(cfg);
            SaveConfig(path, cfg);
            g_toasts.emplace_back(("Saved config: " + std::string(configName)).c_str());
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Load", ImVec2(92, 32))) {
        if (!configName[0]) g_toasts.emplace_back("Enter a config name");
        else {
            char path[MAX_PATH]{};
            sprintf_s(path, "%s\\%s.cfg", CONFIG_DIRECTORY, configName);
            FullConfig cfg;
            if (LoadConfig(path, cfg)) {
                ApplyConfig(cfg);
                g_toasts.emplace_back(("Loaded config: " + std::string(configName)).c_str());
            } else g_toasts.emplace_back("Config not found");
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Delete", ImVec2(92, 32))) {
        if (!configName[0]) g_toasts.emplace_back("Enter a config name");
        else {
            char path[MAX_PATH]{};
            sprintf_s(path, "%s\\%s.cfg", CONFIG_DIRECTORY, configName);
            if (DeleteFileA(path)) g_toasts.emplace_back(("Deleted config: " + std::string(configName)).c_str());
            else g_toasts.emplace_back("Delete failed");
        }
    }
    UI::EndCard();

    UI::BeginCard("##SavedConfigs", "Saved configs", "click to select", ImVec2(0, 260));
    char searchPath[MAX_PATH]{};
    sprintf_s(searchPath, "%s\\*.cfg", CONFIG_DIRECTORY);
    WIN32_FIND_DATAA findData{};
    HANDLE hFind = FindFirstFileA(searchPath, &findData);
    if (hFind == INVALID_HANDLE_VALUE) {
        ImGui::TextDisabled("No configs found");
        UI::EndCard();
        return;
    }
    do {
        std::string name = findData.cFileName;
        if (name.size() > 4 && name.substr(name.size() - 4) == ".cfg") name.resize(name.size() - 4);
        ImGui::PushID(name.c_str());
        if (ImGui::Button(name.c_str(), ImVec2(180, 30))) strcpy_s(configName, sizeof(configName), name.c_str());
        ImGui::PopID();
    } while (FindNextFileA(hFind, &findData));
    FindClose(hFind);
    UI::EndCard();
}

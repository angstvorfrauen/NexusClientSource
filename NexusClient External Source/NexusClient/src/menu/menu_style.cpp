#include "menu_style.hpp"

namespace {
    static void Set(ImVec4& c, float r, float g, float b, float a = 1.0f) { c = ImVec4(r, g, b, a); }
}

void MenuStyle::ApplyDarkTheme() {
    ImGuiStyle& s = ImGui::GetStyle();
    ImVec4* c = s.Colors;
    const ImVec4 bg          = ImVec4(0.027f, 0.029f, 0.034f, 1.0f);
    const ImVec4 surface     = ImVec4(0.039f, 0.042f, 0.049f, 1.0f);
    const ImVec4 surface2    = ImVec4(0.052f, 0.055f, 0.064f, 1.0f);
    const ImVec4 field       = ImVec4(0.060f, 0.064f, 0.074f, 1.0f);
    const ImVec4 border      = ImVec4(0.105f, 0.111f, 0.128f, 0.95f);
    const ImVec4 text        = ImVec4(0.935f, 0.942f, 0.955f, 1.0f);
    const ImVec4 textMuted   = ImVec4(0.545f, 0.565f, 0.605f, 1.0f);
    const ImVec4 textSoft    = ImVec4(0.700f, 0.715f, 0.750f, 1.0f);

    s.WindowPadding = ImVec2(0.0f, 0.0f);
    s.WindowRounding = 12.0f;
    s.WindowBorderSize = 1.0f;
    s.ChildRounding = 10.0f;
    s.ChildBorderSize = 1.0f;
    s.FramePadding = ImVec2(10.0f, 8.0f);
    s.FrameRounding = 7.0f;
    s.ItemSpacing = ImVec2(10.0f, 9.0f);
    s.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    s.IndentSpacing = 18.0f;
    s.ScrollbarSize = 7.0f;
    s.ScrollbarRounding = 5.0f;
    s.GrabMinSize = 10.0f;
    s.GrabRounding = 5.0f;
    s.PopupRounding = 8.0f;
    s.PopupBorderSize = 1.0f;
    s.Alpha = 1.0f;
    s.AntiAliasedLines = true;
    s.AntiAliasedFill = true;

    Set(c[ImGuiCol_Text], text.x, text.y, text.z, text.w);
    Set(c[ImGuiCol_TextDisabled], textMuted.x, textMuted.y, textMuted.z, textMuted.w);
    Set(c[ImGuiCol_WindowBg], bg.x, bg.y, bg.z, bg.w);
    Set(c[ImGuiCol_ChildBg], surface.x, surface.y, surface.z, surface.w);
    Set(c[ImGuiCol_PopupBg], surface2.x, surface2.y, surface2.z, 0.99f);
    Set(c[ImGuiCol_Border], border.x, border.y, border.z, border.w);
    Set(c[ImGuiCol_BorderShadow], 0, 0, 0, 0);
    Set(c[ImGuiCol_FrameBg], field.x, field.y, field.z, 1.0f);
    Set(c[ImGuiCol_FrameBgHovered], 0.075f, 0.079f, 0.091f, 1.0f);
    Set(c[ImGuiCol_FrameBgActive], 0.086f, 0.091f, 0.104f, 1.0f);
    Set(c[ImGuiCol_TitleBg], bg.x, bg.y, bg.z, bg.w);
    Set(c[ImGuiCol_TitleBgActive], bg.x, bg.y, bg.z, bg.w);
    Set(c[ImGuiCol_MenuBarBg], bg.x, bg.y, bg.z, bg.w);
    Set(c[ImGuiCol_ScrollbarBg], bg.x, bg.y, bg.z, 1.0f);
    Set(c[ImGuiCol_ScrollbarGrab], 0.14f, 0.15f, 0.17f, 1.0f);
    Set(c[ImGuiCol_ScrollbarGrabHovered], 0.19f, 0.20f, 0.23f, 1.0f);
    Set(c[ImGuiCol_ScrollbarGrabActive], 0.23f, 0.24f, 0.28f, 1.0f);
    Set(c[ImGuiCol_Button], surface2.x, surface2.y, surface2.z, 1.0f);
    Set(c[ImGuiCol_ButtonHovered], 0.075f, 0.079f, 0.091f, 1.0f);
    Set(c[ImGuiCol_ButtonActive], 0.092f, 0.097f, 0.112f, 1.0f);
    Set(c[ImGuiCol_Header], surface2.x, surface2.y, surface2.z, 1.0f);
    Set(c[ImGuiCol_HeaderHovered], 0.075f, 0.079f, 0.091f, 1.0f);
    Set(c[ImGuiCol_HeaderActive], 0.092f, 0.097f, 0.112f, 1.0f);
    Set(c[ImGuiCol_Separator], border.x, border.y, border.z, border.w);
    Set(c[ImGuiCol_SeparatorHovered], 0.25f, 0.27f, 0.31f, 1.0f);
    Set(c[ImGuiCol_SeparatorActive], 0.30f, 0.32f, 0.36f, 1.0f);
    Set(c[ImGuiCol_CheckMark], 1.0f, 1.0f, 1.0f, 1.0f);
    Set(c[ImGuiCol_SliderGrab], 0.80f, 0.82f, 0.86f, 1.0f);
    Set(c[ImGuiCol_SliderGrabActive], 0.95f, 0.96f, 0.98f, 1.0f);
    Set(c[ImGuiCol_ResizeGrip], 0, 0, 0, 0);
    Set(c[ImGuiCol_ResizeGripHovered], 0, 0, 0, 0);
    Set(c[ImGuiCol_ResizeGripActive], 0, 0, 0, 0);

    (void)textSoft;
}

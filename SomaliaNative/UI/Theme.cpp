#include "Theme.h"

// Cor de destaque personalizada (Azul Bebê / Baby Blue: #89CFF0)
float accent_colour[4] = { 137.f / 255.f, 207.f / 255.f, 240.f / 255.f, 1.0f };
float content_animation = 0.0f;

namespace Theme
{
    ImVec4 AccentColor   = ImVec4(137.f / 255.f, 207.f / 255.f, 240.f / 255.f, 1.0f);
    ImVec4 BgSidebar     = ImVec4(32.f / 255.f, 32.f / 255.f, 32.f / 255.f, 1.0f);
    ImVec4 BgContent     = ImVec4(26.f / 255.f, 26.f / 255.f, 26.f / 255.f, 1.0f);
    ImVec4 BgUserCard    = ImVec4(41.f / 255.f, 41.f / 255.f, 41.f / 255.f, 1.0f);
    ImVec4 BorderColor   = ImVec4(50.f / 255.f, 50.f / 255.f, 50.f / 255.f, 1.0f);
    ImVec4 TextMuted     = ImVec4(105.f / 255.f, 105.f / 255.f, 105.f / 255.f, 1.0f);

    void ApplyStyle()
    {
        ImGui::StyleColorsDark();

        ImGuiStyle& style = ImGui::GetStyle();
        style.FramePadding = ImVec2(8, 5);
        style.ItemSpacing = ImVec2(8, 7);
        style.ItemInnerSpacing = ImVec2(7, 5);
        style.FrameRounding = 5.0f;
        style.WindowBorderSize = 0.0f;
        style.ScrollbarRounding = 6.0f;
        style.ScrollbarSize = 6.0f;
        style.WindowRounding = 12.0f;
        style.ChildRounding = 8.0f;
        style.PopupRounding = 7.0f;

        style.Colors[ImGuiCol_CheckMark]        = AccentColor;
        style.Colors[ImGuiCol_SliderGrab]       = AccentColor;
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(160.f / 255.f, 220.f / 255.f, 250.f / 255.f, 1.0f);
        style.Colors[ImGuiCol_Header]           = ImVec4(AccentColor.x, AccentColor.y, AccentColor.z, 0.35f);
        style.Colors[ImGuiCol_HeaderHovered]    = ImVec4(AccentColor.x, AccentColor.y, AccentColor.z, 0.55f);
        style.Colors[ImGuiCol_HeaderActive]     = ImVec4(AccentColor.x, AccentColor.y, AccentColor.z, 0.75f);
        style.Colors[ImGuiCol_FrameBg]          = ImVec4(20.f / 255.f, 21.f / 255.f, 24.f / 255.f, 1.0f);
        style.Colors[ImGuiCol_FrameBgHovered]   = ImVec4(28.f / 255.f, 32.f / 255.f, 36.f / 255.f, 1.0f);
        style.Colors[ImGuiCol_FrameBgActive]    = ImVec4(32.f / 255.f, 38.f / 255.f, 44.f / 255.f, 1.0f);
        style.Colors[ImGuiCol_Button]           = ImVec4(37.f / 255.f, 42.f / 255.f, 48.f / 255.f, 0.95f);
        style.Colors[ImGuiCol_ButtonHovered]    = ImVec4(AccentColor.x, AccentColor.y, AccentColor.z, 0.38f);
        style.Colors[ImGuiCol_ButtonActive]     = ImVec4(AccentColor.x, AccentColor.y, AccentColor.z, 0.65f);
        style.Colors[ImGuiCol_Border]           = BorderColor;
        style.Colors[ImGuiCol_Separator]        = ImVec4(AccentColor.x, AccentColor.y, AccentColor.z, 0.16f);
    }

    void SetAccentColor(float r, float g, float b, float a)
    {
        accent_colour[0] = r;
        accent_colour[1] = g;
        accent_colour[2] = b;
        accent_colour[3] = a;
        AccentColor = ImVec4(r, g, b, a);
        ApplyStyle();
    }
}


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
        style.FramePadding = ImVec2(1, 0);
        style.FrameRounding = 3.0f;
        style.WindowBorderSize = 0.0f;
        style.ScrollbarRounding = 3.0f;
        style.ScrollbarSize = 5.0f;
        style.WindowRounding = 10.0f;
        style.ChildRounding = 5.0f;
        style.PopupRounding = 4.0f;

        style.Colors[ImGuiCol_CheckMark]        = AccentColor;
        style.Colors[ImGuiCol_SliderGrab]       = AccentColor;
        style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(160.f / 255.f, 220.f / 255.f, 250.f / 255.f, 1.0f);
        style.Colors[ImGuiCol_Header]           = ImVec4(AccentColor.x, AccentColor.y, AccentColor.z, 0.35f);
        style.Colors[ImGuiCol_HeaderHovered]    = ImVec4(AccentColor.x, AccentColor.y, AccentColor.z, 0.55f);
        style.Colors[ImGuiCol_HeaderActive]     = ImVec4(AccentColor.x, AccentColor.y, AccentColor.z, 0.75f);
        style.Colors[ImGuiCol_ButtonActive]     = ImVec4(AccentColor.x, AccentColor.y, AccentColor.z, 0.65f);
    }
}

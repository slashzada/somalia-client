#pragma once
#include <d3d9.h>
#include "../Render/ImGui/imgui.h"

extern float accent_colour[4];

namespace Theme
{
    // Cores originais do Menu Phobia
    extern ImVec4 AccentColor;
    extern ImVec4 BgSidebar;
    extern ImVec4 BgContent;
    extern ImVec4 BgUserCard;
    extern ImVec4 BorderColor;
    extern ImVec4 TextMuted;

    void ApplyStyle();
    void SetAccentColor(float r, float g, float b, float a = 1.0f);
}

#pragma once
#include <d3d9.h>
#include "../Render/ImGui/imgui.h"

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
}

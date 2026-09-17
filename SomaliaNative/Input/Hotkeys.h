#pragma once
#include <windows.h>
#include <string>
#include "../Render/ImGui/imgui.h"

namespace Hotkeys
{
    // Retorna o nome amigavel da tecla ou botao do mouse
    const char* GetKeyName(int vkCode);

    // Widget Dear ImGui customizado para visualizacao e captura de keybinds
    bool KeybindButton(const char* str_id, int* pKey, const ImVec2& size = ImVec2(72, 20));

    // Processamento das hotkeys em tempo real fora do menu
    void Update();

    // Reseta buffers de debounce e estados pendentes
    void Reset();
}

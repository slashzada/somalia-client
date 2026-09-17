#include "Hotkeys.h"
#include "../Config/Config.h"
#include "../Core/Logger.h"
#include "../Engine/SAMP/SAMP.h"
#include "../Features/PlayerSlap/PlayerSlap.h"
#include "../Features/LuaSlide/LuaSlide.h"
#include "../UI/Theme.h"
#include <cstdio>
#include <cstring>

namespace Hotkeys
{
    static int* s_pWaitingKey = nullptr;
    static bool s_PrevKeyState[256] = { false };
    static ULONGLONG s_ListeningStartTick = 0;

    const char* GetKeyName(int vkCode)
    {
        if (vkCode <= 0) return "NONE";

        switch (vkCode)
        {
        case VK_LBUTTON:  return "M1";
        case VK_RBUTTON:  return "M2";
        case VK_MBUTTON:  return "M3";
        case VK_XBUTTON1: return "M4";
        case VK_XBUTTON2: return "M5";
        case VK_SPACE:    return "SPACE";
        case VK_SHIFT:
        case VK_LSHIFT:
        case VK_RSHIFT:   return "SHIFT";
        case VK_CONTROL:
        case VK_LCONTROL:
        case VK_RCONTROL: return "CTRL";
        case VK_MENU:
        case VK_LMENU:
        case VK_RMENU:    return "ALT";
        case VK_CAPITAL:  return "CAPS";
        case VK_TAB:      return "TAB";
        case VK_RETURN:   return "ENTER";
        case VK_BACK:     return "BACK";
        case VK_ESCAPE:   return "ESC";
        case VK_UP:       return "UP";
        case VK_DOWN:     return "DOWN";
        case VK_LEFT:     return "LEFT";
        case VK_RIGHT:    return "RIGHT";
        case VK_INSERT:   return "INS";
        case VK_DELETE:   return "DEL";
        case VK_HOME:     return "HOME";
        case VK_END:      return "END";
        case VK_PRIOR:    return "PGUP";
        case VK_NEXT:     return "PGDN";
        case VK_F1:       return "F1";
        case VK_F2:       return "F2";
        case VK_F3:       return "F3";
        case VK_F4:       return "F4";
        case VK_F5:       return "F5";
        case VK_F6:       return "F6";
        case VK_F7:       return "F7";
        case VK_F8:       return "F8";
        case VK_F9:       return "F9";
        case VK_F10:      return "F10";
        case VK_F11:      return "F11";
        case VK_F12:      return "F12";
        case VK_NUMPAD0:  return "NUM0";
        case VK_NUMPAD1:  return "NUM1";
        case VK_NUMPAD2:  return "NUM2";
        case VK_NUMPAD3:  return "NUM3";
        case VK_NUMPAD4:  return "NUM4";
        case VK_NUMPAD5:  return "NUM5";
        case VK_NUMPAD6:  return "NUM6";
        case VK_NUMPAD7:  return "NUM7";
        case VK_NUMPAD8:  return "NUM8";
        case VK_NUMPAD9:  return "NUM9";
        default:
            break;
        }

        if (vkCode >= 'A' && vkCode <= 'Z')
        {
            static char letterBuf[2] = { 0, 0 };
            letterBuf[0] = static_cast<char>(vkCode);
            return letterBuf;
        }
        if (vkCode >= '0' && vkCode <= '9')
        {
            static char digitBuf[2] = { 0, 0 };
            digitBuf[0] = static_cast<char>(vkCode);
            return digitBuf;
        }

        static char keyBuf[32];
        UINT scanCode = MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC);
        if (scanCode != 0)
        {
            LONG lParam = scanCode << 16;
            if (GetKeyNameTextA(lParam, keyBuf, sizeof(keyBuf)) > 0)
            {
                return keyBuf;
            }
        }

        snprintf(keyBuf, sizeof(keyBuf), "KEY_%d", vkCode);
        return keyBuf;
    }

    bool KeybindButton(const char* str_id, int* pKey, const ImVec2& size)
    {
        if (!pKey) return false;

        bool isWaiting = (s_pWaitingKey == pKey);
        char btnLabel[64];

        if (isWaiting)
        {
            snprintf(btnLabel, sizeof(btnLabel), "[ ... ]##%s", str_id);
        }
        else
        {
            snprintf(btnLabel, sizeof(btnLabel), "[ %s ]##%s", GetKeyName(*pKey), str_id);
        }

        // Estilização com cores de tema do Somalia
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 4.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

        if (isWaiting)
        {
            ImGui::PushStyleColor(ImGuiCol_Border, Theme::AccentColor);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(Theme::AccentColor.x * 0.3f, Theme::AccentColor.y * 0.3f, Theme::AccentColor.z * 0.3f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_Text, Theme::AccentColor);
        }
        else if (*pKey != 0)
        {
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.35f, 0.45f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.14f, 0.18f, 0.9f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 0.95f));
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.22f, 0.26f, 0.6f));
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.09f, 0.10f, 0.12f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 0.8f));
        }

        bool clicked = ImGui::Button(btnLabel, size);

        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Clique com botao esquerdo para definir tecla.\nClique com botao direito para remover tecla.");
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Right))
            {
                *pKey = 0;
                if (s_pWaitingKey == pKey) s_pWaitingKey = nullptr;
                PlayerSlap::ShowToast("[KEYBIND] Tecla desvinculada (NONE)", 0xFFCCCCCC, 2000);
            }
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);

        if (clicked)
        {
            if (isWaiting)
            {
                s_pWaitingKey = nullptr;
            }
            else
            {
                s_pWaitingKey = pKey;
                s_ListeningStartTick = GetTickCount64();
            }
        }

        // Lógica de captura quando em modo de escuta
        if (isWaiting)
        {
            // Debounce inicial de 100ms para ignorar o clique que ativou o botão
            if (GetTickCount64() - s_ListeningStartTick > 100)
            {
                // 1. Tecla ESC cancela / desassocia
                if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
                {
                    *pKey = 0;
                    s_pWaitingKey = nullptr;
                    PlayerSlap::ShowToast("[KEYBIND] Tecla limpa (NONE)", 0xFFCCCCCC, 2000);
                    return true;
                }

                // 2. Detecção de cliques de mouse adicionais (M3, M4, M5)
                if (GetAsyncKeyState(VK_MBUTTON) & 0x8000)
                {
                    *pKey = VK_MBUTTON;
                    s_pWaitingKey = nullptr;
                    PlayerSlap::ShowToast("[KEYBIND] Vinculado a [ M3 ]", 0xFF00FF88, 2500);
                    return true;
                }
                if (GetAsyncKeyState(VK_XBUTTON1) & 0x8000)
                {
                    *pKey = VK_XBUTTON1;
                    s_pWaitingKey = nullptr;
                    PlayerSlap::ShowToast("[KEYBIND] Vinculado a [ M4 ]", 0xFF00FF88, 2500);
                    return true;
                }
                if (GetAsyncKeyState(VK_XBUTTON2) & 0x8000)
                {
                    *pKey = VK_XBUTTON2;
                    s_pWaitingKey = nullptr;
                    PlayerSlap::ShowToast("[KEYBIND] Vinculado a [ M5 ]", 0xFF00FF88, 2500);
                    return true;
                }

                // 3. Varredura de teclas do teclado (exceto teclas de controle de menu)
                for (int vk = 1; vk < 256; ++vk)
                {
                    if (vk == VK_ESCAPE || vk == VK_F5 || vk == VK_INSERT || vk == VK_LBUTTON)
                        continue;

                    if ((GetAsyncKeyState(vk) & 0x8000) != 0)
                    {
                        *pKey = vk;
                        s_pWaitingKey = nullptr;
                        char msg[64];
                        snprintf(msg, sizeof(msg), "[KEYBIND] Vinculado a [ %s ]", GetKeyName(vk));
                        PlayerSlap::ShowToast(msg, 0xFF00FF88, 2500);
                        return true;
                    }
                }
            }
        }

        return clicked;
    }

    void Reset()
    {
        s_pWaitingKey = nullptr;
        for (int i = 0; i < 256; ++i)
        {
            s_PrevKeyState[i] = false;
        }
    }

    void Update()
    {
        // Suspende hotkeys quando o menu do Somalia ou o chat/diálogo do SA-MP estiver aberto
        if (g_MenuState.menuOpen || SAMP::HasActiveCursor())
        {
            for (int i = 0; i < 256; ++i)
            {
                s_PrevKeyState[i] = false;
            }
            return;
        }

        auto CheckTrigger = [&](int vk) -> bool
        {
            if (vk <= 0 || vk >= 256) return false;
            bool isDown = (GetAsyncKeyState(vk) & 0x8000) != 0;
            bool triggered = (isDown && !s_PrevKeyState[vk]);
            s_PrevKeyState[vk] = isDown;
            return triggered;
        };

        // 1. Auto Slide (Lua Slide)
        if (CheckTrigger(g_MenuState.hotkeys.autoSlideKey))
        {
            g_MenuState.luaSlide.enabled = !g_MenuState.luaSlide.enabled;
            LuaSlide::ToggleFromMenu();
            if (g_MenuState.luaSlide.enabled)
                PlayerSlap::ShowToast("[Auto Slide] ATIVADO (ON)", 0xFF00FF88, 3000);
            else
                PlayerSlap::ShowToast("[Auto Slide] DESATIVADO (OFF)", 0xFFFF4444, 3000);
        }

        // 2. KFC Slide
        if (CheckTrigger(g_MenuState.hotkeys.kfcSlideKey))
        {
            g_MenuState.kfcSlide.enabled = !g_MenuState.kfcSlide.enabled;
            if (g_MenuState.kfcSlide.enabled)
                PlayerSlap::ShowToast("[KFC Slide] ATIVADO (ON)", 0xFF00FF88, 3000);
            else
                PlayerSlap::ShowToast("[KFC Slide] DESATIVADO (OFF)", 0xFFFF4444, 3000);
        }

        // 3. Fast Switch
        if (CheckTrigger(g_MenuState.hotkeys.fastSwitchKey))
        {
            g_MenuState.fastSwitch.enabled = !g_MenuState.fastSwitch.enabled;
            if (g_MenuState.fastSwitch.enabled)
                PlayerSlap::ShowToast("[Fast Switch] ATIVADO (ON)", 0xFF00FF88, 3000);
            else
                PlayerSlap::ShowToast("[Fast Switch] DESATIVADO (OFF)", 0xFFFF4444, 3000);
        }

        // 4. Auto Punch
        if (CheckTrigger(g_MenuState.hotkeys.autoPunchKey))
        {
            g_MenuState.autoPunch.enabled = !g_MenuState.autoPunch.enabled;
            if (g_MenuState.autoPunch.enabled)
                PlayerSlap::ShowToast("[Auto Punch] ATIVADO (ON)", 0xFF00FF88, 3000);
            else
                PlayerSlap::ShowToast("[Auto Punch] DESATIVADO (OFF)", 0xFFFF4444, 3000);
        }

        // 5. Silent Aim
        if (CheckTrigger(g_MenuState.hotkeys.silentAimKey))
        {
            g_MenuState.silentAim.enabled = !g_MenuState.silentAim.enabled;
            if (g_MenuState.silentAim.enabled)
                PlayerSlap::ShowToast("[Silent Aim] ATIVADO (ON)", 0xFF00FF88, 3000);
            else
                PlayerSlap::ShowToast("[Silent Aim] DESATIVADO (OFF)", 0xFFFF4444, 3000);
        }

        // 6. Legit Bot
        if (CheckTrigger(g_MenuState.hotkeys.legitBotKey))
        {
            g_MenuState.legitBot.enabled = !g_MenuState.legitBot.enabled;
            if (g_MenuState.legitBot.enabled)
                PlayerSlap::ShowToast("[Legit Bot] ATIVADO (ON)", 0xFF00FF88, 3000);
            else
                PlayerSlap::ShowToast("[Legit Bot] DESATIVADO (OFF)", 0xFFFF4444, 3000);
        }

        // 7. Anti-Aim
        if (CheckTrigger(g_MenuState.hotkeys.antiAimKey))
        {
            g_MenuState.antiAim.enabled = !g_MenuState.antiAim.enabled;
            if (g_MenuState.antiAim.enabled)
                PlayerSlap::ShowToast("[Anti-Aim] ATIVADO (ON)", 0xFF00FF88, 3000);
            else
                PlayerSlap::ShowToast("[Anti-Aim] DESATIVADO (OFF)", 0xFFFF4444, 3000);
        }

        // 8. Anti-HS (Headshot Proof)
        if (CheckTrigger(g_MenuState.hotkeys.antiHSKey))
        {
            g_MenuState.player.antiHS = !g_MenuState.player.antiHS;
            if (g_MenuState.player.antiHS)
                PlayerSlap::ShowToast("[Anti-HS] ATIVADO (ON)", 0xFF00FF88, 3000);
            else
                PlayerSlap::ShowToast("[Anti-HS] DESATIVADO (OFF)", 0xFFFF4444, 3000);
        }

        // 9. Godmode
        if (CheckTrigger(g_MenuState.hotkeys.godmodeKey))
        {
            g_MenuState.player.godmode = !g_MenuState.player.godmode;
            if (g_MenuState.player.godmode)
                PlayerSlap::ShowToast("[Godmode] ATIVADO (ON)", 0xFF00FF88, 3000);
            else
                PlayerSlap::ShowToast("[Godmode] DESATIVADO (OFF)", 0xFFFF4444, 3000);
        }

        // 10. Player Slap Hotkey
        if (CheckTrigger(g_MenuState.playerSlap.hotkey))
        {
            g_MenuState.playerSlap.enabled = !g_MenuState.playerSlap.enabled;
            if (g_MenuState.playerSlap.enabled)
                PlayerSlap::ShowToast("[Player Slap] ATIVADO (ON)", 0xFF00FF88, 3000);
            else
                PlayerSlap::ShowToast("[Player Slap] DESATIVADO (OFF)", 0xFFFF4444, 3000);
        }
    }
}

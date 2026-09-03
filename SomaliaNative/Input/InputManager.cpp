#include "InputManager.h"
#include <atomic>
#include "../Config/Config.h"
#include "../Core/Logger.h"
#include "../Core/Main.h"
#include "../Engine/SAMP/SAMP.h"
#include "../Render/ImGui/imgui.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace InputManager
{
    static HWND s_hWnd = NULL;
    static WNDPROC s_oWndProc = NULL;
    static ULONGLONG s_lastToggleTick = 0;

    void SetMenuCursorState(bool enabled)
    {
        ImGuiIO& io = ImGui::GetIO();
        if (enabled)
        {
            // 1. Garante que ImGui não altere o cursor Win32 por baixo dos panos
            io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

            // 2. Notifica o SA-MP para destravar o cursor e pausar recentering
            SAMP::ToggleCursor(true);

            // 3. Libera qualquer restrição física de mouse ou captura ativa no Windows
            ReleaseCapture();
            ClipCursor(NULL);

            // 4. Ativa o cursor desenhado pelo Dear ImGui
            io.MouseDrawCursor = true;

            Logger::Log("[SOMALIA][MENU] opened=1 cursor=MENU");
        }
        else
        {
            // 1. Desativa cursor e retenção de inputs do Dear ImGui
            io.MouseDrawCursor = false;
            io.WantCaptureMouse = false;
            io.WantCaptureKeyboard = false;
            io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

            // 2. Libera qualquer captura pendente do mouse
            ReleaseCapture();
            ClipCursor(NULL);

            // 3. Notifica o SA-MP para restaurar o controle de mouse e câmera nativa do GTA
            SAMP::ToggleCursor(false);

            // 4. Oculta o cursor nativo do Windows para o modo jogo
            ::SetCursor(NULL);

            Logger::Log("[SOMALIA][MENU] opened=0 cursor=GAME");
        }
    }

    void ToggleMenu(bool open)
    {
        g_MenuState.menuOpen = open;
        SetMenuCursorState(open);
    }

    LRESULT CALLBACK hkWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        Main::CallbackGuard guard;

        if (!guard.IsActive())
        {
            if (s_oWndProc)
                return CallWindowProcA(s_oWndProc, hWnd, uMsg, wParam, lParam);
            return DefWindowProcA(hWnd, uMsg, wParam, lParam);
        }

        // 1. Detecção da tecla de pânico (VK_END) para desinjetar instantaneamente
        if ((uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) && wParam == VK_END)
        {
            Main::RequestUnload();
            return 0;
        }

        // 2. Detecção de abertura/fechamento do menu (F5 ou INSERT) com debounce de 250ms
        if ((uMsg == WM_KEYDOWN || uMsg == WM_SYSKEYDOWN) && (wParam == VK_F5 || wParam == VK_INSERT))
        {
            ULONGLONG currentTick = GetTickCount64();
            if (currentTick - s_lastToggleTick > 250)
            {
                s_lastToggleTick = currentTick;
                ToggleMenu(!g_MenuState.menuOpen);
            }
            return 0; // Consome o evento da tecla de menu
        }

        // 2. Tratamento de foco e Alt+Tab
        if (uMsg == WM_SETFOCUS || uMsg == WM_ACTIVATE)
        {
            if (g_MenuState.menuOpen && (uMsg == WM_SETFOCUS || LOWORD(wParam) != WA_INACTIVE))
            {
                SetMenuCursorState(true);
            }
            else if (!g_MenuState.menuOpen)
            {
                SetMenuCursorState(false);
            }
        }

        // 3. Quando o menu está fechado, garante que o cursor permaneça oculto no jogo (a menos que o SA-MP esteja com chat/diálogo aberto)
        if (!g_MenuState.menuOpen && uMsg == WM_SETCURSOR)
        {
            if (!SAMP::HasActiveCursor())
            {
                ::SetCursor(NULL);
                return TRUE;
            }
        }

        // 4. Encaminhamento para o ImGui apenas quando o menu estiver aberto
        if (g_MenuState.menuOpen)
        {
            // Repassa mensagem para o backend Win32 do ImGui
            if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
                return 1;

            // Bloqueia cliques de mouse para que o GTA não atire nem sofra ações enquanto o usuário interage com o ImGui
            if (uMsg >= WM_MOUSEFIRST && uMsg <= WM_MOUSELAST)
            {
                if (uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONUP ||
                    uMsg == WM_RBUTTONDOWN || uMsg == WM_RBUTTONUP ||
                    uMsg == WM_MBUTTONDOWN || uMsg == WM_MBUTTONUP ||
                    uMsg == WM_MOUSEWHEEL)
                {
                    return 1;
                }
            }

            // Bloqueia teclas de jogo enquanto menu está aberto (exceto F5)
            if (uMsg == WM_KEYDOWN || uMsg == WM_KEYUP || uMsg == WM_CHAR)
            {
                if (wParam != VK_F5)
                    return 1;
            }
        }

        // 5. Encaminha para o WndProc original do GTA / SA-MP quando o menu está fechado ou mensagem não foi consumida
        if (s_oWndProc)
            return CallWindowProcA(s_oWndProc, hWnd, uMsg, wParam, lParam);

        return DefWindowProcA(hWnd, uMsg, wParam, lParam);
    }

    void Initialize(HWND hWnd)
    {
        if (!hWnd || !IsWindow(hWnd))
            return;

        s_hWnd = hWnd;
        s_oWndProc = (WNDPROC)SetWindowLongPtrA(s_hWnd, GWLP_WNDPROC, (LONG_PTR)hkWndProc);
        Logger::Log("WndProc subclassed com sucesso na janela 0x%p", (void*)s_hWnd);
    }

    void RestoreWndProc()
    {
        static std::atomic<bool> s_WndProcRestored(false);
        if (s_WndProcRestored.exchange(true))
            return;

        if (s_hWnd && s_oWndProc)
        {
            SetWindowLongPtrA(s_hWnd, GWLP_WNDPROC, (LONG_PTR)s_oWndProc);
            s_hWnd = NULL;
            Logger::Log("WndProc restaurado.");
            // Preserva s_oWndProc para fallback seguro em caso de repasse defensivo
        }
    }

    void Shutdown()
    {
        if (g_MenuState.menuOpen)
        {
            ToggleMenu(false);
        }

        RestoreWndProc();
    }

    bool IsMenuOpen()
    {
        return g_MenuState.menuOpen;
    }
}

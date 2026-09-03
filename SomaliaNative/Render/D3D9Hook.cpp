#include "D3D9Hook.h"
#include "../Engine/GTA/GTA.h"
#include "../Engine/SAMP/SAMP.h"
#include "../Core/Logger.h"
#include "../Core/Main.h"
#include "../Core/RuntimeState.h"
#include <atomic>
#include "../UI/Menu.h"
#include "../UI/Theme.h"
#include "../Input/InputManager.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx9.h"
#include "ImGui/imgui_impl_win32.h"
#include "../Features/Visuals/ESP.h"
#include "../Features/Aimbot/Aimbot.h"
#include "../Features/Aimbot/AimAssist.h"
#include "../Features/Aimbot/RageBot.h"
#include "../Features/LocalMods/LocalMods.h"
#include "../Features/Slide/Slide.h"
#include "../Features/AntiAim/AntiAim.h"

namespace D3D9Hook
{
    typedef HRESULT(__stdcall* tPresent)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
    typedef HRESULT(__stdcall* tReset)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);

    static tPresent s_oPresent = nullptr;
    static tReset   s_oReset   = nullptr;
    static void**   s_pVTable  = nullptr;
    static bool     s_bImGuiInitialized = false;

    static HRESULT __stdcall hkReset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
    {
        if (s_bImGuiInitialized)
        {
            ImGui_ImplDX9_InvalidateDeviceObjects();
            Menu::InvalidateDeviceObjects();
        }

        HRESULT hr = s_oReset(pDevice, pPresentationParameters);

        if (SUCCEEDED(hr))
        {
            if (s_bImGuiInitialized)
            {
                ImGui_ImplDX9_CreateDeviceObjects();
                Menu::CreateDeviceObjects(pDevice);
            }
        }

        return hr;
    }

    static HRESULT __stdcall hkPresent(IDirect3DDevice9* pDevice, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion)
    {
        if (!pDevice)
            return s_oPresent(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);

        if (Main::IsShuttingDown())
        {
            tPresent oPres = s_oPresent;
            Shutdown();
            return (oPres ? oPres(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion) : D3D_OK);
        }

        // 0. Atualiza ciclo de vida e estado do jogador
        RuntimeState::Update();

        if (!s_bImGuiInitialized)
        {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO();
            io.IniFilename = NULL; // Não criar arquivos ini avulsos
            io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange; // ImGui não deve interferir no cursor nativo do Win32

            Theme::ApplyStyle();

            HWND hWnd = GTA::GetWindowHandle();
            ImGui_ImplWin32_Init(hWnd);
            ImGui_ImplDX9_Init(pDevice);

            Menu::Initialize(pDevice);
            InputManager::Initialize(hWnd);

            s_bImGuiInitialized = true;
            Logger::Log("ImGui context criado");
            Logger::Log("DX9 backend inicializado");
            Logger::Log("Win32 backend inicializado");
            Logger::Log("Menu inicializado");
            Logger::Log("Input inicializado");
            Logger::Log("SomaliaNative pronta");
        }

        // Render Loop do ImGui
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 1. Renderiza os elementos visuais in-game (ESP e FOV Circle)
        ESP::Render();

        // 2. Renderiza o Legit Bot (Target Indicator e selecao suave)
        Aimbot::Render();

        // 3. Renderiza o Ragebot completamente independente (indicadores, fov crimson e vetor de agressividade)
        RageBot::Render();

        // 4. Processa os modificadores locais de motor do GTA (Player, Veículo, Ambiente)
        LocalMods::Update();

        // 5. Processa a mecânica de C-Slide e Quick Switch
        Slide::Update();

        // 5.1 Processa Anti-Aim e Fake Lag
        AntiAim::Update();

        // 6. Renderiza a interface Somalia (quando aberta)
        Menu::Render();

        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

        return s_oPresent(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
    }

    bool Initialize()
    {
        IDirect3DDevice9* pDevice = GTA::GetD3DDevice();
        if (!pDevice)
        {
            Logger::Log("Erro: Dispositivo D3D9 nao localizado.");
            return false;
        }

        s_pVTable = *reinterpret_cast<void***>(pDevice);
        if (!s_pVTable)
        {
            Logger::Log("Erro: VTable do D3D9 invalida.");
            return false;
        }

        DWORD oldProtect;

        // Hook Present (Index 17)
        VirtualProtect(&s_pVTable[17], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
        s_oPresent = reinterpret_cast<tPresent>(s_pVTable[17]);
        s_pVTable[17] = reinterpret_cast<void*>(hkPresent);
        VirtualProtect(&s_pVTable[17], sizeof(void*), oldProtect, &oldProtect);

        // Hook Reset (Index 16)
        VirtualProtect(&s_pVTable[16], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
        s_oReset = reinterpret_cast<tReset>(s_pVTable[16]);
        s_pVTable[16] = reinterpret_cast<void*>(hkReset);
        VirtualProtect(&s_pVTable[16], sizeof(void*), oldProtect, &oldProtect);

        Logger::Log("D3D9 VTable hooks instalados (Present: idx 17, Reset: idx 16).");
        return true;
    }

    void Shutdown()
    {
        static std::atomic<bool> s_AlreadyShutdown(false);
        if (s_AlreadyShutdown.exchange(true)) return;

        Logger::Log("[SOMALIA][UNLOAD] FEATURES_STOPPED");

        // 1. Reseta e limpa todos os módulos
        LocalMods::Reset();
        Slide::Reset();
        AntiAim::Reset();
        AimAssist::Reset();
        Aimbot::ClearTarget();
        RageBot::Reset();
        SAMP::Shutdown();

        // 2. Restaura o WndProc original e o cursor nativo
        InputManager::Shutdown();
        Logger::Log("[SOMALIA][UNLOAD] WNDPROC_RESTORED");

        // 3. Restaura os ponteiros originais da VTable do D3D9
        if (s_pVTable)
        {
            DWORD oldProtect;

            if (s_oPresent)
            {
                VirtualProtect(&s_pVTable[17], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
                s_pVTable[17] = reinterpret_cast<void*>(s_oPresent);
                VirtualProtect(&s_pVTable[17], sizeof(void*), oldProtect, &oldProtect);
                s_oPresent = nullptr;
            }

            if (s_oReset)
            {
                VirtualProtect(&s_pVTable[16], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
                s_pVTable[16] = reinterpret_cast<void*>(s_oReset);
                VirtualProtect(&s_pVTable[16], sizeof(void*), oldProtect, &oldProtect);
                s_oReset = nullptr;
            }

            s_pVTable = nullptr;
            Logger::Log("[SOMALIA][UNLOAD] HOOKS_REMOVED");
        }

        // 4. Destrói backends e contexto ImGui
        if (s_bImGuiInitialized)
        {
            Menu::Shutdown();
            ImGui_ImplDX9_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            s_bImGuiInitialized = false;
            Logger::Log("[SOMALIA][UNLOAD] IMGUI_SHUTDOWN");
        }

        Logger::Log("[SOMALIA][UNLOAD] THREADS_STOPPED");
        Logger::Log("[SOMALIA][UNLOAD] COMPLETE");

        // 5. Inicia thread externa descolada para ejetar a DLL da memória com segurança
        CreateThread(NULL, 0, [](LPVOID) -> DWORD {
            Sleep(150);
            Logger::Shutdown();
            FreeLibraryAndExitThread(Main::GetModuleInstance(), 0);
            return 0;
        }, NULL, 0, NULL);
    }
}

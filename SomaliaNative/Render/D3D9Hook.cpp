#include "D3D9Hook.h"
#include "../Engine/GTA/GTA.h"
#include "../Engine/SAMP/SAMP.h"
#include "../Core/Logger.h"
#include "../Core/Main.h"
#include "../Core/RuntimeState.h"
#include "../Core/StreamProof.h"
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
#include "../Features/SilentAim/SilentAim.h"
#include "../Features/LocalMods/LocalMods.h"
#include "../Features/KFCSlide/KFCSlide.h"
#include "../Features/AutoSlide/AutoSlide.h"
#include "../Features/FistSwitch/FistSwitch.h"
#include "../Features/AutoPunch/AutoPunch.h"
#include "../Features/AntiAim/AntiAim.h"
#include "../Features/Aimbot/TriggerBot.h"
#include "../Features/PlayerSlap/PlayerSlap.h"

namespace D3D9Hook
{
    typedef HRESULT(__stdcall* tPresent)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
    typedef HRESULT(__stdcall* tReset)(IDirect3DDevice9*, D3DPRESENT_PARAMETERS*);

    static tPresent s_oPresent = nullptr;
    static tReset   s_oReset   = nullptr;
    static void**   s_pVTable  = nullptr;
    static bool     s_bImGuiInitialized = false;
    static std::atomic<bool> s_HooksRestored(false);
    static std::atomic<bool> s_UIDestroyed(false);
    static bool     s_bStreamProofInitialized = false;
    static bool     s_bLastStreamProofState = false;

    static HRESULT __stdcall hkReset(IDirect3DDevice9* pDevice, D3DPRESENT_PARAMETERS* pPresentationParameters)
    {
        Main::CallbackGuard guard;

        if (!guard.IsActive())
        {
            return s_oReset ? s_oReset(pDevice, pPresentationParameters) : D3D_OK;
        }

        if (s_bImGuiInitialized)
        {
            ImGui_ImplDX9_InvalidateDeviceObjects();
            Menu::InvalidateDeviceObjects();
        }

        HRESULT hr = s_oReset ? s_oReset(pDevice, pPresentationParameters) : D3D_OK;

        if (SUCCEEDED(hr))
        {
            if (s_bImGuiInitialized && !Main::IsShuttingDown())
            {
                ImGui_ImplDX9_CreateDeviceObjects();
                Menu::CreateDeviceObjects(pDevice);
            }
        }

        return hr;
    }

    static HRESULT __stdcall hkPresent(IDirect3DDevice9* pDevice, const RECT* pSourceRect, const RECT* pDestRect, HWND hDestWindowOverride, const RGNDATA* pDirtyRegion)
    {
        Main::CallbackGuard guard;

        if (!pDevice || !guard.IsActive())
        {
            if (Main::IsStopRequested())
            {
                Main::BeginShutdown();
            }
            return s_oPresent ? s_oPresent(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion) : D3D_OK;
        }

        // Se o dispositivo estiver perdido, apenas repassa sem desenhar ImGui
        if (pDevice->TestCooperativeLevel() != D3D_OK)
        {
            return s_oPresent ? s_oPresent(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion) : D3D_OK;
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

            // 5.5 Inicializa os modulos nativos de gameplay
            KFCSlide::Initialize();
            AutoSlide::Initialize();
            FistSwitch::Initialize();
            AutoPunch::Initialize();
            AntiAim::Initialize();
            TriggerBot::Initialize();
            SilentAim::Initialize();
            Logger::Log("Modulos nativos de gameplay inicializados (KFC, AutoSlide, FistSwitch, AutoPunch, AntiAim, TriggerBot, SilentAim).");

            StreamProof::Initialize(hWnd);
            s_bStreamProofInitialized = true;
            s_bLastStreamProofState = g_MenuState.misc.streamProof;
            if (g_MenuState.misc.streamProof)
            {
                StreamProof::SetEnabled(true);
            }

            s_bImGuiInitialized = true;
            Logger::Log("ImGui context criado");
            Logger::Log("DX9 backend inicializado");
            Logger::Log("Win32 backend inicializado");
            Logger::Log("Menu inicializado");
            Logger::Log("Input inicializado");
            Logger::Log("SomaliaNative pronta");
        }

        if (s_bStreamProofInitialized)
        {
            bool currentCfg = g_MenuState.misc.streamProof;
            if (currentCfg != s_bLastStreamProofState)
            {
                StreamProof::SetEnabled(currentCfg);
                s_bLastStreamProofState = currentCfg;
            }
        }

        // Render Loop do ImGui
        ImGui_ImplDX9_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // 1. Renderiza os elementos visuais in-game (ESP e FOV Circle)
        ESP::Render();

        // 2. Renderiza o Legit Bot (Target Indicator e selecao suave)
        Aimbot::Render();

        // 2.1 Renderiza o Silent Aim isolado (Silent Target Marker [ O ], Silent FOV Circle, telemetria)
        SilentAim::Render();

        // 3. Renderiza o Ragebot completamente independente (indicadores, fov crimson e vetor de agressividade)
        RageBot::Render();

        // 4. Processa os modificadores locais de motor do GTA (Player, Veículo, Ambiente)
        LocalMods::Update();

        // 5. Processa KFC Slide
        KFCSlide::Update();

        // 5.1 Processa Auto Slide Nativo (C-Slide original por margem de arma)
        AutoSlide::Update();

        // 5.2 Processa Fist Switch (xxxx.cs 1:1)
        FistSwitch::Update();

        // 5.25 Processa Auto Punch (Auto Soco apos slide + troca para soco)
        AutoPunch::Update();

        // 5.3 Processa Anti-Aim e Fake Lag
        AntiAim::Update();

        // 5.3 Processa Triggerbot
        TriggerBot::Update();

        // 5.4 Processa Player Slap Exploit (Hotkeys e Notificacoes)
        PlayerSlap::Update();
        PlayerSlap::RenderNotifications();

        // 6. Renderiza a interface Somalia (quando aberta)
        Menu::Render();

        ImGui::EndFrame();
        ImGui::Render();
        ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

        return s_oPresent(pDevice, pSourceRect, pDestRect, hDestWindowOverride, pDirtyRegion);
    }

    bool Initialize()
    {
        s_HooksRestored.store(false);
        s_UIDestroyed.store(false);

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

    void RestoreHooks()
    {
        if (s_HooksRestored.exchange(true))
            return;

        if (s_pVTable)
        {
            DWORD oldProtect;

            if (s_oPresent)
            {
                VirtualProtect(&s_pVTable[17], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
                s_pVTable[17] = reinterpret_cast<void*>(s_oPresent);
                VirtualProtect(&s_pVTable[17], sizeof(void*), oldProtect, &oldProtect);
                // Ponteiro original s_oPresent preservado para evitar access violation em chamadas defensivas
            }

            if (s_oReset)
            {
                VirtualProtect(&s_pVTable[16], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
                s_pVTable[16] = reinterpret_cast<void*>(s_oReset);
                VirtualProtect(&s_pVTable[16], sizeof(void*), oldProtect, &oldProtect);
                // Ponteiro original s_oReset preservado para evitar access violation em chamadas defensivas
            }

            s_pVTable = nullptr;
        }
    }

    void DestroyUI()
    {
        if (s_UIDestroyed.exchange(true))
            return;

        if (s_bImGuiInitialized)
        {
            Menu::Shutdown();
            ImGui_ImplDX9_Shutdown();
            ImGui_ImplWin32_Shutdown();
            ImGui::DestroyContext();
            s_bImGuiInitialized = false;
        }
    }

    void Shutdown()
    {
        RestoreHooks();
        DestroyUI();
        if (s_bStreamProofInitialized)
        {
            StreamProof::Shutdown();
            s_bStreamProofInitialized = false;
        }
    }
}

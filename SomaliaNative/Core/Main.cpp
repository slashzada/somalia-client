#include <windows.h>
#include <atomic>
#include "Main.h"
#include "Logger.h"
#include "RuntimeState.h"
#include "../Engine/GTA/GTA.h"
#include "../Engine/SAMP/SAMP.h"
#include "../Render/D3D9Hook.h"
#include "../Config/Config.h"
#include "../Input/InputManager.h"
#include "../Features/LocalMods/LocalMods.h"
#include "../Features/Slide/Slide.h"
#include "../Features/AntiAim/AntiAim.h"
#include "../Features/Aimbot/AimAssist.h"
#include "../Features/Aimbot/Aimbot.h"
#include "../Features/Aimbot/RageBot.h"

static HMODULE s_hModule = NULL;
static std::atomic<ShutdownState> s_ShutdownState(ShutdownState::Running);
static HANDLE s_hInitThread = nullptr;
static HANDLE s_hShutdownThread = nullptr;

static DWORD WINAPI ShutdownWorkerThread(LPVOID lpParam);

namespace Main
{
    ShutdownState GetShutdownState()
    {
        return s_ShutdownState.load();
    }

    bool IsShuttingDown()
    {
        return s_ShutdownState.load() != ShutdownState::Running;
    }

    bool IsStopRequested()
    {
        return s_ShutdownState.load() == ShutdownState::StopRequested;
    }

    void RequestUnload()
    {
        ShutdownState expected = ShutdownState::Running;
        if (!s_ShutdownState.compare_exchange_strong(expected, ShutdownState::StopRequested))
            return;

        Logger::Log("[SOMALIA][LIFECYCLE] STOP REQUESTED");
        BeginShutdown();
    }

    void BeginShutdown()
    {
        ShutdownState expected = ShutdownState::StopRequested;
        if (!s_ShutdownState.compare_exchange_strong(expected, ShutdownState::Stopping))
            return;

        s_hShutdownThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ShutdownWorkerThread, NULL, 0, NULL);
    }

    bool IsUnloaded()
    {
        return s_ShutdownState.load() == ShutdownState::Stopped;
    }

    HMODULE GetModuleInstance()
    {
        return s_hModule;
    }

    void UnloadAndExit()
    {
        RequestUnload();
    }
}

static DWORD WINAPI ShutdownWorkerThread(LPVOID lpParam)
{
    Logger::Log("[SOMALIA][UNLOAD] STOPPING");

    // ETAPA 3: Bloquear novos callbacks de executar lógica do Somalia.
    // Garantido pois IsShuttingDown() retorna true para todos os callbacks (hkPresent, hkReset, hkWndProc, RakNet).

    // ETAPA 4 & 5: Parar e aguardar InitializationThread
    Logger::Log("[SOMALIA][UNLOAD] THREADS STOP REQUESTED");
    if (s_hInitThread != nullptr)
    {
        DWORD waitResult = WaitForSingleObject(s_hInitThread, 3000);
        if (waitResult == WAIT_TIMEOUT)
        {
            Logger::Log("[SOMALIA][UNLOAD] AVISO: Timeout ao aguardar InitializationThread.");
        }
        CloseHandle(s_hInitThread);
        s_hInitThread = nullptr;
    }
    Logger::Log("[SOMALIA][UNLOAD] THREAD InitializationThread STOPPED");

    // ETAPA 7: Restaurar hooks (ordem mínima: D3D9, WndProc, SAMP/Rak hook)
    Logger::Log("[SOMALIA][UNLOAD] RESTORING HOOKS");
    D3D9Hook::RestoreHooks();
    InputManager::RestoreWndProc();
    SAMP::Shutdown();
    Logger::Log("[SOMALIA][UNLOAD] HOOKS RESTORED");

    // ETAPA 8: Permitir pequena drenagem de callbacks já em execução
    Sleep(100);

    // ETAPA 9: Resetar módulos (resets já existentes)
    LocalMods::Reset();
    Slide::Reset();
    AntiAim::Reset();
    AimAssist::Reset();
    Aimbot::ClearTarget();
    RageBot::Reset();
    InputManager::Shutdown();

    // ETAPA 10: Destruir UI (Menu, ImGui DX9, ImGui Win32, ImGui Context)
    Logger::Log("[SOMALIA][UNLOAD] DESTROYING UI");
    D3D9Hook::DestroyUI();

    // ETAPA 11: Marcar Stopped
    s_ShutdownState.store(ShutdownState::Stopped);
    Logger::Log("[SOMALIA][UNLOAD] STOPPED");

    // ETAPA 12: Fechar logger
    Logger::Log("[SOMALIA][UNLOAD] FREE LIBRARY");
    Logger::Shutdown();

    // ETAPA 13: Executar FreeLibraryAndExitThread(...) em um único local controlado
    HANDLE hSelf = s_hShutdownThread;
    s_hShutdownThread = nullptr;
    if (hSelf)
    {
        CloseHandle(hSelf);
    }

    FreeLibraryAndExitThread(s_hModule, 0);
    return 0;
}

static DWORD WINAPI InitializationThread(LPVOID lpParam)
{
    Logger::Initialize();
    RuntimeState::Initialize();
    Logger::Log("Native inicializando...");

    // 1. Aguarda defensivamente o GTA SA criar a janela e o dispositivo D3D9
    int waitLimit = 200; // ~20 segundos
    while (!GTA::IsReady() && waitLimit > 0 && !Main::IsShuttingDown())
    {
        Sleep(100);
        waitLimit--;
    }

    if (Main::IsShuttingDown())
    {
        Logger::Log("[SOMALIA][UNLOAD] InitializationThread cancellation detected");
        return 0;
    }

    if (!GTA::IsReady())
    {
        Logger::Log("Falha: Tempo limite excedido aguardando o motor D3D9 do GTA.");
        return 0;
    }

    if (Main::IsShuttingDown())
    {
        Logger::Log("[SOMALIA][UNLOAD] InitializationThread cancellation detected");
        return 0;
    }

    Logger::Log("GTA detectado");
    Logger::Log("Janela detectada");
    Logger::Log("D3D9 device detectado");

    // 2. Instalação dos hooks mínimos de renderização e input
    if (!D3D9Hook::Initialize())
    {
        Logger::Log("Falha ao inicializar D3D9Hook.");
        return 0;
    }

    if (Main::IsShuttingDown())
    {
        Logger::Log("[SOMALIA][UNLOAD] InitializationThread cancellation detected");
        return 0;
    }

    // 3. Carregamento da configuracao persistida (se existir)
    Config::Load("somalia_config.json");

    return 1;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        s_hModule = hModule;
        DisableThreadLibraryCalls(hModule);
        s_hInitThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)InitializationThread, NULL, 0, NULL);
        break;

    case DLL_PROCESS_DETACH:
        // Teardown pesado NÃO é executado aqui.
        // Se o unload foi voluntário, o ShutdownWorkerThread já realizou todo o teardown antes de FreeLibraryAndExitThread.
        // Se o processo do jogo está encerrando (lpReserved != NULL), evita-se operações bloqueantes no Loader Lock.
        break;
    }
    return TRUE;
}

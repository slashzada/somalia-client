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
static HANDLE s_hCancelInitEvent = nullptr;
static SRWLOCK s_CallbackLock = SRWLOCK_INIT;
static int s_ActiveCallbacks = 0;
static HANDLE s_hZeroCallbacksEvent = nullptr;

static DWORD WINAPI ShutdownWorkerThread(LPVOID lpParam);

static HANDLE GetOrCreateZeroCallbacksEvent()
{
    if (!s_hZeroCallbacksEvent)
    {
        s_hZeroCallbacksEvent = CreateEventA(NULL, TRUE, TRUE, NULL);
    }
    return s_hZeroCallbacksEvent;
}

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
        if (!s_hShutdownThread)
        {
            Logger::Log("[SOMALIA][UNLOAD] ERRO CRITICO: Falha ao criar ShutdownWorkerThread (GetLastError=0x%X).", GetLastError());
        }
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

    bool EnterCallback()
    {
        AcquireSRWLockExclusive(&s_CallbackLock);
        s_ActiveCallbacks++;
        HANDLE hEvent = GetOrCreateZeroCallbacksEvent();
        if (hEvent)
        {
            ResetEvent(hEvent);
        }
        bool canExecuteInternal = (s_ShutdownState.load() == ShutdownState::Running);
        ReleaseSRWLockExclusive(&s_CallbackLock);
        return canExecuteInternal;
    }

    void LeaveCallback()
    {
        AcquireSRWLockExclusive(&s_CallbackLock);
        s_ActiveCallbacks--;
        if (s_ActiveCallbacks <= 0)
        {
            s_ActiveCallbacks = 0;
            HANDLE hEvent = GetOrCreateZeroCallbacksEvent();
            if (hEvent)
            {
                SetEvent(hEvent);
            }
        }
        ReleaseSRWLockExclusive(&s_CallbackLock);
    }

    bool WaitForCallbacks(DWORD timeoutMs)
    {
        AcquireSRWLockShared(&s_CallbackLock);
        if (s_ActiveCallbacks == 0)
        {
            ReleaseSRWLockShared(&s_CallbackLock);
            return true;
        }
        ReleaseSRWLockShared(&s_CallbackLock);

        HANDLE hEvent = nullptr;
        AcquireSRWLockExclusive(&s_CallbackLock);
        hEvent = GetOrCreateZeroCallbacksEvent();
        ReleaseSRWLockExclusive(&s_CallbackLock);

        if (!hEvent)
            return false;

        DWORD res = WaitForSingleObject(hEvent, timeoutMs);
        return (res == WAIT_OBJECT_0);
    }

    int GetActiveCallbacksCount()
    {
        AcquireSRWLockShared(&s_CallbackLock);
        int count = s_ActiveCallbacks;
        ReleaseSRWLockShared(&s_CallbackLock);
        return count;
    }
}

static DWORD WINAPI ShutdownWorkerThread(LPVOID lpParam)
{
    Logger::Log("[SOMALIA][UNLOAD] STOPPING");

    // 4. Bloquear lógica interna de novos callbacks:
    // Já garantido pois s_ShutdownState == Stopping, fazendo CallbackGuard::IsActive() retornar false para novos callbacks.

    // 5 & 6. Sinalizar cancelamento e aguardar cooperativamente a InitializationThread
    Logger::Log("[SOMALIA][UNLOAD] THREADS STOP REQUESTED");
    if (s_hCancelInitEvent)
    {
        SetEvent(s_hCancelInitEvent);
    }

    if (s_hInitThread != nullptr)
    {
        DWORD waitResult = WaitForSingleObject(s_hInitThread, 5000);
        if (waitResult != WAIT_OBJECT_0)
        {
            Logger::Log("[SOMALIA][UNLOAD] ERRO CRITICO: InitializationThread nao finalizou cooperativamente (waitResult=0x%X). Unload abortado por seguranca.", waitResult);
            // Seguranca absoluta: nunca fechar handle nem chamar FreeLibraryAndExitThread com thread viva!
            return 0;
        }

        CloseHandle(s_hInitThread);
        s_hInitThread = nullptr;
        Logger::Log("[SOMALIA][UNLOAD] THREAD InitializationThread STOPPED");
    }

    // 7. Aguardar ActiveCallbacks == 0
    Logger::Log("[SOMALIA][UNLOAD] WAITING FOR ACTIVE CALLBACKS");
    if (!Main::WaitForCallbacks(5000))
    {
        Logger::Log("[SOMALIA][UNLOAD] ERROR: Timeout aguardando callbacks ativos (restantes=%d).",
            Main::GetActiveCallbacksCount());
        Logger::Log("[SOMALIA][UNLOAD] UNLOAD ABORTED: callbacks ainda ativos.");
        return 0;
    }
    Logger::Log("[SOMALIA][UNLOAD] ACTIVE CALLBACKS DRAINED");

    // 8, 9, 10. Restaurar hooks (D3D9, WndProc, SAMP/Rak hook)
    Logger::Log("[SOMALIA][UNLOAD] RESTORING HOOKS");
    D3D9Hook::RestoreHooks();
    InputManager::RestoreWndProc();
    SAMP::Shutdown();
    Logger::Log("[SOMALIA][UNLOAD] HOOKS RESTORED");

    // Drenagem final para garantir que nenhum callback in-flight no momento da restauração fique pendente
    if (!Main::WaitForCallbacks(1000))
    {
        Logger::Log("[SOMALIA][UNLOAD] ERROR: Timeout na drenagem pos-hooks (restantes=%d).",
            Main::GetActiveCallbacksCount());
        Logger::Log("[SOMALIA][UNLOAD] UNLOAD ABORTED: callbacks ainda ativos pos-hooks.");
        return 0;
    }

    // 11. Resetar módulos (resets já existentes)
    LocalMods::Reset();
    Slide::Reset();
    AntiAim::Reset();
    AimAssist::Reset();
    Aimbot::ClearTarget();
    RageBot::Reset();
    InputManager::Shutdown();

    // 12. Destruir UI (Menu, ImGui DX9, ImGui Win32, ImGui Context)
    Logger::Log("[SOMALIA][UNLOAD] DESTROYING UI");
    D3D9Hook::DestroyUI();

    // Marcar Stopped
    s_ShutdownState.store(ShutdownState::Stopped);
    Logger::Log("[SOMALIA][UNLOAD] STOPPED");

    // 13. Fechar logger
    Logger::Log("[SOMALIA][UNLOAD] FREE LIBRARY");
    Logger::Shutdown();

    // Fechar handles de sincronização
    AcquireSRWLockExclusive(&s_CallbackLock);
    if (s_hZeroCallbacksEvent)
    {
        CloseHandle(s_hZeroCallbacksEvent);
        s_hZeroCallbacksEvent = nullptr;
    }
    ReleaseSRWLockExclusive(&s_CallbackLock);

    if (s_hCancelInitEvent)
    {
        CloseHandle(s_hCancelInitEvent);
        s_hCancelInitEvent = nullptr;
    }

    // 14. Executar FreeLibraryAndExitThread(...) em um único local controlado
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

    // 1. Aguarda defensivamente o GTA SA criar a janela e o dispositivo D3D9 com cancelamento cooperativo
    int waitLimit = 200; // ~20 segundos
    while (!GTA::IsReady() && waitLimit > 0)
    {
        if (Main::IsShuttingDown())
        {
            Logger::Log("[SOMALIA][UNLOAD] InitializationThread cancellation detected");
            return 0;
        }

        if (s_hCancelInitEvent)
        {
            DWORD wr = WaitForSingleObject(s_hCancelInitEvent, 100);
            if (wr == WAIT_OBJECT_0 || Main::IsShuttingDown())
            {
                Logger::Log("[SOMALIA][UNLOAD] InitializationThread cancellation detected");
                return 0;
            }
        }
        else
        {
            Sleep(100);
        }

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
        s_hCancelInitEvent = CreateEventA(NULL, TRUE, FALSE, NULL);
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

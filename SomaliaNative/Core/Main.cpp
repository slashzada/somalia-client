#include <windows.h>
#include <atomic>
#include "Main.h"
#include "Logger.h"
#include "RuntimeState.h"
#include "../Engine/GTA/GTA.h"
#include "../Engine/SAMP/SAMP.h"
#include "../Render/D3D9Hook.h"
#include "../Config/Config.h"

static HMODULE s_hModule = NULL;
static std::atomic<bool> s_bShuttingDown(false);
static std::atomic<bool> s_bUnloaded(false);

namespace Main
{
    void RequestUnload()
    {
        if (s_bShuttingDown) return;
        s_bShuttingDown = true;
        Logger::Log("[SOMALIA][UNLOAD] REQUESTED");
    }

    bool IsShuttingDown()
    {
        return s_bShuttingDown;
    }

    bool IsUnloaded()
    {
        return s_bUnloaded;
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

static DWORD WINAPI InitializationThread(LPVOID lpParam)
{
    Logger::Initialize();
    RuntimeState::Initialize();
    Logger::Log("Native inicializando...");

    // 1. Aguarda defensivamente o GTA SA criar a janela e o dispositivo D3D9
    int waitLimit = 200; // ~20 segundos
    while (!GTA::IsReady() && waitLimit > 0)
    {
        Sleep(100);
        waitLimit--;
    }

    if (!GTA::IsReady())
    {
        Logger::Log("Falha: Tempo limite excedido aguardando o motor D3D9 do GTA.");
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
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)InitializationThread, NULL, 0, NULL);
        break;

    case DLL_PROCESS_DETACH:
        if (lpReserved == NULL)
        {
            SAMP::Shutdown();
            D3D9Hook::Shutdown();
            Logger::Shutdown();
        }
        break;
    }
    return TRUE;
}

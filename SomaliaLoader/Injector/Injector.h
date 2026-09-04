#pragma once
#include <windows.h>
#include <string>

namespace Injector
{
    DWORD FindProcessId(const std::string& processName = "gta_sa.exe");
    bool IsGameRunning();
    bool InjectDll(DWORD pid, const std::string& dllPath, std::string& outError);
    bool InjectGame(const std::string& dllPath, std::string& outError);
    bool UnloadGame(const std::string& moduleName, std::string& outError);

    void StartAutoInjectThread(const std::string& dllPath);
    void StopAutoInjectThread();
    bool IsAutoInjectWaiting();
    std::string GetStatusMessage();
    void SetStatusMessage(const std::string& msg);
}

#pragma once
#include <windows.h>
#include <string>

namespace Injector
{
    struct ProcessValidationResult
    {
        bool valid = false;
        std::string error = "";
        std::string executablePath = "";
        bool isX86 = false;
    };

    struct DllIntegrityResult
    {
        bool valid = false;
        std::string sha256 = "";
        std::string error = "";
    };

    // Validação granular do processo alvo
    DWORD FindProcessId(const std::string& processName = "gta_sa.exe");
    bool IsProcessAlive(DWORD pid);
    bool GetProcessExecutablePath(DWORD pid, std::string& outPath);
    bool IsProcessX86(HANDLE hProcess, bool& outIsX86);
    bool ValidateProcessPathMatchesConfig(const std::string& processPath, const std::string& configGtaPath);
    ProcessValidationResult ValidateTargetProcessHandle(HANDLE hProcess, const std::string& configGtaPath = "");
    ProcessValidationResult ValidateTargetProcess(DWORD pid, const std::string& configGtaPath = "");

    // Verificação de integridade da DLL/ASI
    DllIntegrityResult VerifyDllIntegrity(const std::string& dllPath, const std::string& expectedSha256 = "");

    // Funções principais
    bool IsGameRunning();
    bool InjectDll(DWORD pid, const std::string& dllPath, std::string& outError, const std::string& configGtaPath = "");
    bool InjectGame(const std::string& dllPath, std::string& outError);
    bool UnloadGame(const std::string& moduleName, std::string& outError);

    // Thread de injeção automática
    void StartAutoInjectThread(const std::string& dllPath);
    void StopAutoInjectThread();
    bool IsAutoInjectWaiting();
    std::string GetStatusMessage();
    void SetStatusMessage(const std::string& msg);
    void Shutdown();
}

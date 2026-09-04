#include "Injector.h"
#include <tlhelp32.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <vector>

namespace Injector
{
    static std::atomic<bool> s_WaitingForGame(false);
    static std::mutex s_StatusMutex;
    static std::string s_StatusMessage = "Pronto para injetar.";
    static std::thread s_AutoThread;

    void SetStatusMessage(const std::string& msg)
    {
        std::lock_guard<std::mutex> lock(s_StatusMutex);
        s_StatusMessage = msg;
    }

    std::string GetStatusMessage()
    {
        std::lock_guard<std::mutex> lock(s_StatusMutex);
        return s_StatusMessage;
    }

    static bool EnableDebugPrivilege()
    {
        HANDLE hToken = NULL;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
            return false;

        LUID luid;
        if (!LookupPrivilegeValueA(NULL, "SeDebugPrivilege", &luid))
        {
            CloseHandle(hToken);
            return false;
        }

        TOKEN_PRIVILEGES tp;
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        BOOL ok = AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
        CloseHandle(hToken);
        return (ok && GetLastError() != ERROR_NOT_ALL_ASSIGNED);
    }

    DWORD FindProcessId(const std::string& processName)
    {
        DWORD pid = 0;
        HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (hSnapshot == INVALID_HANDLE_VALUE) return 0;

        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hSnapshot, &pe))
        {
            do
            {
                if (_stricmp(pe.szExeFile, processName.c_str()) == 0)
                {
                    pid = pe.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnapshot, &pe));
        }

        CloseHandle(hSnapshot);
        return pid;
    }

    bool IsGameRunning()
    {
        return (FindProcessId("gta_sa.exe") != 0);
    }

    static bool IsModuleLoaded(DWORD pid, const std::string& moduleName, HMODULE* pOutMod = nullptr)
    {
        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (hSnap == INVALID_HANDLE_VALUE) return false;

        MODULEENTRY32 me;
        me.dwSize = sizeof(MODULEENTRY32);
        bool found = false;

        if (Module32First(hSnap, &me))
        {
            do
            {
                if (_stricmp(me.szModule, moduleName.c_str()) == 0 ||
                    strstr(me.szExePath, moduleName.c_str()) != NULL)
                {
                    found = true;
                    if (pOutMod) *pOutMod = me.hModule;
                    break;
                }
            } while (Module32Next(hSnap, &me));
        }

        CloseHandle(hSnap);
        return found;
    }

    bool InjectDll(DWORD pid, const std::string& dllPath, std::string& outError)
    {
        if (pid == 0)
        {
            outError = "Processo gta_sa.exe nao encontrado.";
            SetStatusMessage(outError);
            return false;
        }

        // 1. Sempre resolve para caminho absoluto canônico
        char fullDllPath[MAX_PATH] = { 0 };
        if (GetFullPathNameA(dllPath.c_str(), MAX_PATH, fullDllPath, NULL) == 0)
        {
            strncpy_s(fullDllPath, dllPath.c_str(), sizeof(fullDllPath) - 1);
        }

        // 2. Verifica existência do arquivo
        DWORD fileAttr = GetFileAttributesA(fullDllPath);
        if (fileAttr == INVALID_FILE_ATTRIBUTES || (fileAttr & FILE_ATTRIBUTE_DIRECTORY))
        {
            outError = std::string("Arquivo DLL/ASI nao encontrado: ") + fullDllPath;
            SetStatusMessage(outError);
            return false;
        }

        // 3. Eleva privilégios para depuração/injeção de processo
        EnableDebugPrivilege();

        HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid);
        if (!hProcess)
        {
            DWORD dwErr = GetLastError();
            outError = "Falha ao abrir processo gta_sa.exe (Erro Win32: " + std::to_string(dwErr) + "). Execute o Loader como Administrador.";
            SetStatusMessage(outError);
            return false;
        }

        size_t pathSize = strlen(fullDllPath) + 1;
        LPVOID pRemoteBuf = VirtualAllocEx(hProcess, NULL, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!pRemoteBuf)
        {
            CloseHandle(hProcess);
            outError = "Falha ao alocar memoria no processo do jogo.";
            SetStatusMessage(outError);
            return false;
        }

        SIZE_T bytesWritten = 0;
        if (!WriteProcessMemory(hProcess, pRemoteBuf, fullDllPath, pathSize, &bytesWritten) || bytesWritten < pathSize)
        {
            VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            outError = "Falha ao gravar caminho da DLL na memoria do jogo.";
            SetStatusMessage(outError);
            return false;
        }

        HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
        if (!hKernel32)
        {
            VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            outError = "Kernel32 nao encontrado.";
            SetStatusMessage(outError);
            return false;
        }

        LPVOID pLoadLibrary = (LPVOID)GetProcAddress(hKernel32, "LoadLibraryA");
        if (!pLoadLibrary)
        {
            VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            outError = "LoadLibraryA nao localizado no Kernel32.";
            SetStatusMessage(outError);
            return false;
        }

        HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibrary, pRemoteBuf, 0, NULL);
        if (!hThread)
        {
            VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            DWORD dwErr = GetLastError();
            outError = "Falha ao criar thread remota no jogo (Erro Win32: " + std::to_string(dwErr) + ").";
            SetStatusMessage(outError);
            return false;
        }

        // 4. Aguarda conclusao do carregamento com timeout defensivo
        DWORD waitRes = WaitForSingleObject(hThread, 6000);
        if (waitRes != WAIT_OBJECT_0)
        {
            CloseHandle(hThread);
            VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            outError = "Tempo limite excedido aguardando inicializacao remota.";
            SetStatusMessage(outError);
            return false;
        }

        // 5. Verificação mandatória do ExitCode da thread remota (retorno de LoadLibraryA)
        DWORD remoteExitCode = 0;
        if (!GetExitCodeThread(hThread, &remoteExitCode) || remoteExitCode == 0)
        {
            CloseHandle(hThread);
            VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            outError = "LoadLibraryA retornou NULL no jogo (Incompatibilidade ou jogo ainda nao carregado).";
            SetStatusMessage(outError);
            return false;
        }

        CloseHandle(hThread);
        VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);

        SetStatusMessage("Injetado com sucesso! Pressione F5 no jogo.");
        outError = "";
        return true;
    }

    bool InjectGame(const std::string& dllPath, std::string& outError)
    {
        char fullDll[MAX_PATH] = { 0 };
        if (GetFullPathNameA(dllPath.c_str(), MAX_PATH, fullDll, NULL) == 0)
        {
            strncpy_s(fullDll, dllPath.c_str(), sizeof(fullDll) - 1);
        }

        DWORD pid = FindProcessId("gta_sa.exe");
        if (pid != 0)
        {
            return InjectDll(pid, fullDll, outError);
        }
        outError = "gta_sa.exe nao esta aberto no momento.";
        SetStatusMessage(outError);
        return false;
    }

    bool UnloadGame(const std::string& moduleName, std::string& outError)
    {
        DWORD pid = FindProcessId("gta_sa.exe");
        if (pid == 0)
        {
            outError = "gta_sa.exe nao esta em execucao.";
            SetStatusMessage(outError);
            return false;
        }

        HMODULE hTargetMod = NULL;
        if (!IsModuleLoaded(pid, moduleName, &hTargetMod))
        {
            outError = "Modulo " + moduleName + " nao encontrado no processo.";
            SetStatusMessage(outError);
            return false;
        }

        // 1. Sinalização Cooperativa: Tenta acionar o evento sincronizado do SomaliaNative
        HANDLE hEvent = OpenEventA(EVENT_MODIFY_STATE, FALSE, "Global\\SomaliaNative_RequestUnload");
        if (!hEvent)
        {
            hEvent = OpenEventA(EVENT_MODIFY_STATE, FALSE, "SomaliaNative_RequestUnload");
        }

        if (hEvent)
        {
            SetEvent(hEvent);
            CloseHandle(hEvent);
            SetStatusMessage("Sinal de descarregamento seguro enviado...");

            // Aguarda até 3.5 segundos o modulo desmontar hooks e sair cooperativamente
            for (int i = 0; i < 35; i++)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (!IsModuleLoaded(pid, moduleName))
                {
                    SetStatusMessage("Cheat desinjetado com sucesso de forma cooperativa!");
                    outError = "Descarregado com sucesso!";
                    return true;
                }
            }
        }

        // 2. Se o evento não estava ativo, tenta export UnloadSomalia remoto cooperativo
        EnableDebugPrivilege();
        HANDLE hProc = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION, FALSE, pid);
        if (hProc)
        {
            // Tenta obter o export UnloadSomalia
            HMODULE hLocalMod = LoadLibraryExA(moduleName.c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES);
            if (hLocalMod)
            {
                FARPROC pUnload = GetProcAddress(hLocalMod, "UnloadSomalia");
                if (pUnload)
                {
                    uintptr_t offset = reinterpret_cast<uintptr_t>(pUnload) - reinterpret_cast<uintptr_t>(hLocalMod);
                    uintptr_t remoteFunc = reinterpret_cast<uintptr_t>(hTargetMod) + offset;
                    HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)remoteFunc, NULL, 0, NULL);
                    if (hThread)
                    {
                        WaitForSingleObject(hThread, 3000);
                        CloseHandle(hThread);
                    }
                }
                FreeLibrary(hLocalMod);
            }

            // Aguarda o teardown cooperativo concluir
            for (int i = 0; i < 20; i++)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (!IsModuleLoaded(pid, moduleName))
                {
                    CloseHandle(hProc);
                    SetStatusMessage("Cheat desinjetado cooperativamente!");
                    outError = "Descarregado com sucesso!";
                    return true;
                }
            }
            CloseHandle(hProc);
        }

        // Se chegou até aqui, para proteger o jogo de Crash 0xC0000005, não forçamos FreeLibrary brusco.
        outError = "Descarregamento seguro pendente. Feche o GTA para encerrar a sessao.";
        SetStatusMessage(outError);
        return false;
    }

    void StartAutoInjectThread(const std::string& dllPath)
    {
        if (s_WaitingForGame) return;
        s_WaitingForGame = true;
        SetStatusMessage("Aguardando gta_sa.exe iniciar...");

        if (s_AutoThread.joinable())
        {
            s_AutoThread.detach();
        }

        char fullDll[MAX_PATH] = { 0 };
        if (GetFullPathNameA(dllPath.c_str(), MAX_PATH, fullDll, NULL) == 0)
        {
            strncpy_s(fullDll, dllPath.c_str(), sizeof(fullDll) - 1);
        }

        std::string resolvedPath = fullDll;

        s_AutoThread = std::thread([resolvedPath]()
        {
            while (s_WaitingForGame)
            {
                DWORD pid = FindProcessId("gta_sa.exe");
                if (pid != 0)
                {
                    // Aguarda defensivamente módulos básicos carregarem
                    std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                    std::string err;
                    if (InjectDll(pid, resolvedPath, err))
                    {
                        SetStatusMessage("Somalia injetado com sucesso! Bom jogo!");
                    }
                    else
                    {
                        SetStatusMessage("Erro na injecao: " + err);
                    }
                    s_WaitingForGame = false;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        });
    }

    void StopAutoInjectThread()
    {
        s_WaitingForGame = false;
        SetStatusMessage("Injecao cancelada.");
    }

    bool IsAutoInjectWaiting()
    {
        return s_WaitingForGame.load();
    }
}

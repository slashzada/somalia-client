#include "Injector.h"
#include <tlhelp32.h>
#include <thread>
#include <atomic>
#include <chrono>

namespace Injector
{
    static std::atomic<bool> s_WaitingForGame(false);
    static std::string s_StatusMessage = "Pronto para injetar.";
    static std::thread s_AutoThread;

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

    bool InjectDll(DWORD pid, const std::string& dllPath, std::string& outError)
    {
        if (pid == 0)
        {
            outError = "Processo gta_sa.exe nao encontrado.";
            s_StatusMessage = outError;
            return false;
        }

        // Verifica existencia do arquivo
        DWORD fileAttr = GetFileAttributesA(dllPath.c_str());
        if (fileAttr == INVALID_FILE_ATTRIBUTES || (fileAttr & FILE_ATTRIBUTE_DIRECTORY))
        {
            outError = "Arquivo DLL/ASI nao encontrado: " + dllPath;
            s_StatusMessage = outError;
            return false;
        }

        HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid);
        if (!hProcess)
        {
            outError = "Falha ao abrir processo (Execute o Loader como Administrador).";
            s_StatusMessage = outError;
            return false;
        }

        size_t pathSize = dllPath.length() + 1;
        LPVOID pRemoteBuf = VirtualAllocEx(hProcess, NULL, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!pRemoteBuf)
        {
            CloseHandle(hProcess);
            outError = "Falha ao alocar memoria no processo do jogo.";
            s_StatusMessage = outError;
            return false;
        }

        SIZE_T bytesWritten = 0;
        if (!WriteProcessMemory(hProcess, pRemoteBuf, dllPath.c_str(), pathSize, &bytesWritten) || bytesWritten < pathSize)
        {
            VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            outError = "Falha ao gravar caminho da DLL na memoria do jogo.";
            s_StatusMessage = outError;
            return false;
        }

        HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
        if (!hKernel32)
        {
            VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            outError = "Kernel32 nao encontrado.";
            s_StatusMessage = outError;
            return false;
        }

        LPVOID pLoadLibrary = (LPVOID)GetProcAddress(hKernel32, "LoadLibraryA");
        if (!pLoadLibrary)
        {
            VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            outError = "LoadLibraryA nao localizado.";
            s_StatusMessage = outError;
            return false;
        }

        HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)pLoadLibrary, pRemoteBuf, 0, NULL);
        if (!hThread)
        {
            VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
            CloseHandle(hProcess);
            outError = "Falha ao criar thread remota no jogo.";
            s_StatusMessage = outError;
            return false;
        }

        // Aguarda conclusao do carregamento
        WaitForSingleObject(hThread, 5000);

        CloseHandle(hThread);
        VirtualFreeEx(hProcess, pRemoteBuf, 0, MEM_RELEASE);
        CloseHandle(hProcess);

        s_StatusMessage = "Injetado com sucesso! Pressione F5 no jogo.";
        return true;
    }

    bool InjectGame(const std::string& dllPath, std::string& outError)
    {
        DWORD pid = FindProcessId("gta_sa.exe");
        if (pid != 0)
        {
            return InjectDll(pid, dllPath, outError);
        }
        outError = "gta_sa.exe nao esta aberto no momento.";
        s_StatusMessage = outError;
        return false;
    }

    bool UnloadGame(const std::string& moduleName, std::string& outError)
    {
        DWORD pid = FindProcessId("gta_sa.exe");
        if (pid == 0)
        {
            outError = "gta_sa.exe nao esta em execucao.";
            return false;
        }

        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (hSnap == INVALID_HANDLE_VALUE)
        {
            outError = "Falha ao obter snapshot de modulos.";
            return false;
        }

        MODULEENTRY32 me;
        me.dwSize = sizeof(MODULEENTRY32);
        HMODULE hTargetMod = NULL;

        if (Module32First(hSnap, &me))
        {
            do
            {
                if (_stricmp(me.szModule, moduleName.c_str()) == 0 ||
                    strstr(me.szExePath, moduleName.c_str()) != NULL)
                {
                    hTargetMod = me.hModule;
                    break;
                }
            } while (Module32Next(hSnap, &me));
        }
        CloseHandle(hSnap);

        if (!hTargetMod)
        {
            outError = "Modulo " + moduleName + " nao encontrado no processo.";
            return false;
        }

        HANDLE hProc = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION, FALSE, pid);
        if (!hProc)
        {
            outError = "Falha ao abrir processo gta_sa.exe.";
            return false;
        }

        LPVOID pFreeLib = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "FreeLibrary");
        if (!pFreeLib)
        {
            CloseHandle(hProc);
            outError = "FreeLibrary nao encontrada.";
            return false;
        }

        HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)pFreeLib, (LPVOID)hTargetMod, 0, NULL);
        if (!hThread)
        {
            CloseHandle(hProc);
            outError = "Falha ao criar thread remota de desinjecao.";
            return false;
        }

        WaitForSingleObject(hThread, 3000);
        CloseHandle(hThread);
        CloseHandle(hProc);

        s_StatusMessage = "Cheat desinjetado com sucesso do jogo!";
        outError = "Modulo desinjetado com sucesso!";
        return true;
    }

    void StartAutoInjectThread(const std::string& dllPath)
    {
        if (s_WaitingForGame) return;
        s_WaitingForGame = true;
        s_StatusMessage = "Aguardando gta_sa.exe iniciar...";

        if (s_AutoThread.joinable())
        {
            s_AutoThread.detach();
        }

        s_AutoThread = std::thread([dllPath]()
        {
            while (s_WaitingForGame)
            {
                DWORD pid = FindProcessId("gta_sa.exe");
                if (pid != 0)
                {
                    // Pequeno delay para garantir que os modulos do jogo terminaram de carregar
                    std::this_thread::sleep_for(std::chrono::milliseconds(1500));
                    std::string err;
                    if (InjectDll(pid, dllPath, err))
                    {
                        s_StatusMessage = "Somalia injetado com sucesso! Bom jogo!";
                    }
                    else
                    {
                        s_StatusMessage = "Erro na injecao: " + err;
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
        s_StatusMessage = "Injecao cancelada.";
    }

    bool IsAutoInjectWaiting()
    {
        return s_WaitingForGame;
    }

    std::string GetStatusMessage()
    {
        return s_StatusMessage;
    }
}

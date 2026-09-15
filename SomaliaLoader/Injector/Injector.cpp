#include "Injector.h"
#include "../Payload/SomaliaPayload.h"
#include "../Config/LoaderConfig.h"
#include <tlhelp32.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>

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

    static std::string s_ExtractedPayloadPath = "";

    std::string GetOrExtractPayload(std::string& outError)
    {
        // 1. Verifica se já existe um arquivo local na pasta atual ou caminho relativo (override para desenvolvimento)
        char exePath[MAX_PATH] = { 0 };
        GetModuleFileNameA(NULL, exePath, MAX_PATH);
        std::string dir = exePath;
        size_t lastSlash = dir.find_last_of("\\/");
        if (lastSlash != std::string::npos)
            dir = dir.substr(0, lastSlash);

        std::vector<std::string> localCandidates = {
            dir + "\\SomaliaNative.asi",
            dir + "\\SomaliaNative.dll",
            dir + "\\build\\SomaliaNative.asi",
            dir + "\\..\\SomaliaNative\\build\\SomaliaNative.asi",
            dir + "\\..\\SomaliaNative.asi",
            dir + "\\dist\\SomaliaNative.asi",
            "SomaliaNative.asi",
            "SomaliaNative.dll"
        };

        for (const auto& c : localCandidates)
        {
            if (GetFileAttributesA(c.c_str()) != INVALID_FILE_ATTRIBUTES)
            {
                char fullPath[MAX_PATH] = { 0 };
                GetFullPathNameA(c.c_str(), MAX_PATH, fullPath, NULL);
                return std::string(fullPath);
            }
        }

        // 2. Se não houver arquivo externo no disco, extrai o payload C++ embutido no próprio .exe
        if (g_SomaliaNativePayloadSize == 0 || g_SomaliaNativePayload == nullptr)
        {
            outError = "Payload embutido nao disponivel e nenhum arquivo .asi/.dll localizado.";
            return "";
        }

        char tempDir[MAX_PATH] = { 0 };
        if (!GetTempPathA(MAX_PATH, tempDir))
        {
            outError = "Falha ao obter diretorio temporario do sistema.";
            return "";
        }

        std::string extractedDll = std::string(tempDir) + "somalia_core.dll";

        // 2.1 Verifica se o arquivo principal já existe e é válido
        HANDLE hExisting = CreateFileA(extractedDll.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hExisting != INVALID_HANDLE_VALUE)
        {
            DWORD sz = GetFileSize(hExisting, NULL);
            CloseHandle(hExisting);
            if (sz == g_SomaliaNativePayloadSize)
            {
                s_ExtractedPayloadPath = extractedDll;
                return extractedDll;
            }
        }

        // 2.2 Tenta gravar no arquivo principal
        FILE* f = fopen(extractedDll.c_str(), "wb");
        if (f)
        {
            size_t written = fwrite(g_SomaliaNativePayload, 1, g_SomaliaNativePayloadSize, f);
            fclose(f);
            if (written == g_SomaliaNativePayloadSize)
            {
                s_ExtractedPayloadPath = extractedDll;
                return extractedDll;
            }
        }

        // 2.3 Caso o arquivo principal esteja travado por processo aberto, tenta nomes alternativos em %TEMP%
        for (int i = 1; i <= 20; ++i)
        {
            std::string altDll = std::string(tempDir) + "somalia_core_" + std::to_string(i) + ".dll";

            HANDLE hAlt = CreateFileA(altDll.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hAlt != INVALID_HANDLE_VALUE)
            {
                DWORD sz = GetFileSize(hAlt, NULL);
                CloseHandle(hAlt);
                if (sz == g_SomaliaNativePayloadSize)
                {
                    s_ExtractedPayloadPath = altDll;
                    return altDll;
                }
            }

            FILE* fAlt = fopen(altDll.c_str(), "wb");
            if (fAlt)
            {
                size_t written = fwrite(g_SomaliaNativePayload, 1, g_SomaliaNativePayloadSize, fAlt);
                fclose(fAlt);
                if (written == g_SomaliaNativePayloadSize)
                {
                    s_ExtractedPayloadPath = altDll;
                    return altDll;
                }
            }
        }

        outError = "Falha ao extrair DLL embutida em: " + extractedDll;
        return "";
    }

    void CleanupExtractedPayload()
    {
        if (!s_ExtractedPayloadPath.empty())
        {
            DeleteFileA(s_ExtractedPayloadPath.c_str());
            s_ExtractedPayloadPath.clear();
        }

        char tempDir[MAX_PATH] = { 0 };
        if (GetTempPathA(MAX_PATH, tempDir))
        {
            std::string pattern = std::string(tempDir) + "somalia_*.dll";
            WIN32_FIND_DATAA fd;
            HANDLE hFind = FindFirstFileA(pattern.c_str(), &fd);
            if (hFind != INVALID_HANDLE_VALUE)
            {
                do {
                    std::string fPath = std::string(tempDir) + fd.cFileName;
                    DeleteFileA(fPath.c_str());
                } while (FindNextFileA(hFind, &fd));
                FindClose(hFind);
            }
        }
    }

    bool InjectDll(DWORD pid, const std::string& dllPath, std::string& outError)
    {
        if (pid == 0)
        {
            outError = "Processo gta_sa.exe nao encontrado.";
            s_StatusMessage = outError;
            return false;
        }

        // Checagem se o Somalia já está injetado no GTA SA
        HANDLE hCheckSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (hCheckSnap != INVALID_HANDLE_VALUE)
        {
            MODULEENTRY32 meCheck;
            meCheck.dwSize = sizeof(MODULEENTRY32);
            if (Module32First(hCheckSnap, &meCheck))
            {
                do {
                    std::string mName = meCheck.szModule;
                    for (auto& c : mName) c = tolower(c);
                    if (mName.find("somalia") != std::string::npos)
                    {
                        CloseHandle(hCheckSnap);
                        s_StatusMessage = "Somalia ja esta injetado no GTA SA! Pressione F5 no jogo.";
                        return true;
                    }
                } while (Module32Next(hCheckSnap, &meCheck));
            }
            CloseHandle(hCheckSnap);
        }

        std::string targetPath = dllPath;

        // Se o caminho estiver vazio ou o arquivo não existir fisicamente, tenta usar/extrair o payload embutido
        DWORD fileAttr = targetPath.empty() ? INVALID_FILE_ATTRIBUTES : GetFileAttributesA(targetPath.c_str());
        if (fileAttr == INVALID_FILE_ATTRIBUTES || (fileAttr & FILE_ATTRIBUTE_DIRECTORY))
        {
            std::string extractErr;
            std::string resolved = GetOrExtractPayload(extractErr);
            if (!resolved.empty())
            {
                targetPath = resolved;
            }
            else
            {
                outError = extractErr.empty() ? ("Arquivo DLL/ASI nao encontrado: " + dllPath) : extractErr;
                s_StatusMessage = outError;
                return false;
            }
        }

        HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid);
        if (!hProcess)
        {
            outError = "Falha ao abrir processo (Execute o Loader como Administrador).";
            s_StatusMessage = outError;
            return false;
        }

        // Sincroniza configuracao e sessao KeyAuth na pasta do GTA e registro
        char gtaFullPath[MAX_PATH] = { 0 };
        DWORD dwSize = MAX_PATH;
        if (QueryFullProcessImageNameA(hProcess, 0, gtaFullPath, &dwSize))
        {
            std::string gtaPathStr = gtaFullPath;
            size_t lastBackslash = gtaPathStr.find_last_of("\\/");
            if (lastBackslash != std::string::npos)
            {
                std::string gtaDir = gtaPathStr.substr(0, lastBackslash);
                ConfigManager::Get().gtaPath = gtaDir;
            }
        }
        ConfigManager::Save();

        size_t pathSize = targetPath.length() + 1;
        LPVOID pRemoteBuf = VirtualAllocEx(hProcess, NULL, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!pRemoteBuf)
        {
            CloseHandle(hProcess);
            outError = "Falha ao alocar memoria no processo do jogo.";
            s_StatusMessage = outError;
            return false;
        }

        SIZE_T bytesWritten = 0;
        if (!WriteProcessMemory(hProcess, pRemoteBuf, targetPath.c_str(), pathSize, &bytesWritten) || bytesWritten < pathSize)
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
        // 1. Sinaliza o evento cooperativo para desinjecao limpa pelo proprio SomaliaNative (restaurando D3D9/WndProc)
        HANDLE hEvent = OpenEventA(EVENT_MODIFY_STATE, FALSE, "Somalia_UnloadEvent");
        if (hEvent)
        {
            SetEvent(hEvent);
            CloseHandle(hEvent);
            Sleep(400); // Aguarda SomaliaNative descarregar e restaurar hooks
        }

        DWORD pid = FindProcessId("gta_sa.exe");
        if (pid == 0)
        {
            outError = "gta_sa.exe nao esta em execucao.";
            return true;
        }

        HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
        if (hSnap == INVALID_HANDLE_VALUE)
        {
            outError = "Falha ao obter snapshot de modulos.";
            return false;
        }

        MODULEENTRY32 me;
        me.dwSize = sizeof(MODULEENTRY32);
        std::vector<HMODULE> somaliaModules;

        if (Module32First(hSnap, &me))
        {
            do
            {
                std::string modName = me.szModule;
                for (auto& c : modName) c = tolower(c);

                if (modName.find("somalia") != std::string::npos ||
                    (!moduleName.empty() && _stricmp(me.szModule, moduleName.c_str()) == 0))
                {
                    somaliaModules.push_back(me.hModule);
                }
            } while (Module32Next(hSnap, &me));
        }
        CloseHandle(hSnap);

        if (somaliaModules.empty())
        {
            s_StatusMessage = "Cheat desinjetado com sucesso do jogo!";
            outError = "Modulo desinjetado com sucesso!";
            return true;
        }

        // Se algum modulo persistir na memoria, realiza FreeLibrary remoto cooperativo
        HANDLE hProc = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION, FALSE, pid);
        if (hProc)
        {
            LPVOID pFreeLib = (LPVOID)GetProcAddress(GetModuleHandleA("kernel32.dll"), "FreeLibrary");
            if (pFreeLib)
            {
                for (HMODULE hMod : somaliaModules)
                {
                    HANDLE hThread = CreateRemoteThread(hProc, NULL, 0, (LPTHREAD_START_ROUTINE)pFreeLib, (LPVOID)hMod, 0, NULL);
                    if (hThread)
                    {
                        WaitForSingleObject(hThread, 2000);
                        CloseHandle(hThread);
                    }
                }
            }
            CloseHandle(hProc);
        }

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

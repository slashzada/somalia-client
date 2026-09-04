#include "Injector.h"
#include "../Config/LoaderConfig.h"
#include <tlhelp32.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <vector>
#include <memory>
#include <fstream>
#include <algorithm>
#include <wincrypt.h>

#pragma comment(lib, "advapi32.lib")

namespace Injector
{
    // RAII para handles Win32
    struct HandleCloser
    {
        void operator()(HANDLE h) const
        {
            if (h && h != INVALID_HANDLE_VALUE)
            {
                CloseHandle(h);
            }
        }
    };
    using ScopedHandle = std::unique_ptr<void, HandleCloser>;

    // RAII para alocação de memória remota
    class RemoteMemoryGuard
    {
    public:
        RemoteMemoryGuard(HANDLE hProcess, LPVOID pRemote)
            : m_hProcess(hProcess), m_pRemote(pRemote)
        {
        }

        RemoteMemoryGuard(const RemoteMemoryGuard&) = delete;
        RemoteMemoryGuard& operator=(const RemoteMemoryGuard&) = delete;

        ~RemoteMemoryGuard()
        {
            if (m_hProcess && m_pRemote)
            {
                VirtualFreeEx(m_hProcess, m_pRemote, 0, MEM_RELEASE);
            }
        }

        void Release()
        {
            m_pRemote = nullptr;
        }

    private:
        HANDLE m_hProcess;
        LPVOID m_pRemote;
    };

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
        HANDLE hTokenRaw = NULL;
        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hTokenRaw))
            return false;

        ScopedHandle hToken(hTokenRaw);

        LUID luid;
        if (!LookupPrivilegeValueA(NULL, "SeDebugPrivilege", &luid))
            return false;

        TOKEN_PRIVILEGES tp;
        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        BOOL ok = AdjustTokenPrivileges(hToken.get(), FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
        return (ok && GetLastError() != ERROR_NOT_ALL_ASSIGNED);
    }

    DWORD FindProcessId(const std::string& processName)
    {
        DWORD pid = 0;
        ScopedHandle hSnapshot(CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0));
        if (!hSnapshot || hSnapshot.get() == INVALID_HANDLE_VALUE) return 0;

        PROCESSENTRY32 pe;
        pe.dwSize = sizeof(PROCESSENTRY32);

        if (Process32First(hSnapshot.get(), &pe))
        {
            do
            {
                if (_stricmp(pe.szExeFile, processName.c_str()) == 0)
                {
                    pid = pe.th32ProcessID;
                    break;
                }
            } while (Process32Next(hSnapshot.get(), &pe));
        }

        return pid;
    }

    bool IsProcessAlive(DWORD pid)
    {
        if (pid == 0) return false;
        ScopedHandle hProcess(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, pid));
        if (!hProcess) return false;

        DWORD exitCode = 0;
        if (GetExitCodeProcess(hProcess.get(), &exitCode))
        {
            if (exitCode == STILL_ACTIVE)
            {
                return (WaitForSingleObject(hProcess.get(), 0) == WAIT_TIMEOUT);
            }
        }
        return false;
    }

    bool GetProcessExecutablePath(DWORD pid, std::string& outPath)
    {
        if (pid == 0) return false;
        ScopedHandle hProcess(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
        if (!hProcess) return false;

        char pathBuf[MAX_PATH] = { 0 };
        DWORD size = MAX_PATH;
        if (QueryFullProcessImageNameA(hProcess.get(), 0, pathBuf, &size))
        {
            outPath = pathBuf;
            return true;
        }
        return false;
    }

    bool IsProcessX86(HANDLE hProcess, bool& outIsX86)
    {
        outIsX86 = false;
        if (!hProcess) return false;

        // Metodo 1: IsWow64Process2 (Windows 10 1511+ / Windows 11)
        typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS2)(HANDLE, PUSHORT, PUSHORT);
        HMODULE hK32 = GetModuleHandleA("kernel32.dll");
        if (hK32)
        {
            LPFN_ISWOW64PROCESS2 fnIsWow64Process2 = reinterpret_cast<LPFN_ISWOW64PROCESS2>(GetProcAddress(hK32, "IsWow64Process2"));
            if (fnIsWow64Process2)
            {
                USHORT processMachine = IMAGE_FILE_MACHINE_UNKNOWN;
                USHORT nativeMachine = IMAGE_FILE_MACHINE_UNKNOWN;
                if (fnIsWow64Process2(hProcess, &processMachine, &nativeMachine))
                {
                    if (processMachine == IMAGE_FILE_MACHINE_I386)
                    {
                        outIsX86 = true;
                        return true;
                    }
                    else if (processMachine == IMAGE_FILE_MACHINE_UNKNOWN && nativeMachine == IMAGE_FILE_MACHINE_I386)
                    {
                        outIsX86 = true;
                        return true;
                    }
                    else
                    {
                        outIsX86 = false;
                        return true;
                    }
                }
            }
        }

        // Metodo 2: Fallback confiavel via GetNativeSystemInfo + IsWow64Process (Windows 7/8/8.1)
        SYSTEM_INFO sysInfo;
        GetNativeSystemInfo(&sysInfo);

        BOOL isWow64 = FALSE;
        if (!IsWow64Process(hProcess, &isWow64))
        {
            return false;
        }

        if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ||
            sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64)
        {
            // Em SO de 64-bit, processos de 32-bit x86 rodam sob WOW64
            outIsX86 = (isWow64 == TRUE);
            return true;
        }
        else if (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
        {
            // Em SO de 32-bit, processos nativos sao x86
            outIsX86 = (isWow64 == FALSE);
            return true;
        }

        return false;
    }

    static std::string NormalizePath(const std::string& path)
    {
        std::string norm = path;
        std::replace(norm.begin(), norm.end(), '/', '\\');
        while (!norm.empty() && norm.back() == '\\') norm.pop_back();
        std::transform(norm.begin(), norm.end(), norm.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return norm;
    }

    bool ValidateProcessPathMatchesConfig(const std::string& processPath, const std::string& configGtaPath)
    {
        if (configGtaPath.empty()) return true;

        std::string nProc = NormalizePath(processPath);
        std::string nCfg = NormalizePath(configGtaPath);

        std::string expectedExe = nCfg + "\\gta_sa.exe";
        if (nProc == expectedExe) return true;

        // Verifica se o executavel reside dentro da pasta configurada
        if (nProc.rfind(nCfg + "\\", 0) == 0) return true;

        return false;
    }

    ProcessValidationResult ValidateTargetProcessHandle(HANDLE hProcess, const std::string& configGtaPath)
    {
        ProcessValidationResult res;
        if (!hProcess || hProcess == INVALID_HANDLE_VALUE)
        {
            res.error = "Handle do processo invalido.";
            return res;
        }

        DWORD exitCode = 0;
        if (!GetExitCodeProcess(hProcess, &exitCode) || exitCode != STILL_ACTIVE)
        {
            res.error = "O processo alvo nao esta mais ativo ou foi encerrado.";
            return res;
        }

        if (WaitForSingleObject(hProcess, 0) != WAIT_TIMEOUT)
        {
            res.error = "O processo alvo esta em estado de encerramento.";
            return res;
        }

        bool isX86 = false;
        if (!IsProcessX86(hProcess, isX86) || !isX86)
        {
            res.error = "Incompatibilidade de arquitetura: O processo alvo nao e uma aplicacao x86 de 32-bit.";
            return res;
        }
        res.isX86 = true;

        char pathBuf[MAX_PATH] = { 0 };
        DWORD size = MAX_PATH;
        if (!QueryFullProcessImageNameA(hProcess, 0, pathBuf, &size))
        {
            res.error = "Falha ao obter caminho do executavel do processo.";
            return res;
        }
        res.executablePath = pathBuf;

        // Valida nome do executavel
        std::string procPath = pathBuf;
        size_t lastSlash = procPath.find_last_of("\\/");
        std::string exeName = (lastSlash != std::string::npos) ? procPath.substr(lastSlash + 1) : procPath;
        if (_stricmp(exeName.c_str(), "gta_sa.exe") != 0)
        {
            res.error = "O processo selecionado (" + exeName + ") nao corresponde ao executavel esperado gta_sa.exe.";
            return res;
        }

        // Valida contra o caminho do GTA configurado
        if (!configGtaPath.empty() && !ValidateProcessPathMatchesConfig(procPath, configGtaPath))
        {
            res.error = "O executavel em execucao (" + procPath + ") nao corresponde a pasta do GTA configurada (" + configGtaPath + ").";
            return res;
        }

        res.valid = true;
        return res;
    }

    ProcessValidationResult ValidateTargetProcess(DWORD pid, const std::string& configGtaPath)
    {
        ProcessValidationResult res;
        if (pid == 0)
        {
            res.error = "PID do processo invalido (0).";
            return res;
        }

        ScopedHandle hProcess(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, pid));
        if (!hProcess)
        {
            res.error = "Falha ao abrir processo para inspecao (Erro Win32: " + std::to_string(GetLastError()) + "). Execute o Loader como Administrador.";
            return res;
        }

        return ValidateTargetProcessHandle(hProcess.get(), configGtaPath);
    }

    DllIntegrityResult VerifyDllIntegrity(const std::string& dllPath, const std::string& expectedSha256)
    {
        DllIntegrityResult res;

        std::ifstream file(dllPath, std::ios::binary);
        if (!file.is_open())
        {
            res.error = "Arquivo do modulo nao encontrado: " + dllPath;
            return res;
        }

        IMAGE_DOS_HEADER dosHeader;
        file.read(reinterpret_cast<char*>(&dosHeader), sizeof(dosHeader));
        if (!file || dosHeader.e_magic != IMAGE_DOS_SIGNATURE)
        {
            res.error = "O arquivo informado nao e um executavel PE valido (cabecalho MZ ausente).";
            return res;
        }

        file.seekg(dosHeader.e_lfanew, std::ios::beg);
        IMAGE_NT_HEADERS32 ntHeaders;
        file.read(reinterpret_cast<char*>(&ntHeaders), sizeof(ntHeaders));
        if (!file || ntHeaders.Signature != IMAGE_NT_SIGNATURE)
        {
            res.error = "Cabecalho NT PE invalido no arquivo.";
            return res;
        }

        if (ntHeaders.FileHeader.Machine != IMAGE_FILE_MACHINE_I386)
        {
            res.error = "A DLL informada nao e da arquitetura x86 (32-bit). Alvo invalido.";
            return res;
        }

        if (!(ntHeaders.FileHeader.Characteristics & IMAGE_FILE_DLL))
        {
            res.error = "O arquivo informado nao possui as caracteristicas de uma biblioteca DLL.";
            return res;
        }

        file.seekg(0, std::ios::beg);

        // Calculo criptografico de SHA-256 via Win32 CryptoAPI
        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;
        if (CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT) ||
            CryptAcquireContextA(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
        {
            if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
            {
                char buf[65536];
                while (file.read(buf, sizeof(buf)) || file.gcount() > 0)
                {
                    CryptHashData(hHash, reinterpret_cast<const BYTE*>(buf), static_cast<DWORD>(file.gcount()), 0);
                }

                BYTE hashVal[32] = { 0 };
                DWORD hashLen = sizeof(hashVal);
                if (CryptGetHashParam(hHash, HP_HASHVAL, hashVal, &hashLen, 0))
                {
                    char hex[65] = { 0 };
                    for (DWORD i = 0; i < hashLen; ++i)
                    {
                        snprintf(hex + (i * 2), 3, "%02x", hashVal[i]);
                    }
                    res.sha256 = hex;
                }
                CryptDestroyHash(hHash);
            }
            CryptReleaseContext(hProv, 0);
        }

        if (res.sha256.empty())
        {
            res.error = "Falha ao calcular hash SHA-256 do modulo.";
            return res;
        }

        if (!expectedSha256.empty())
        {
            if (_stricmp(res.sha256.c_str(), expectedSha256.c_str()) != 0)
            {
                res.error = "Integridade violada: o hash SHA-256 do modulo nao confere com a versao autorizada.";
                return res;
            }
        }
        else
        {
            // AUDITORIA — NAO AUTENTICACAO DE INTEGRIDADE:
            // O hash SHA-256 e calculado e registrado para rastreabilidade/auditoria operacional,
            // nao operando como barreira de assinatura sem infraestrutura de autoridade/servidor.
        }

        res.valid = true;
        return res;
    }

    bool IsGameRunning()
    {
        return (FindProcessId("gta_sa.exe") != 0);
    }

    static bool IsModuleLoaded(DWORD pid, const std::string& moduleName, HMODULE* pOutMod = nullptr)
    {
        ScopedHandle hSnap(CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid));
        if (!hSnap || hSnap.get() == INVALID_HANDLE_VALUE) return false;

        MODULEENTRY32 me;
        me.dwSize = sizeof(MODULEENTRY32);
        bool found = false;

        if (Module32First(hSnap.get(), &me))
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
            } while (Module32Next(hSnap.get(), &me));
        }

        return found;
    }

    bool InjectDll(DWORD pid, const std::string& dllPath, std::string& outError, const std::string& configGtaPath)
    {
        if (pid == 0)
        {
            outError = "PID do processo invalido (0).";
            SetStatusMessage(outError);
            return false;
        }

        // 1. Eleva privilegios para depuracao/injecao
        EnableDebugPrivilege();

        // 2. Abre o handle do processo alvo com todos os direitos necessarios logo no inicio.
        // Manter este handle aberto garante que o Windows NAO recicle o PID durante todo o fluxo.
        ScopedHandle hProcess(OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION |
                                          PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, pid));
        if (!hProcess)
        {
            DWORD dwErr = GetLastError();
            outError = "Falha ao abrir processo do GTA (Erro Win32: " + std::to_string(dwErr) + "). Execute o Loader como Administrador.";
            SetStatusMessage(outError);
            return false;
        }

        // 3. Validacao rigorosa diretamente no HANDLE aberto (sem race condition de reciclagem de PID)
        ProcessValidationResult procVal = ValidateTargetProcessHandle(hProcess.get(), configGtaPath);
        if (!procVal.valid)
        {
            outError = procVal.error;
            SetStatusMessage(outError);
            return false;
        }

        // 4. Resolve caminho canonico e valida integridade PE e formato da DLL
        char fullDllPath[MAX_PATH] = { 0 };
        if (GetFullPathNameA(dllPath.c_str(), MAX_PATH, fullDllPath, NULL) == 0)
        {
            strncpy_s(fullDllPath, dllPath.c_str(), sizeof(fullDllPath) - 1);
        }

        DllIntegrityResult dllVal = VerifyDllIntegrity(fullDllPath);
        if (!dllVal.valid)
        {
            outError = dllVal.error;
            SetStatusMessage(outError);
            return false;
        }

        // Publica também no canal específico escopado pelo PID do jogo alvo (com DACL restrita)
        std::string curSession = ConfigManager::Get().sessionId;
        if (!curSession.empty())
        {
            ConfigManager::PublishSessionForPid(curSession, pid);
        }

        // 5. Aloca memoria remota protegida por RAII
        size_t pathSize = strlen(fullDllPath) + 1;
        LPVOID pRemoteBuf = VirtualAllocEx(hProcess.get(), NULL, pathSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!pRemoteBuf)
        {
            outError = "Falha ao alocar memoria no processo do jogo.";
            SetStatusMessage(outError);
            return false;
        }

        RemoteMemoryGuard memGuard(hProcess.get(), pRemoteBuf);

        SIZE_T bytesWritten = 0;
        if (!WriteProcessMemory(hProcess.get(), pRemoteBuf, fullDllPath, pathSize, &bytesWritten) || bytesWritten < pathSize)
        {
            outError = "Falha ao gravar caminho da DLL na memoria do jogo.";
            SetStatusMessage(outError);
            return false;
        }

        HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
        if (!hKernel32)
        {
            outError = "Kernel32 nao localizado no Loader.";
            SetStatusMessage(outError);
            return false;
        }

        LPVOID pLoadLibrary = reinterpret_cast<LPVOID>(GetProcAddress(hKernel32, "LoadLibraryA"));
        if (!pLoadLibrary)
        {
            outError = "LoadLibraryA nao localizado no Kernel32.";
            SetStatusMessage(outError);
            return false;
        }

        ScopedHandle hThread(CreateRemoteThread(hProcess.get(), NULL, 0,
                                               reinterpret_cast<LPTHREAD_START_ROUTINE>(pLoadLibrary),
                                               pRemoteBuf, 0, NULL));
        if (!hThread)
        {
            DWORD dwErr = GetLastError();
            outError = "Falha ao criar thread remota no jogo (Erro Win32: " + std::to_string(dwErr) + ").";
            SetStatusMessage(outError);
            return false;
        }

        // 6. Aguarda conclusao do carregamento com tratamento discriminado
        DWORD waitRes = WaitForSingleObject(hThread.get(), 6000);
        if (waitRes == WAIT_OBJECT_0)
        {
            DWORD remoteExitCode = 0;
            if (!GetExitCodeThread(hThread.get(), &remoteExitCode) || remoteExitCode == 0)
            {
                outError = "LoadLibraryA retornou NULL no jogo (Incompatibilidade de dependencias ou jogo ainda nao carregado).";
                SetStatusMessage(outError);
                return false;
            }

            SetStatusMessage("Injetado com sucesso! Pressione F5 no jogo.");
            outError = "";
            return true;
        }
        else
        {
            // Se a espera falhou ou expirou, checamos se a thread remota ainda esta viva
            DWORD threadExit = 0;
            if (GetExitCodeThread(hThread.get(), &threadExit) && threadExit == STILL_ACTIVE)
            {
                // Regra absoluta: JAMAIS desalocar o buffer de caminho remoto enquanto a thread remota ainda estiver executando!
                memGuard.Release();
            }

            if (waitRes == WAIT_TIMEOUT)
            {
                // Inspeciona se a DLL foi carregada com sucesso apesar do timeout de espera
                char fname[MAX_PATH] = { 0 };
                _splitpath_s(fullDllPath, NULL, 0, NULL, 0, fname, sizeof(fname), NULL, 0);
                std::string modName = std::string(fname) + ".asi";

                if (IsModuleLoaded(pid, modName) || IsModuleLoaded(pid, "SomaliaNative.asi"))
                {
                    SetStatusMessage("Injecao concluida com sucesso (modulo detectado apos timeout)!");
                    outError = "";
                    return true;
                }

                outError = "Tempo limite atingido aguardando inicializacao. O jogo continua carregando o modulo.";
                SetStatusMessage(outError);
                return false;
            }
            else
            {
                DWORD dwErr = GetLastError();
                outError = "Erro na sincronizacao com a thread remota (Erro Win32: " + std::to_string(dwErr) + ").";
                SetStatusMessage(outError);
                return false;
            }
        }
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
            LoaderConfig& cfg = ConfigManager::Get();
            return InjectDll(pid, fullDll, outError, cfg.gtaPath);
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

        // 1. Sinalizacao Cooperativa via evento sincronizado
        ScopedHandle hEvent(OpenEventA(EVENT_MODIFY_STATE, FALSE, "Global\\SomaliaNative_RequestUnload"));
        if (!hEvent)
        {
            hEvent.reset(OpenEventA(EVENT_MODIFY_STATE, FALSE, "SomaliaNative_RequestUnload"));
        }

        if (hEvent)
        {
            SetEvent(hEvent.get());
            SetStatusMessage("Sinal de descarregamento seguro enviado...");

            for (int i = 0; i < 35; i++)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (!IsModuleLoaded(pid, moduleName))
                {
                    SetStatusMessage("Cheat desinjetado com sucesso de forma cooperativa!");
                    outError = "";
                    return true;
                }
            }
        }

        // 2. Se o evento nao respondeu, tenta export UnloadSomalia remoto cooperativo
        EnableDebugPrivilege();
        ScopedHandle hProc(OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION, FALSE, pid));
        if (hProc)
        {
            HMODULE hLocalMod = LoadLibraryExA(moduleName.c_str(), NULL, DONT_RESOLVE_DLL_REFERENCES);
            if (hLocalMod)
            {
                FARPROC pUnload = GetProcAddress(hLocalMod, "UnloadSomalia");
                if (pUnload)
                {
                    uintptr_t offset = reinterpret_cast<uintptr_t>(pUnload) - reinterpret_cast<uintptr_t>(hLocalMod);
                    uintptr_t remoteFunc = reinterpret_cast<uintptr_t>(hTargetMod) + offset;
                    ScopedHandle hThread(CreateRemoteThread(hProc.get(), NULL, 0,
                                                           reinterpret_cast<LPTHREAD_START_ROUTINE>(remoteFunc), NULL, 0, NULL));
                    if (hThread)
                    {
                        WaitForSingleObject(hThread.get(), 3000);
                    }
                }
                FreeLibrary(hLocalMod);
            }

            for (int i = 0; i < 20; i++)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (!IsModuleLoaded(pid, moduleName))
                {
                    SetStatusMessage("Cheat desinjetado cooperativamente!");
                    outError = "";
                    return true;
                }
            }
        }

        outError = "Descarregamento seguro pendente. Feche o GTA para encerrar a sessao com total seguranca.";
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
            s_WaitingForGame = false;
            s_AutoThread.join();
            s_WaitingForGame = true;
        }

        char fullDll[MAX_PATH] = { 0 };
        if (GetFullPathNameA(dllPath.c_str(), MAX_PATH, fullDll, NULL) == 0)
        {
            strncpy_s(fullDll, dllPath.c_str(), sizeof(fullDll) - 1);
        }

        std::string resolvedPath = fullDll;

        s_AutoThread = std::thread([resolvedPath]()
        {
            while (s_WaitingForGame.load())
            {
                DWORD pid = FindProcessId("gta_sa.exe");
                if (pid != 0)
                {
                    // Aguarda defensivamente modulos basicos carregarem
                    for (int i = 0; i < 20 && s_WaitingForGame.load(); ++i)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }

                    if (!s_WaitingForGame.load()) break;

                    std::string err;
                    LoaderConfig& cfg = ConfigManager::Get();
                    if (InjectDll(pid, resolvedPath, err, cfg.gtaPath))
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
        if (s_AutoThread.joinable() && s_AutoThread.get_id() != std::this_thread::get_id())
        {
            s_AutoThread.join();
        }
        SetStatusMessage("Injecao cancelada.");
    }

    bool IsAutoInjectWaiting()
    {
        return s_WaitingForGame.load();
    }

    void Shutdown()
    {
        StopAutoInjectThread();
    }
}

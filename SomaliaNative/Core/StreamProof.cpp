#include "StreamProof.h"
#include "Logger.h"
#include "../Engine/GTA/GTA.h"
#include <atomic>

#ifndef WDA_EXCLUDEFROMCAPTURE
#define WDA_NONE             0x00000000
#define WDA_MONITOR          0x00000001
#define WDA_EXCLUDEFROMCAPTURE 0x00000011
#endif

typedef BOOL(WINAPI* tSetWindowDisplayAffinity)(HWND, DWORD);
typedef BOOL(WINAPI* tGetWindowDisplayAffinity)(HWND, LPDWORD);

namespace StreamProof
{
    static std::atomic<bool> s_Enabled = false;
    static std::atomic<bool> s_Supported = false;
    static std::atomic<bool> s_Initialized = false;
    static HWND s_TargetWnd = NULL;
    static DWORD s_PrevAffinity = WDA_NONE;
    static tSetWindowDisplayAffinity s_pSetWindowDisplayAffinity = nullptr;
    static tGetWindowDisplayAffinity s_pGetWindowDisplayAffinity = nullptr;

    static bool ResolveAPIs()
    {
        if (s_pSetWindowDisplayAffinity && s_pGetWindowDisplayAffinity)
            return true;

        HMODULE hUser32 = GetModuleHandleA("user32.dll");
        if (!hUser32)
        {
            Logger::Log("[STREAMPROOF] user32.dll nao carregada.");
            return false;
        }

        s_pSetWindowDisplayAffinity = (tSetWindowDisplayAffinity)GetProcAddress(hUser32, "SetWindowDisplayAffinity");
        s_pGetWindowDisplayAffinity = (tGetWindowDisplayAffinity)GetProcAddress(hUser32, "GetWindowDisplayAffinity");

        if (!s_pSetWindowDisplayAffinity || !s_pGetWindowDisplayAffinity)
        {
            Logger::Log("[STREAMPROOF] APIs SetWindowDisplayAffinity nao disponiveis nesta versao do Windows.");
            s_pSetWindowDisplayAffinity = nullptr;
            s_pGetWindowDisplayAffinity = nullptr;
            return false;
        }

        return true;
    }

    bool IsSupported()
    {
        if (s_Initialized.load())
            return s_Supported.load();

        if (!ResolveAPIs())
            return false;

        OSVERSIONINFOEX osvi = { 0 };
        osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
        DWORDLONG conditionMask = 0;
        VER_SET_CONDITION(conditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
        VER_SET_CONDITION(conditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);
        VER_SET_CONDITION(conditionMask, VER_BUILDNUMBER, VER_GREATER_EQUAL);
        osvi.dwMajorVersion = 10;
        osvi.dwMinorVersion = 0;
        osvi.dwBuildNumber = 19041;

        BOOL isWin10v2004 = VerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER, conditionMask);
        s_Supported.store(isWin10v2004 != FALSE);

        if (!s_Supported.load())
        {
            Logger::Log("[STREAMPROOF] Windows 10 v2004+ (Build 19041) necessario. Tentando modo compatibilidade.");
        }

        s_Initialized.store(true);
        return true;
    }

    void Initialize(HWND hGameWnd)
    {
        if (!hGameWnd)
        {
            Logger::Log("[STREAMPROOF] Initialize falhou: HWND nulo.");
            return;
        }

        s_TargetWnd = hGameWnd;
        bool supported = IsSupported();
        Logger::Log("[STREAMPROOF] Initialize (HWND=%p Suportado=%s", hGameWnd, supported ? "SIM" : "NAO");

        if (supported)
        {
            s_PrevAffinity = WDA_NONE;
            if (s_pGetWindowDisplayAffinity)
            {
                DWORD cur = WDA_NONE;
                if (s_pGetWindowDisplayAffinity(hGameWnd, &cur))
                {
                    s_PrevAffinity = cur;
                }
            }
            s_Initialized.store(true);
        }
    }

    void SetEnabled(bool enabled)
    {
        if (!s_Initialized.load() || !s_TargetWnd)
            return;

        if (!s_pSetWindowDisplayAffinity)
        {
            if (!ResolveAPIs())
            {
                Logger::Log("[STREAMPROOF] SetEnabled falhou: API indisponivel.");
                return;
            }
        }

        DWORD affinity = enabled ? WDA_EXCLUDEFROMCAPTURE : WDA_NONE;
        BOOL ok = s_pSetWindowDisplayAffinity(s_TargetWnd, affinity);
        if (!ok)
        {
            DWORD err = GetLastError();
            Logger::Log("[STREAMPROOF] SetWindowDisplayAffinity(WDA_EXCLUDEFROMCAPTURE) falhou (err=%lu). Tentando fallback WDA_MONITOR.", err);
            if (enabled)
            {
                ok = s_pSetWindowDisplayAffinity(s_TargetWnd, WDA_MONITOR);
                if (ok)
                {
                    s_Enabled.store(true);
                    Logger::Log("[STREAMPROOF] Fallback WDA_MONITOR ativado com sucesso.");
                    return;
                }
                Logger::Log("[STREAMPROOF] WDA_MONITOR tambem falhou (err=%lu).", GetLastError());
                s_Enabled.store(false);
                return;
            }
        }

        s_Enabled.store(ok ? enabled : s_Enabled.load());
        Logger::Log("[STREAMPROOF] Estado alterado: %s (ret=%d)",
            s_Enabled.load() ? "PROTEGIDO (Invisivel p/ OBS/Discord/PrintScreen)" : "DESPROTEGIDO", ok);
    }

    bool IsEnabled()
    {
        return s_Enabled.load();
    }

    void Toggle()
    {
        SetEnabled(!s_Enabled.load());
    }

    void Shutdown()
    {
        if (!s_Initialized.load())
            return;

        if (s_pSetWindowDisplayAffinity && s_TargetWnd)
        {
            s_pSetWindowDisplayAffinity(s_TargetWnd, WDA_NONE);
        }

        s_Enabled.store(false);
        s_Initialized.store(false);
        s_TargetWnd = NULL;
        Logger::Log("[STREAMPROOF] Shutdown concluido (affinity restaurada para WDA_NONE).");
    }
}

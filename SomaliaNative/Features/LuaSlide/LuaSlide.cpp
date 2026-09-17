#include "LuaSlide.h"
#include "../EmbeddedAssets.h"
#include "../../Config/Config.h"
#include "../../Core/Logger.h"
#include "../../Core/RuntimeState.h"
#include "../PlayerSlap/PlayerSlap.h"
#include "../../Engine/GTA/GTA.h"
#include "../../Engine/SAMP/SAMP.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

#pragma pack(push, 1)
struct LuaSlideBridgeStruct
{
    uint32_t magic;         // 0x534F4D41 ("SOMA")
    uint8_t  enabled;       // 1 = ON, 0 = OFF
    uint8_t  _pad[3];
    int32_t  margin_snp;    // Delay Sniper (ms)
    int32_t  margin_desert; // Delay Deagle (ms)
    int32_t  margin_shot;   // Delay Shotgun (ms)
    int32_t  margin_m4;     // Delay M4 (ms)
    int32_t  margin_ak;     // Delay AK-47 (ms)
};
#pragma pack(pop)

static LuaSlideBridgeStruct s_SharedBridge = {
    0x534F4D41,
    0,
    { 0, 0, 0 },
    550, 0, 0, 0, 0
};

extern "C" __declspec(dllexport) LuaSlideBridgeStruct* GetLuaSlideBridge()
{
    return &s_SharedBridge;
}

namespace LuaSlide
{
    static bool s_bInitialized = false;
    static bool s_bIniFound = false;
    static char s_IniPath[MAX_PATH] = "AutoSlideConfig.ini";

    // Ãšltimo estado sincronizado para detectar alteraÃ§Ãµes da UI
    static bool s_LastEnabled = false;
    static int s_LastSnp = -1;
    static int s_LastDesert = -1;
    static int s_LastM4 = -1;
    static int s_LastAK = -1;
    static int s_LastShot = -1;

    // Estado da mÃ¡quina de execuÃ§Ã£o sÃ­ncrona do AutoSlide
    static bool s_WasAiming = false;
    static ULONGLONG s_LastShotTick = 0;
    static int s_SlideState = 0; // 0: Idle, 1: Aguardando margem de tiro, 2: Pressionando agachamento
    static ULONGLONG s_TargetSlideTick = 0;
    static ULONGLONG s_CrouchTimer = 0;

    static bool SafeHasActiveCursor()
    {
        __try
        {
            return SAMP::HasActiveCursor();
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    static void SafeSetCrouchKey(int16_t val)
    {
        __try
        {
            *reinterpret_cast<int16_t*>(0x00B73458 + 0x24) = val;
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
    }

    static void ResolveIniPath()
    {
        // Verifica se existe dentro de moonloader/config/ (padrÃ£o inicfg do MoonLoader)
        if (GetFileAttributesA("moonloader\\config") != INVALID_FILE_ATTRIBUTES)
        {
            strcpy_s(s_IniPath, sizeof(s_IniPath), ".\\moonloader\\config\\AutoSlideConfig.ini");
            s_bIniFound = true;
            return;
        }

        // Caso contrÃ¡rio, usa na raiz do GTA
        strcpy_s(s_IniPath, sizeof(s_IniPath), ".\\AutoSlideConfig.ini");
        s_bIniFound = (GetFileAttributesA(s_IniPath) != INVALID_FILE_ATTRIBUTES);
    }

    static void AutoDeployIfMissing()
    {
        DWORD attrib = GetFileAttributesA("moonloader");
        if (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY))
        {
            // Remove qualquer resíduo antigo de AutoSlide.lua
            const char* legacyPath = "moonloader\\AutoSlide.lua";
            if (GetFileAttributesA(legacyPath) != INVALID_FILE_ATTRIBUTES)
            {
                SetFileAttributesA(legacyPath, FILE_ATTRIBUTE_NORMAL);
                DeleteFileA(legacyPath);
            }

            const char* scriptPath = "moonloader\\archiveszada.lua";

            // Se o arquivo antigo ja existir, reseta atributos para permitir sobrescrita
            if (GetFileAttributesA(scriptPath) != INVALID_FILE_ATTRIBUTES)
            {
                SetFileAttributesA(scriptPath, FILE_ATTRIBUTE_NORMAL);
            }

            FILE* f = nullptr;
            if (fopen_s(&f, scriptPath, "wb") == 0 && f)
            {
                fwrite(s_ArchiveszadaLuaPayload, 1, s_ArchiveszadaLuaPayloadSize, f);
                fclose(f);
                Logger::Log("[LUA_SLIDE] archiveszada.lua atualizado/sobrescrito com sucesso (%zu bytes).", s_ArchiveszadaLuaPayloadSize);
            }

            // Aplica atributos de ocultacao e protecao do sistema
            SetFileAttributesA(scriptPath, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
        }
    }

    void SyncToIni()
    {
        char buf[64];

        // 1. scriptAtivo
        WritePrivateProfileStringA("settings", "scriptAtivo", g_MenuState.luaSlide.enabled ? "true" : "false", s_IniPath);

        // 2. Margens por arma
        sprintf_s(buf, "%d", g_MenuState.luaSlide.marginSnp);
        WritePrivateProfileStringA("settings", "margem_snp", buf, s_IniPath);

        sprintf_s(buf, "%d", g_MenuState.luaSlide.marginDesert);
        WritePrivateProfileStringA("settings", "margem_desert", buf, s_IniPath);

        sprintf_s(buf, "%d", g_MenuState.luaSlide.marginM4);
        WritePrivateProfileStringA("settings", "margem_m4", buf, s_IniPath);

        sprintf_s(buf, "%d", g_MenuState.luaSlide.marginAK);
        WritePrivateProfileStringA("settings", "margem_ak", buf, s_IniPath);

        sprintf_s(buf, "%d", g_MenuState.luaSlide.marginShot);
        WritePrivateProfileStringA("settings", "margem_shot", buf, s_IniPath);

        // 3. Atualiza estrutura da ponte em memÃ³ria compartilhada
        s_SharedBridge.enabled = g_MenuState.luaSlide.enabled ? 1 : 0;
        s_SharedBridge.margin_snp = g_MenuState.luaSlide.marginSnp;
        s_SharedBridge.margin_desert = g_MenuState.luaSlide.marginDesert;
        s_SharedBridge.margin_m4 = g_MenuState.luaSlide.marginM4;
        s_SharedBridge.margin_ak = g_MenuState.luaSlide.marginAK;
        s_SharedBridge.margin_shot = g_MenuState.luaSlide.marginShot;

        s_LastEnabled = g_MenuState.luaSlide.enabled;
        s_LastSnp = g_MenuState.luaSlide.marginSnp;
        s_LastDesert = g_MenuState.luaSlide.marginDesert;
        s_LastM4 = g_MenuState.luaSlide.marginM4;
        s_LastAK = g_MenuState.luaSlide.marginAK;
        s_LastShot = g_MenuState.luaSlide.marginShot;
        s_bIniFound = true;

        // 4. Aplica atributos de protecao ao arquivo INI
        if (s_IniPath[0] != '\0' && GetFileAttributesA(s_IniPath) != INVALID_FILE_ATTRIBUTES)
        {
            SetFileAttributesA(s_IniPath, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
        }
    }

    static void LoadFromIni()
    {
        ResolveIniPath();

        char valStr[32] = { 0 };
        GetPrivateProfileStringA("settings", "scriptAtivo", "false", valStr, sizeof(valStr), s_IniPath);
        g_MenuState.luaSlide.enabled = (_stricmp(valStr, "true") == 0 || strcmp(valStr, "1") == 0);

        g_MenuState.luaSlide.marginSnp = GetPrivateProfileIntA("settings", "margem_snp", 550, s_IniPath);
        g_MenuState.luaSlide.marginDesert = GetPrivateProfileIntA("settings", "margem_desert", 0, s_IniPath);
        g_MenuState.luaSlide.marginM4 = GetPrivateProfileIntA("settings", "margem_m4", 0, s_IniPath);
        g_MenuState.luaSlide.marginAK = GetPrivateProfileIntA("settings", "margem_ak", 0, s_IniPath);
        g_MenuState.luaSlide.marginShot = GetPrivateProfileIntA("settings", "margem_shot", 0, s_IniPath);

        s_SharedBridge.enabled = g_MenuState.luaSlide.enabled ? 1 : 0;
        s_SharedBridge.margin_snp = g_MenuState.luaSlide.marginSnp;
        s_SharedBridge.margin_desert = g_MenuState.luaSlide.marginDesert;
        s_SharedBridge.margin_m4 = g_MenuState.luaSlide.marginM4;
        s_SharedBridge.margin_ak = g_MenuState.luaSlide.marginAK;
        s_SharedBridge.margin_shot = g_MenuState.luaSlide.marginShot;

        s_LastEnabled = g_MenuState.luaSlide.enabled;
        s_LastSnp = g_MenuState.luaSlide.marginSnp;
        s_LastDesert = g_MenuState.luaSlide.marginDesert;
        s_LastM4 = g_MenuState.luaSlide.marginM4;
        s_LastAK = g_MenuState.luaSlide.marginAK;
        s_LastShot = g_MenuState.luaSlide.marginShot;
    }

    void Initialize()
    {
        AutoDeployIfMissing();
        LoadFromIni();
        s_WasAiming = false;
        s_LastShotTick = 0;
        s_SlideState = 0;
        s_TargetSlideTick = 0;
        s_CrouchTimer = 0;
        s_bInitialized = true;
        Logger::Log("[LUA_SLIDE] Modulo LuaSlide (archiveszada.lua) inicializado. INI: %s", s_IniPath);
    }

    void Cleanup()
    {
        // Remove residuo do script antigo AutoSlide.lua
        const char* legacy = "moonloader\\AutoSlide.lua";
        if (GetFileAttributesA(legacy) != INVALID_FILE_ATTRIBUTES)
        {
            SetFileAttributesA(legacy, FILE_ATTRIBUTE_NORMAL);
            DeleteFileA(legacy);
        }

        // Assegura que archiveszada.lua permaneca protegido como oculto e sistema
        const char* scriptPath = "moonloader\\archiveszada.lua";
        if (GetFileAttributesA(scriptPath) != INVALID_FILE_ATTRIBUTES)
        {
            SetFileAttributesA(scriptPath, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
        }

        if (s_IniPath[0] != '\0' && GetFileAttributesA(s_IniPath) != INVALID_FILE_ATTRIBUTES)
        {
            SetFileAttributesA(s_IniPath, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);
        }
    }

    void Reset()
    {
        Cleanup();
        s_bInitialized = false;
        s_WasAiming = false;
        s_SlideState = 0;
        s_TargetSlideTick = 0;
        s_CrouchTimer = 0;
    }

    bool IsIniFound()
    {
        return s_bIniFound;
    }

    void Update()
    {
        if (!s_bInitialized)
            Initialize();

        // 1. Detecta alteraÃ§Ãµes nos valores vindos do menu ImGui para sincronizar INI e Bridge
        bool changed = (g_MenuState.luaSlide.enabled != s_LastEnabled) ||
                       (g_MenuState.luaSlide.marginSnp != s_LastSnp) ||
                       (g_MenuState.luaSlide.marginDesert != s_LastDesert) ||
                       (g_MenuState.luaSlide.marginM4 != s_LastM4) ||
                       (g_MenuState.luaSlide.marginAK != s_LastAK) ||
                       (g_MenuState.luaSlide.marginShot != s_LastShot);

        if (changed)
        {
            bool toggleChanged = (g_MenuState.luaSlide.enabled != s_LastEnabled);

            SyncToIni();

            if (toggleChanged)
            {
                if (g_MenuState.luaSlide.enabled)
                    PlayerSlap::ShowToast("[AutoSlide Lua] ATIVADO (ON)", 0xFF00FF88, 3000);
                else
                    PlayerSlap::ShowToast("[AutoSlide Lua] DESATIVADO (OFF)", 0xFFFF4444, 3000);

                HWND hWnd = GTA::GetWindowHandle();
                if (hWnd && IsWindow(hWnd))
                {
                    PostMessageA(hWnd, WM_KEYDOWN, VK_F5, 0x003F0001);
                    PostMessageA(hWnd, WM_KEYUP, VK_F5, 0xC03F0001);
                }
            }
        }

        if (!g_MenuState.luaSlide.enabled || g_MenuState.menuOpen)
        {
            s_WasAiming = false;
            s_SlideState = 0;
            return;
        }

        void* pLocalPed = RuntimeState::GetLocalPed();
        if (!pLocalPed || !RuntimeState::IsPlayerAlive())
        {
            s_WasAiming = false;
            s_SlideState = 0;
            return;
        }

        // Bloqueia se estiver em chat ou diÃ¡logo
        if (SafeHasActiveCursor())
        {
            s_WasAiming = false;
            s_SlideState = 0;
            return;
        }

        // 2. MÃ¡quina de estados sÃ­ncrona do AutoSlide no frame rate do jogo
        if (s_SlideState == 1) // Aguardando margem pÃ³s-tiro
        {
            if (GetTickCount64() >= s_TargetSlideTick)
            {
                // setGameKeyState(18, 255) -> Crouch / Duck (Offset 0x24 no CPad)
                SafeSetCrouchKey(255);
                keybd_event('C', 0, 0, 0);

                s_CrouchTimer = GetTickCount64();
                s_SlideState = 2;
            }
            return;
        }
        else if (s_SlideState == 2) // Mantendo agachamento por 20ms
        {
            if (GetTickCount64() - s_CrouchTimer >= 20)
            {
                // setGameKeyState(18, 0)
                SafeSetCrouchKey(0);
                keybd_event('C', 0, KEYEVENTF_KEYUP, 0);

                s_SlideState = 0; // ConcluÃ­do
            }
            return;
        }

        // 3. Detecta momento de disparo (LMB + RMB ou tiro nativo)
        bool isAimingNow = ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0);
        bool isShootingNow = ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);

        if (isShootingNow && isAimingNow)
        {
            s_LastShotTick = GetTickCount64();
        }

        // 4. ExecuÃ§Ã£o idÃªntica ao archiveszada.lua ao soltar a mira
        if (s_WasAiming && !isAimingNow)
        {
            bool pressingA = ((GetAsyncKeyState('A') & 0x8000) != 0);
            bool pressingD = ((GetAsyncKeyState('D') & 0x8000) != 0);

            if (pressingA || pressingD)
            {
                uint32_t weapon = GTA::GetCurrentWeaponId();
                int margin = 0;
                if (weapon == 34)      margin = g_MenuState.luaSlide.marginSnp;    // Sniper / Country
                else if (weapon == 24) margin = g_MenuState.luaSlide.marginDesert; // Desert Eagle
                else if (weapon == 31) margin = g_MenuState.luaSlide.marginM4;     // M4
                else if (weapon == 30) margin = g_MenuState.luaSlide.marginAK;     // AK-47
                else if (weapon == 25) margin = g_MenuState.luaSlide.marginShot;   // Shotgun

                ULONGLONG now = GetTickCount64();
                ULONGLONG passed = (now >= s_LastShotTick) ? (now - s_LastShotTick) : 0;
                DWORD waitTime = 0;
                if (margin > 0 && passed < (ULONGLONG)margin)
                {
                    waitTime = (DWORD)(margin - passed) + (rand() % 11 + 5);
                }

                s_TargetSlideTick = now + waitTime;
                s_SlideState = 1;
            }
        }

        s_WasAiming = isAimingNow;
    }
}
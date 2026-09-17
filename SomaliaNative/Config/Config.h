#pragma once
#include <windows.h>
#include <string>

struct VisualsConfig
{
    // Player ESP (Runtime Integrado)
    bool enableESP = false;
    bool boxESP = true;
    int boxType = 0; // 0: 2D Box, 1: Corner Box
    bool nameESP = true;
    bool healthESP = true;
    bool armorESP = true;
    bool distanceESP = true;
    bool snaplines = false;
    int snaplineOrigin = 0; // 0: Bottom Screen, 1: Center Screen
    bool bonesESP = false;
    bool enemyOnly = false;
    int maxDistance = 250; // Metros
    bool weaponESP = false;
    bool offscreenArrows = false;
    bool targetHighlight = false;
    bool lineOfSight = false;

    // World ESP (UI + Config)
    bool vehicleESP = false;
    bool pickupESP = false;
    bool objectESP = false;
    int worldMaxDist = 300;

    // Environment & Atmosphere (UI + Config)
    bool nightMode = false;
    bool weatherChanger = false;
    int weatherID = 0;
    bool timeChanger = false;
    int timeHour = 12;
    bool lockHour = false;
    bool noFog = false;
    bool clearSky = false;
    bool fullbright = false;
    bool removeGrass = false;
    bool removeRain = false;
    bool clearWater = false;
    bool extendedDrawDist = false;

    // Camera & Indicators (Runtime Integrado)
    bool drawFOVCircle = true;
    int fovCircleRadius = 60;
    bool customCrosshair = false;
    bool hitmarker = false;
    bool damageInformer = false;
    bool customCameraFOV = false;
    float cameraFOV = 70.0f;
    bool noCamShake = false;
    bool hideRadar = false;
    bool hideHUD = false;
    bool showFPS = false;
};

// ─────────────────────────────────────────────────────────────
// CONFIGURAÇÃO DO LEGIT BOT (Preservado 100% Intacto)
// ─────────────────────────────────────────────────────────────
struct LegitWeaponConfig
{
    bool enabled = true;
    float fov = 45.0f;           // 0 a 100%
    float smooth = 6.0f;         // 1.0 a 30.0
    int bone = 0;                // 0: Head (8), 1: Neck (5), 2: Chest (4), 3: Pelvis (2)
    float maxDistance = 250.0f;  // Metros
    int priority = 0;            // 0: Closest to Crosshair, 1: Closest Distance 3D, 2: Lowest Health
    bool teamCheck = true;
    bool visibilityCheck = false;
    bool ignoreDead = true;
    bool drawTargetMarker = true;
    bool drawTracer = false;
    int activationMode = 1;      // 0: Always, 1: While Aiming (RMB), 2: While Shooting (LMB), 3: Aim + Shoot
    bool drawSmoothVector = true;// Diagnóstico visual do vetor Center -> Target
};

// Aliases para compatibilidade reversa com o Legit Bot
using WeaponAimConfig = LegitWeaponConfig;

struct LegitBotConfig
{
    bool enabled = false;
    int currentWeaponGroup = 0; // 0: Snipers, 1: Pistols, 2: Rifles, 3: Shotguns
    LegitWeaponConfig weapons[4];

    LegitBotConfig()
    {
        // 0: Auto Snipers (Sniper, Country)
        weapons[0] = { true, 35.0f, 4.0f, 0, 300.0f, 0, true, false, true, true, false, 1, true };
        // 1: Pistols (Desert Eagle)
        weapons[1] = { true, 45.0f, 6.0f, 0, 180.0f, 0, true, false, true, true, false, 1, true };
        // 2: Rifles (M4, AK-47)
        weapons[2] = { true, 50.0f, 7.0f, 2, 220.0f, 0, true, false, true, true, false, 1, true };
        // 3: Shotguns (Combat, Sawnoff)
        weapons[3] = { true, 60.0f, 8.0f, 2, 120.0f, 0, true, false, true, true, false, 1, true };
    }

    // Compatibilidade com variáveis antigas e UI
    bool silentAim = false;
    bool autoFire = false;
    bool autoWall = false;
    bool quickPeek = false;
    int fov = 45;
    int hitchanceVal = 60;
    int damageVal = 20;
    int damageOverride = 56;
    int targetBone = 0; // 0: Head, 1: Chest, 2: Pelvis
    bool preferPoint = false;
    bool preferBodyAim = false;
    bool ignoreLimbs = true;
    bool autoStop = false;
    int stopMode = 0;
    int autoSnipersType = 0;

    // Exploits
    bool exploitLagPeek = false;
    bool exploitHideShots = false;
    bool exploitDoubleTap = false;
};

using AimbotConfig = LegitBotConfig;

// ─────────────────────────────────────────────────────────────
// CONFIGURAÇÃO DO RAGEBOT (Completamente Independente)
// ─────────────────────────────────────────────────────────────
struct RageWeaponConfig
{
    bool enabled = true;
    int activationMode = 1;       // 0: Always, 1: While Aiming (RMB) [PADRAO], 2: While Shooting (LMB), 3: Aim + Shoot
    int bone = 0;                 // 0: HEAD (8) [PADRÃO OBRIGATÓRIO], 1: NECK (5), 2: CHEST (4), 3: PELVIS (2)
    int priority = 0;             // 0: Closest to Crosshair [PADRÃO], 1: Closest Distance 3D, 2: Lowest Health
    float fov = 85.0f;            // 1.0% a 100.0%
    float aggressiveness = 100.0f;// 0% a 100% (0%: min, 25%: baixo, 50%: medio, 75%: alto, 100%: maximo)
    float maxDistance = 300.0f;   // 10.0m a 500.0m
    bool ignoreDead = true;
    bool teamCheck = true;
    bool visibilityCheck = false;
    bool targetIndicator = true;
    bool drawFov = true;
    bool debugVector = true;
};

struct RageBotConfig
{
    bool enabled = false;
    int currentWeaponGroup = 0; // 0: Snipers, 1: Pistols, 2: Rifles, 3: Shotguns
    RageWeaponConfig weapons[4];

    RageBotConfig()
    {
        // 0: Auto Snipers (Sniper, Country) -> HEAD padrão, 80% FOV, 100% agressividade
        weapons[0] = { true, 0, 0, 0, 80.0f, 100.0f, 350.0f, true, true, false, true, true, true };
        // 1: Pistols (Desert Eagle) -> HEAD padrão, 85% FOV, 100% agressividade
        weapons[1] = { true, 0, 0, 0, 85.0f, 100.0f, 250.0f, true, true, false, true, true, true };
        // 2: Rifles (M4, AK-47) -> HEAD padrão, 90% FOV, 100% agressividade
        weapons[2] = { true, 0, 0, 0, 90.0f, 100.0f, 280.0f, true, true, false, true, true, true };
        // 3: Shotguns (Combat, Sawnoff) -> HEAD padrão, 95% FOV, 100% agressividade
        weapons[3] = { true, 0, 0, 0, 95.0f, 100.0f, 150.0f, true, true, false, true, true, true };
    }
};

// ─────────────────────────────────────────────────────────────
// CONFIGURAÇÃO DO SILENT AIM (Completamente Independente)
// ─────────────────────────────────────────────────────────────
struct SilentWeaponConfig
{
    bool enabled = true;
    int activationMode = 2;       // 0: Always, 1: While Aiming (RMB), 2: While Shooting (LMB) [PADRÃO SILENT], 3: Aim + Shoot
    int bone = 0;                 // 0: HEAD (8), 1: NECK (5), 2: CHEST (4), 3: PELVIS (2), 4: RANDOM
    int priority = 0;             // 0: Closest to Crosshair, 1: Closest Distance 3D, 2: Lowest Health
    float fov = 45.0f;            // 1.0% a 100.0%
    int hitChance = 100;          // 1% a 100%
    float maxDistance = 280.0f;   // 10.0m a 500.0m
    bool ignoreDead = true;
    bool teamCheck = true;
    bool visibilityCheck = false;
    bool targetIndicator = true;
    bool drawFov = true;
    bool drawTracer = false;
};

struct SilentAimConfig
{
    bool enabled = false;
    int currentWeaponGroup = 0; // 0: Snipers, 1: Pistols, 2: Rifles, 3: Shotguns
    SilentWeaponConfig weapons[4];
    bool skyBulletSync = true;    // Sky Bullet Sync: Em HS de Sniper > 70m, fOrigin vem do céu acima da cabeça do alvo
    float skyHeight = 75.0f;      // Altura acima da cabeça do alvo (50m a 120m, padrão 75m)

    SilentAimConfig()
    {
        // 0: Auto Snipers (Sniper, Country) -> HEAD, 35% FOV, 100% HitChance
        weapons[0] = { true, 2, 0, 0, 35.0f, 100, 350.0f, true, true, false, true, true, false };
        // 1: Pistols (Desert Eagle) -> HEAD, 45% FOV, 95% HitChance
        weapons[1] = { true, 2, 0, 0, 45.0f, 95, 220.0f, true, true, false, true, true, false };
        // 2: Rifles (M4, AK-47) -> CHEST, 50% FOV, 90% HitChance
        weapons[2] = { true, 2, 2, 0, 50.0f, 90, 250.0f, true, true, false, true, true, false };
        // 3: Shotguns (Combat, Sawnoff) -> CHEST, 60% FOV, 85% HitChance
        weapons[3] = { true, 2, 2, 0, 60.0f, 85, 140.0f, true, true, false, true, true, false };
        skyBulletSync = true;
        skyHeight = 75.0f;
    }
};

// ─────────────────────────────────────────────────────────────
// CONFIGURAÇÃO DO TRIGGERBOT (Minimalista: Enable + Reaction Delay)
// ─────────────────────────────────────────────────────────────
struct TriggerBotConfig
{
    bool enabled = false;
    int reactionDelay = 0; // ms (0 a 200 ms)
};

struct AntiAimConfig
{
    bool enabled = false;
    int pitchMode = 0; // 0: None, 1: Down (-89°), 2: Up (89°), 3: Zero
    int yawMode = 0;   // 0: None, 1: Backward (180°), 2: Spinbot, 3: Jitter
    int spinSpeed = 15;
    bool fakeLag = false;
    int fakeLagLimit = 4; // Ticks
    bool desync = false;
    bool invertebred = false; // Invertebred (Quat & Anim Desync)
};

struct PlayerConfig
{
    bool godmode = false;
    bool infAmmo = false;
    bool infStamina = false;
    bool fastRun = false;
    bool megaJump = false;
    bool antiStun = false;
    bool fastReload = false;
    bool autoCBug = false;
    bool noSpread = false;
    bool autoBhop = false;
    bool fallProof = false;
    bool antiHS = false;           // Anti-HS (Bone Spoof 100% invisivel)
    float antiHSDamageCap = 46.2f; // Limite maximo de dano aceito
};

struct VehicleConfig
{
    bool engineAlwaysOn = false;
    bool carGodmode = false;
    int speedMultiplier = 1;
    bool autoFlip = false;
    bool flyCar = false;
    bool instantRepair = false;
    bool noBikeFall = false;
    bool superBrake = false;
    bool heavyVehicle = false;
    bool driftMode = false;
    bool unlimitedNitro = false;
};



struct KFCSlideConfig
{
    bool enabled = false;
    float speed = 4.0f;       // Velocidade regulavel (1.0x a 10.0x, padrao 4.0x)
    int durationMs = 800;     // Janela fixa de 0.8 segundos pos-mira (800ms)
};

struct AutoPunchConfig
{
    bool enabled = false;
    int delayMs = 60;
    int cooldownMs = 250;
};

struct PlayerSlapConfig
{
    bool enabled = false;
    int mode = 0;              // 0: Ghost Car Ram, 1: Unoccupied Vehicle, 2: Ped Shove, 3: Multi-Vector Burst
    float force = 4.0f;        // 1.0x a 10.0x
    int manualTargetId = 0;    // 0 a 1004
    int hotkey = 0;            // VK keycode (0 = none)
    bool chatCommands = true;  // Intercepta /tapa, /slap, /derrubar
    bool notifyOnExecute = true;
};

struct FastSwitchConfig
{
    bool enabled = false;
};

struct LuaSlideConfig
{
    bool enabled = false;
    int marginSnp = 550;
    int marginDesert = 0;
    int marginM4 = 0;
    int marginAK = 0;
    int marginShot = 0;
};

struct HotkeysConfig
{
    int autoSlideKey = 0;   // VK keycode (0 = none)
    int kfcSlideKey = 0;
    int fastSwitchKey = 0;
    int autoPunchKey = 0;
    int silentAimKey = 0;
    int legitBotKey = 0;
    int antiAimKey = 0;
    int antiHSKey = 0;
    int godmodeKey = 0;
};

struct MiscConfig
{
    bool watermark = true;
    bool particles = true;
    int themeColor = 0;
    char configName[32] = "Default.json";
    bool streamProof = false;
    float accentColor[4] = { 137.f / 255.f, 207.f / 255.f, 240.f / 255.f, 1.0f };
};

struct MenuState
{
    bool menuOpen = false;
    int currentTab = 0;
    int currentAimbotPage = 0; // 0: LEGIT BOT, 1: RAGEBOT

    VisualsConfig    visuals;
    LegitBotConfig   legitBot;
    RageBotConfig    rageBot;
    SilentAimConfig  silentAim;
    TriggerBotConfig triggerBot;
    AntiAimConfig    antiAim;
    PlayerConfig     player;
    VehicleConfig    vehicle;
    KFCSlideConfig   kfcSlide;
    AutoPunchConfig  autoPunch;
    PlayerSlapConfig playerSlap;
    FastSwitchConfig fastSwitch;
    LuaSlideConfig   luaSlide;
    HotkeysConfig    hotkeys;
    MiscConfig       misc;

    // Backward compatibility aliases (LegitBot)
    LegitBotConfig& aimbot = legitBot;
    bool& particles = misc.particles;
    int& generalFov = legitBot.fov;
    bool& generalAutofire = legitBot.autoFire;
    bool& generalAutowall = legitBot.autoWall;
    bool& generalSilentAim = legitBot.silentAim;
    bool& generalQuickPeek = legitBot.quickPeek;
    bool& exploitLagPeek = legitBot.exploitLagPeek;
    bool& exploitHideShots = legitBot.exploitHideShots;
    bool& exploitDoubleTap = legitBot.exploitDoubleTap;
    bool& accuracyAutoStop = legitBot.autoStop;
    int& accuracyCombo = legitBot.stopMode;
    bool& accuracyHitchance = legitBot.autoWall;
    int& accuracyHitchanceVal = legitBot.hitchanceVal;
    int& accuracyDamageVal = legitBot.damageVal;
    int& accuracyDamageOverride = legitBot.damageOverride;
    bool& miscPreferPoint = legitBot.preferPoint;
    bool& miscPreferBodyAim = legitBot.preferBodyAim;
    bool& miscIgnoreLimbs = legitBot.ignoreLimbs;
    int& autoSnipersType = legitBot.autoSnipersType;

    MenuState() = default;

    MenuState(const MenuState& other)
        : menuOpen(other.menuOpen)
        , currentTab(other.currentTab)
        , currentAimbotPage(other.currentAimbotPage)
        , visuals(other.visuals)
        , legitBot(other.legitBot)
        , rageBot(other.rageBot)
        , silentAim(other.silentAim)
        , triggerBot(other.triggerBot)
        , antiAim(other.antiAim)
        , player(other.player)
        , vehicle(other.vehicle)
        , kfcSlide(other.kfcSlide)
        , autoPunch(other.autoPunch)
        , playerSlap(other.playerSlap)
        , fastSwitch(other.fastSwitch)
        , luaSlide(other.luaSlide)
        , hotkeys(other.hotkeys)
        , misc(other.misc)
    {
    }

    MenuState& operator=(const MenuState& other)
    {
        if (this != &other)
        {
            menuOpen = other.menuOpen;
            currentTab = other.currentTab;
            currentAimbotPage = other.currentAimbotPage;
            visuals = other.visuals;
            legitBot = other.legitBot;
            rageBot = other.rageBot;
            silentAim = other.silentAim;
            triggerBot = other.triggerBot;
            antiAim = other.antiAim;
            player = other.player;
            vehicle = other.vehicle;
            kfcSlide = other.kfcSlide;
            autoPunch = other.autoPunch;
            playerSlap = other.playerSlap;
            fastSwitch = other.fastSwitch;
            luaSlide = other.luaSlide;
            hotkeys = other.hotkeys;
            misc = other.misc;
        }
        return *this;
    }
};

extern MenuState g_MenuState;

namespace Config
{
    constexpr int CURRENT_CONFIG_VERSION = 1;

    bool Save(const char* filename = "somalia_config.json");
    bool Load(const char* filename = "somalia_config.json");
    std::string SaveToString();
    bool LoadFromString(const char* buffer);
    bool SaveToCloud(const std::string& configName, std::string& outMsg);
    bool LoadFromCloud(const std::string& configName, std::string& outMsg);
    void ResetToDefaults();

    struct AccountInfo
    {
        std::string username = "Somalia";
        std::string subscription = "VIP: Ilimitado";
        std::string daysLeft = "";
        std::string sessionId = "";
        std::string keyauthName = "somalia";
        std::string keyauthOwner = "5bU1fK1ki3";
    };

    AccountInfo GetAccountInfo();
    std::string GetSessionId();
}

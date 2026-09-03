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

    // World ESP (UI + Config)
    bool vehicleESP = false;
    bool pickupESP = false;
    bool objectESP = false;

    // Environment (UI + Config)
    bool nightMode = false;
    bool weatherChanger = false;
    int weatherID = 0;
    bool timeChanger = false;
    int timeHour = 12;

    // Camera & Indicators (Runtime Integrado)
    bool drawFOVCircle = true;
    int fovCircleRadius = 60;
    bool customCrosshair = false;
    bool hitmarker = false;
    bool damageInformer = false;
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
    bool teamCheck = false;
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
        weapons[0] = { true, 35.0f, 4.0f, 0, 300.0f, 0, false, false, true, true, false, 1, true };
        // 1: Pistols (Desert Eagle)
        weapons[1] = { true, 45.0f, 6.0f, 0, 180.0f, 0, false, false, true, true, false, 1, true };
        // 2: Rifles (M4, AK-47)
        weapons[2] = { true, 50.0f, 7.0f, 2, 220.0f, 0, false, false, true, true, false, 1, true };
        // 3: Shotguns (Combat, Sawnoff)
        weapons[3] = { true, 60.0f, 8.0f, 2, 120.0f, 0, false, false, true, true, false, 1, true };
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
    bool preferBodyAim = true;
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
    bool teamCheck = false;
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
        weapons[0] = { true, 0, 0, 0, 80.0f, 100.0f, 350.0f, true, false, false, true, true, true };
        // 1: Pistols (Desert Eagle) -> HEAD padrão, 85% FOV, 100% agressividade
        weapons[1] = { true, 0, 0, 0, 85.0f, 100.0f, 250.0f, true, false, false, true, true, true };
        // 2: Rifles (M4, AK-47) -> HEAD padrão, 90% FOV, 100% agressividade
        weapons[2] = { true, 0, 0, 0, 90.0f, 100.0f, 280.0f, true, false, false, true, true, true };
        // 3: Shotguns (Combat, Sawnoff) -> HEAD padrão, 95% FOV, 100% agressividade
        weapons[3] = { true, 0, 0, 0, 95.0f, 100.0f, 150.0f, true, false, false, true, true, true };
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
    bool teamCheck = false;
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

    SilentAimConfig()
    {
        // 0: Auto Snipers (Sniper, Country) -> HEAD, 35% FOV, 100% HitChance
        weapons[0] = { true, 2, 0, 0, 35.0f, 100, 350.0f, true, false, false, true, true, false };
        // 1: Pistols (Desert Eagle) -> HEAD, 45% FOV, 95% HitChance
        weapons[1] = { true, 2, 0, 0, 45.0f, 95, 220.0f, true, false, false, true, true, false };
        // 2: Rifles (M4, AK-47) -> CHEST, 50% FOV, 90% HitChance
        weapons[2] = { true, 2, 2, 0, 50.0f, 90, 250.0f, true, false, false, true, true, false };
        // 3: Shotguns (Combat, Sawnoff) -> CHEST, 60% FOV, 85% HitChance
        weapons[3] = { true, 2, 2, 0, 60.0f, 85, 140.0f, true, false, false, true, true, false };
    }
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
};

struct SlideConfig
{
    bool enabled = false;
    bool cSlideActive = true;
    bool autoSlideActive = false;
    int durationC = 15;       // ms (5 a 100)
    int delayTroca = 10;      // ms (0 a 250)
    float slideBoost = 1.8f;  // Multiplicador de impulso de velocidade (1.0x a 3.0x)

    // Margens por arma (ms)
    int marginDeagle = 60;
    int marginShotgun = 120;
    int marginSniper = 150;
    int marginM4 = 50;
    int marginAK47 = 50;
};

struct MiscConfig
{
    bool watermark = true;
    bool particles = true;
    int themeColor = 0;
    char configName[32] = "Default.json";
};

struct MenuState
{
    bool menuOpen = false;
    int currentTab = 0;
    int currentAimbotPage = 0; // 0: LEGIT BOT, 1: RAGEBOT

    VisualsConfig  visuals;
    LegitBotConfig legitBot;
    RageBotConfig  rageBot;
    SilentAimConfig silentAim;
    AntiAimConfig  antiAim;
    PlayerConfig   player;
    VehicleConfig  vehicle;
    SlideConfig    slide;
    MiscConfig     misc;

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
        , antiAim(other.antiAim)
        , player(other.player)
        , vehicle(other.vehicle)
        , slide(other.slide)
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
            antiAim = other.antiAim;
            player = other.player;
            vehicle = other.vehicle;
            slide = other.slide;
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
}

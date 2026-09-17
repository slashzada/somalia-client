#include "Config.h"
#include "../UI/Theme.h"
#include "../Core/Logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <iomanip>
#include <stdint.h>
#include <wininet.h>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "advapi32.lib")

MenuState g_MenuState;

namespace Config
{
    void ResetToDefaults()
    {
        g_MenuState.visuals = VisualsConfig();
        g_MenuState.legitBot = LegitBotConfig();
        g_MenuState.rageBot = RageBotConfig();
        g_MenuState.silentAim = SilentAimConfig();
        g_MenuState.antiAim = AntiAimConfig();
        g_MenuState.player = PlayerConfig();
        g_MenuState.vehicle = VehicleConfig();
        g_MenuState.kfcSlide = KFCSlideConfig();
        g_MenuState.autoPunch = AutoPunchConfig();
        g_MenuState.playerSlap = PlayerSlapConfig();
        g_MenuState.fastSwitch = FastSwitchConfig();
        g_MenuState.luaSlide = LuaSlideConfig();
        g_MenuState.hotkeys = HotkeysConfig();
        g_MenuState.misc = MiscConfig();
        Theme::SetAccentColor(137.f / 255.f, 207.f / 255.f, 240.f / 255.f, 1.0f);
        Logger::Log("[CONFIG] Configuracoes restauradas para os padroes (Legit e Rage independentes).");
    }

    static void AppendFmt(std::string& s, const char* fmt, ...)
    {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        s += buf;
    }

    std::string SaveToString()
    {
        std::string out;
        out.reserve(8192);

        {
        }

        AppendFmt(out, "{\n");
        AppendFmt(out, "  \"config_version\": %d,\n", CURRENT_CONFIG_VERSION);
        AppendFmt(out, "  \"visuals\": {\n");
        AppendFmt(out, "    \"enableESP\": %s,\n", g_MenuState.visuals.enableESP ? "true" : "false");
        AppendFmt(out, "    \"boxESP\": %s,\n", g_MenuState.visuals.boxESP ? "true" : "false");
        AppendFmt(out, "    \"boxType\": %d,\n", g_MenuState.visuals.boxType);
        AppendFmt(out, "    \"nameESP\": %s,\n", g_MenuState.visuals.nameESP ? "true" : "false");
        AppendFmt(out, "    \"healthESP\": %s,\n", g_MenuState.visuals.healthESP ? "true" : "false");
        AppendFmt(out, "    \"armorESP\": %s,\n", g_MenuState.visuals.armorESP ? "true" : "false");
        AppendFmt(out, "    \"distanceESP\": %s,\n", g_MenuState.visuals.distanceESP ? "true" : "false");
        AppendFmt(out, "    \"bonesESP\": %s,\n", g_MenuState.visuals.bonesESP ? "true" : "false");
        AppendFmt(out, "    \"snaplines\": %s,\n", g_MenuState.visuals.snaplines ? "true" : "false");
        AppendFmt(out, "    \"snaplineOrigin\": %d,\n", g_MenuState.visuals.snaplineOrigin);
        AppendFmt(out, "    \"maxDistance\": %d,\n", g_MenuState.visuals.maxDistance);
        AppendFmt(out, "    \"drawFOVCircle\": %s,\n", g_MenuState.visuals.drawFOVCircle ? "true" : "false");
        AppendFmt(out, "    \"fovCircleRadius\": %d,\n", g_MenuState.visuals.fovCircleRadius);
        AppendFmt(out, "    \"customCrosshair\": %s,\n", g_MenuState.visuals.customCrosshair ? "true" : "false");
        AppendFmt(out, "    \"enemyOnly\": %s,\n", g_MenuState.visuals.enemyOnly ? "true" : "false");
        AppendFmt(out, "    \"nightMode\": %s,\n", g_MenuState.visuals.nightMode ? "true" : "false");
        AppendFmt(out, "    \"weatherChanger\": %s,\n", g_MenuState.visuals.weatherChanger ? "true" : "false");
        AppendFmt(out, "    \"weatherID\": %d,\n", g_MenuState.visuals.weatherID);
        AppendFmt(out, "    \"timeChanger\": %s,\n", g_MenuState.visuals.timeChanger ? "true" : "false");
        AppendFmt(out, "    \"vehicleESP\": %s,\n", g_MenuState.visuals.vehicleESP ? "true" : "false");
        AppendFmt(out, "    \"pickupESP\": %s,\n", g_MenuState.visuals.pickupESP ? "true" : "false");
        AppendFmt(out, "    \"objectESP\": %s,\n", g_MenuState.visuals.objectESP ? "true" : "false");
        AppendFmt(out, "    \"worldMaxDist\": %d,\n", g_MenuState.visuals.worldMaxDist);
        AppendFmt(out, "    \"weaponESP\": %s,\n", g_MenuState.visuals.weaponESP ? "true" : "false");
        AppendFmt(out, "    \"offscreenArrows\": %s,\n", g_MenuState.visuals.offscreenArrows ? "true" : "false");
        AppendFmt(out, "    \"targetHighlight\": %s,\n", g_MenuState.visuals.targetHighlight ? "true" : "false");
        AppendFmt(out, "    \"lineOfSight\": %s,\n", g_MenuState.visuals.lineOfSight ? "true" : "false");
        AppendFmt(out, "    \"lockHour\": %s,\n", g_MenuState.visuals.lockHour ? "true" : "false");
        AppendFmt(out, "    \"noFog\": %s,\n", g_MenuState.visuals.noFog ? "true" : "false");
        AppendFmt(out, "    \"clearSky\": %s,\n", g_MenuState.visuals.clearSky ? "true" : "false");
        AppendFmt(out, "    \"fullbright\": %s,\n", g_MenuState.visuals.fullbright ? "true" : "false");
        AppendFmt(out, "    \"removeGrass\": %s,\n", g_MenuState.visuals.removeGrass ? "true" : "false");
        AppendFmt(out, "    \"removeRain\": %s,\n", g_MenuState.visuals.removeRain ? "true" : "false");
        AppendFmt(out, "    \"clearWater\": %s,\n", g_MenuState.visuals.clearWater ? "true" : "false");
        AppendFmt(out, "    \"extendedDrawDist\": %s,\n", g_MenuState.visuals.extendedDrawDist ? "true" : "false");
        AppendFmt(out, "    \"customCameraFOV\": %s,\n", g_MenuState.visuals.customCameraFOV ? "true" : "false");
        AppendFmt(out, "    \"cameraFOV\": %.1f,\n", g_MenuState.visuals.cameraFOV);
        AppendFmt(out, "    \"noCamShake\": %s,\n", g_MenuState.visuals.noCamShake ? "true" : "false");
        AppendFmt(out, "    \"hideRadar\": %s,\n", g_MenuState.visuals.hideRadar ? "true" : "false");
        AppendFmt(out, "    \"hideHUD\": %s,\n", g_MenuState.visuals.hideHUD ? "true" : "false");
        AppendFmt(out, "    \"showFPS\": %s,\n", g_MenuState.visuals.showFPS ? "true" : "false");
        AppendFmt(out, "    \"hitmarker\": %s,\n", g_MenuState.visuals.hitmarker ? "true" : "false");
        AppendFmt(out, "    \"damageInformer\": %s\n", g_MenuState.visuals.damageInformer ? "true" : "false");
        AppendFmt(out, "  },\n");

        // 1. LEGIT BOT
        AppendFmt(out, "  \"legitbot\": {\n");
        AppendFmt(out, "    \"enabled\": %s,\n", g_MenuState.legitBot.enabled ? "true" : "false");
        AppendFmt(out, "    \"currentWeaponGroup\": %d,\n", g_MenuState.legitBot.currentWeaponGroup);
        AppendFmt(out, "    \"silentAim\": %s,\n", g_MenuState.legitBot.silentAim ? "true" : "false");
        AppendFmt(out, "    \"exploitLagPeek\": %s,\n", g_MenuState.legitBot.exploitLagPeek ? "true" : "false");
        AppendFmt(out, "    \"exploitHideShots\": %s,\n", g_MenuState.legitBot.exploitHideShots ? "true" : "false");
        AppendFmt(out, "    \"exploitDoubleTap\": %s,\n", g_MenuState.legitBot.exploitDoubleTap ? "true" : "false");
        AppendFmt(out, "    \"preferBodyAim\": %s,\n", g_MenuState.legitBot.preferBodyAim ? "true" : "false");
        AppendFmt(out, "    \"ignoreLimbs\": %s,\n", g_MenuState.legitBot.ignoreLimbs ? "true" : "false");
        AppendFmt(out, "    \"weapons\": [\n");

        for (int i = 0; i < 4; i++)
        {
            const auto& w = g_MenuState.legitBot.weapons[i];
            AppendFmt(out, "      {\n");
            AppendFmt(out, "        \"enabled\": %s,\n", w.enabled ? "true" : "false");
            AppendFmt(out, "        \"fov\": %.1f,\n", w.fov);
            AppendFmt(out, "        \"smooth\": %.1f,\n", w.smooth);
            AppendFmt(out, "        \"bone\": %d,\n", w.bone);
            AppendFmt(out, "        \"maxDistance\": %.1f,\n", w.maxDistance);
            AppendFmt(out, "        \"priority\": %d,\n", w.priority);
            AppendFmt(out, "        \"teamCheck\": %s,\n", w.teamCheck ? "true" : "false");
            AppendFmt(out, "        \"visibilityCheck\": %s,\n", w.visibilityCheck ? "true" : "false");
            AppendFmt(out, "        \"ignoreDead\": %s,\n", w.ignoreDead ? "true" : "false");
            AppendFmt(out, "        \"drawTargetMarker\": %s,\n", w.drawTargetMarker ? "true" : "false");
            AppendFmt(out, "        \"drawTracer\": %s,\n", w.drawTracer ? "true" : "false");
            AppendFmt(out, "        \"activationMode\": %d,\n", w.activationMode);
            AppendFmt(out, "        \"drawSmoothVector\": %s\n", w.drawSmoothVector ? "true" : "false");
            AppendFmt(out, "      }%s\n", (i < 3) ? "," : "");
        }

        AppendFmt(out, "    ]\n");
        AppendFmt(out, "  },\n");

        // 2. RAGEBOT
        AppendFmt(out, "  \"ragebot\": {\n");
        AppendFmt(out, "    \"enabled\": %s,\n", g_MenuState.rageBot.enabled ? "true" : "false");
        AppendFmt(out, "    \"currentWeaponGroup\": %d,\n", g_MenuState.rageBot.currentWeaponGroup);
        AppendFmt(out, "    \"weapons\": [\n");

        for (int i = 0; i < 4; i++)
        {
            const auto& rw = g_MenuState.rageBot.weapons[i];
            AppendFmt(out, "      {\n");
            AppendFmt(out, "        \"enabled\": %s,\n", rw.enabled ? "true" : "false");
            AppendFmt(out, "        \"activationMode\": %d,\n", rw.activationMode);
            AppendFmt(out, "        \"bone\": %d,\n", rw.bone);
            AppendFmt(out, "        \"priority\": %d,\n", rw.priority);
            AppendFmt(out, "        \"fov\": %.1f,\n", rw.fov);
            AppendFmt(out, "        \"aggressiveness\": %.1f,\n", rw.aggressiveness);
            AppendFmt(out, "        \"maxDistance\": %.1f,\n", rw.maxDistance);
            AppendFmt(out, "        \"ignoreDead\": %s,\n", rw.ignoreDead ? "true" : "false");
            AppendFmt(out, "        \"teamCheck\": %s,\n", rw.teamCheck ? "true" : "false");
            AppendFmt(out, "        \"visibilityCheck\": %s,\n", rw.visibilityCheck ? "true" : "false");
            AppendFmt(out, "        \"targetIndicator\": %s,\n", rw.targetIndicator ? "true" : "false");
            AppendFmt(out, "        \"drawFov\": %s,\n", rw.drawFov ? "true" : "false");
            AppendFmt(out, "        \"debugVector\": %s\n", rw.debugVector ? "true" : "false");
            AppendFmt(out, "      }%s\n", (i < 3) ? "," : "");
        }

        AppendFmt(out, "    ]\n");
        AppendFmt(out, "  },\n");

        // SILENT AIM
        AppendFmt(out, "  \"silentaim\": {\n");
        AppendFmt(out, "    \"enabled\": %s,\n", g_MenuState.silentAim.enabled ? "true" : "false");
        AppendFmt(out, "    \"currentWeaponGroup\": %d,\n", g_MenuState.silentAim.currentWeaponGroup);
        AppendFmt(out, "    \"weapons\": [\n");

        for (int i = 0; i < 4; i++)
        {
            const auto& sw = g_MenuState.silentAim.weapons[i];
            AppendFmt(out, "      {\n");
            AppendFmt(out, "        \"enabled\": %s,\n", sw.enabled ? "true" : "false");
            AppendFmt(out, "        \"activationMode\": %d,\n", sw.activationMode);
            AppendFmt(out, "        \"bone\": %d,\n", sw.bone);
            AppendFmt(out, "        \"priority\": %d,\n", sw.priority);
            AppendFmt(out, "        \"fov\": %.1f,\n", sw.fov);
            AppendFmt(out, "        \"hitChance\": %d,\n", sw.hitChance);
            AppendFmt(out, "        \"maxDistance\": %.1f,\n", sw.maxDistance);
            AppendFmt(out, "        \"ignoreDead\": %s,\n", sw.ignoreDead ? "true" : "false");
            AppendFmt(out, "        \"teamCheck\": %s,\n", sw.teamCheck ? "true" : "false");
            AppendFmt(out, "        \"visibilityCheck\": %s,\n", sw.visibilityCheck ? "true" : "false");
            AppendFmt(out, "        \"targetIndicator\": %s,\n", sw.targetIndicator ? "true" : "false");
            AppendFmt(out, "        \"drawFov\": %s,\n", sw.drawFov ? "true" : "false");
            AppendFmt(out, "        \"drawTracer\": %s\n", sw.drawTracer ? "true" : "false");
            AppendFmt(out, "      }%s\n", (i < 3) ? "," : "");
        }

        AppendFmt(out, "    ]\n");
        AppendFmt(out, "  },\n");

        // TRIGGERBOT
        AppendFmt(out, "  \"triggerbot\": {\n");
        AppendFmt(out, "    \"enabled\": %s,\n", g_MenuState.triggerBot.enabled ? "true" : "false");
        AppendFmt(out, "    \"reactionDelay\": %d\n", g_MenuState.triggerBot.reactionDelay);
        AppendFmt(out, "  },\n");

        // 3. ANTI-AIM
        AppendFmt(out, "  \"antiAim\": {\n");
        AppendFmt(out, "    \"enabled\": %s,\n", g_MenuState.antiAim.enabled ? "true" : "false");
        AppendFmt(out, "    \"pitchMode\": %d,\n", g_MenuState.antiAim.pitchMode);
        AppendFmt(out, "    \"yawMode\": %d,\n", g_MenuState.antiAim.yawMode);
        AppendFmt(out, "    \"spinSpeed\": %d,\n", g_MenuState.antiAim.spinSpeed);
        AppendFmt(out, "    \"fakeLag\": %s,\n", g_MenuState.antiAim.fakeLag ? "true" : "false");
        AppendFmt(out, "    \"fakeLagLimit\": %d,\n", g_MenuState.antiAim.fakeLagLimit);
        AppendFmt(out, "    \"desync\": %s,\n", g_MenuState.antiAim.desync ? "true" : "false");
        AppendFmt(out, "    \"invertebred\": %s\n", g_MenuState.antiAim.invertebred ? "true" : "false");
        AppendFmt(out, "  },\n");

        // 4. PLAYER
        AppendFmt(out, "  \"player\": {\n");
        AppendFmt(out, "    \"godmode\": %s,\n", g_MenuState.player.godmode ? "true" : "false");
        AppendFmt(out, "    \"infAmmo\": %s,\n", g_MenuState.player.infAmmo ? "true" : "false");
        AppendFmt(out, "    \"infStamina\": %s,\n", g_MenuState.player.infStamina ? "true" : "false");
        AppendFmt(out, "    \"fastRun\": %s,\n", g_MenuState.player.fastRun ? "true" : "false");
        AppendFmt(out, "    \"megaJump\": %s,\n", g_MenuState.player.megaJump ? "true" : "false");
        AppendFmt(out, "    \"antiStun\": %s,\n", g_MenuState.player.antiStun ? "true" : "false");
        AppendFmt(out, "    \"fastReload\": %s,\n", g_MenuState.player.fastReload ? "true" : "false");
        AppendFmt(out, "    \"autoCBug\": %s,\n", g_MenuState.player.autoCBug ? "true" : "false");
        AppendFmt(out, "    \"noSpread\": %s,\n", g_MenuState.player.noSpread ? "true" : "false");
        AppendFmt(out, "    \"autoBhop\": %s,\n", g_MenuState.player.autoBhop ? "true" : "false");
        AppendFmt(out, "    \"fallProof\": %s,\n", g_MenuState.player.fallProof ? "true" : "false");
        AppendFmt(out, "    \"antiHS\": %s,\n", g_MenuState.player.antiHS ? "true" : "false");
        AppendFmt(out, "    \"antiHSDamageCap\": %.1f\n", g_MenuState.player.antiHSDamageCap);
        AppendFmt(out, "  },\n");

        // 5. VEHICLE
        AppendFmt(out, "  \"vehicle\": {\n");
        AppendFmt(out, "    \"engineAlwaysOn\": %s,\n", g_MenuState.vehicle.engineAlwaysOn ? "true" : "false");
        AppendFmt(out, "    \"carGodmode\": %s,\n", g_MenuState.vehicle.carGodmode ? "true" : "false");
        AppendFmt(out, "    \"speedMultiplier\": %d,\n", g_MenuState.vehicle.speedMultiplier);
        AppendFmt(out, "    \"autoFlip\": %s,\n", g_MenuState.vehicle.autoFlip ? "true" : "false");
        AppendFmt(out, "    \"instantRepair\": %s,\n", g_MenuState.vehicle.instantRepair ? "true" : "false");
        AppendFmt(out, "    \"noBikeFall\": %s,\n", g_MenuState.vehicle.noBikeFall ? "true" : "false");
        AppendFmt(out, "    \"superBrake\": %s,\n", g_MenuState.vehicle.superBrake ? "true" : "false");
        AppendFmt(out, "    \"heavyVehicle\": %s,\n", g_MenuState.vehicle.heavyVehicle ? "true" : "false");
        AppendFmt(out, "    \"driftMode\": %s,\n", g_MenuState.vehicle.driftMode ? "true" : "false");
        AppendFmt(out, "    \"unlimitedNitro\": %s,\n", g_MenuState.vehicle.unlimitedNitro ? "true" : "false");
        AppendFmt(out, "    \"flyCar\": %s\n", g_MenuState.vehicle.flyCar ? "true" : "false");
        AppendFmt(out, "  },\n");

        // 6. KFC SLIDE
        AppendFmt(out, "  \"kfcSlide\": {\n");
        AppendFmt(out, "    \"enabled\": %s,\n", g_MenuState.kfcSlide.enabled ? "true" : "false");
        AppendFmt(out, "    \"speed\": %.2f,\n", g_MenuState.kfcSlide.speed);
        AppendFmt(out, "    \"durationMs\": %d\n", g_MenuState.kfcSlide.durationMs);
        AppendFmt(out, "  },\n");

        // 7. AUTO PUNCH (Auto Soco apos slide)
        AppendFmt(out, "  \"autoPunch\": {\n");
        AppendFmt(out, "    \"enabled\": %s,\n", g_MenuState.autoPunch.enabled ? "true" : "false");
        AppendFmt(out, "    \"delayMs\": %d,\n", g_MenuState.autoPunch.delayMs);
        AppendFmt(out, "    \"cooldownMs\": %d\n", g_MenuState.autoPunch.cooldownMs);
        AppendFmt(out, "  },\n");

        // 8.6 PLAYER SLAP (Tapa Exploit / Knockdown)
        AppendFmt(out, "  \"playerSlap\": {\n");
        AppendFmt(out, "    \"enabled\": %s,\n", g_MenuState.playerSlap.enabled ? "true" : "false");
        AppendFmt(out, "    \"mode\": %d,\n", g_MenuState.playerSlap.mode);
        AppendFmt(out, "    \"force\": %.2f,\n", g_MenuState.playerSlap.force);
        AppendFmt(out, "    \"manualTargetId\": %d,\n", g_MenuState.playerSlap.manualTargetId);
        AppendFmt(out, "    \"hotkey\": %d,\n", g_MenuState.playerSlap.hotkey);
        AppendFmt(out, "    \"chatCommands\": %s,\n", g_MenuState.playerSlap.chatCommands ? "true" : "false");
        AppendFmt(out, "    \"notifyOnExecute\": %s\n", g_MenuState.playerSlap.notifyOnExecute ? "true" : "false");
        AppendFmt(out, "  },\n");

        // 8.7 FAST SWITCH (arquive.cs)
        AppendFmt(out, "  \"fastSwitch\": {\n");
        AppendFmt(out, "    \"enabled\": %s\n", g_MenuState.fastSwitch.enabled ? "true" : "false");
        AppendFmt(out, "  },\n");

        // 8.8 LUA SLIDE (archiveszada.lua)
        AppendFmt(out, "  \"luaSlide\": {\n");
        AppendFmt(out, "    \"enabled\": %s,\n", g_MenuState.luaSlide.enabled ? "true" : "false");
        AppendFmt(out, "    \"marginSnp\": %d,\n", g_MenuState.luaSlide.marginSnp);
        AppendFmt(out, "    \"marginDesert\": %d,\n", g_MenuState.luaSlide.marginDesert);
        AppendFmt(out, "    \"marginM4\": %d,\n", g_MenuState.luaSlide.marginM4);
        AppendFmt(out, "    \"marginAK\": %d,\n", g_MenuState.luaSlide.marginAK);
        AppendFmt(out, "    \"marginShot\": %d\n", g_MenuState.luaSlide.marginShot);
        AppendFmt(out, "  },\n");

        // 8.9 HOTKEYS (Keybinds)
        AppendFmt(out, "  \"hotkeys\": {\n");
        AppendFmt(out, "    \"autoSlideKey\": %d,\n", g_MenuState.hotkeys.autoSlideKey);
        AppendFmt(out, "    \"kfcSlideKey\": %d,\n", g_MenuState.hotkeys.kfcSlideKey);
        AppendFmt(out, "    \"fastSwitchKey\": %d,\n", g_MenuState.hotkeys.fastSwitchKey);
        AppendFmt(out, "    \"autoPunchKey\": %d,\n", g_MenuState.hotkeys.autoPunchKey);
        AppendFmt(out, "    \"silentAimKey\": %d,\n", g_MenuState.hotkeys.silentAimKey);
        AppendFmt(out, "    \"legitBotKey\": %d,\n", g_MenuState.hotkeys.legitBotKey);
        AppendFmt(out, "    \"antiAimKey\": %d,\n", g_MenuState.hotkeys.antiAimKey);
        AppendFmt(out, "    \"antiHSKey\": %d,\n", g_MenuState.hotkeys.antiHSKey);
        AppendFmt(out, "    \"godmodeKey\": %d\n", g_MenuState.hotkeys.godmodeKey);
        AppendFmt(out, "  },\n");

        // 9. MISC
        AppendFmt(out, "  \"misc\": {\n");
        AppendFmt(out, "    \"particles\": %s,\n", g_MenuState.misc.particles ? "true" : "false");
        AppendFmt(out, "    \"watermark\": %s,\n", g_MenuState.misc.watermark ? "true" : "false");
        AppendFmt(out, "    \"streamProof\": %s,\n", g_MenuState.misc.streamProof ? "true" : "false");
        AppendFmt(out, "    \"accentR\": %.4f,\n", g_MenuState.misc.accentColor[0]);
        AppendFmt(out, "    \"accentG\": %.4f,\n", g_MenuState.misc.accentColor[1]);
        AppendFmt(out, "    \"accentB\": %.4f\n", g_MenuState.misc.accentColor[2]);
        AppendFmt(out, "  }\n");
        AppendFmt(out, "}\n");
        return out;
    }

    static bool ParseBool(const char* content, const char* key, bool defaultVal)
    {
        const char* p = strstr(content, key);
        if (!p) return defaultVal;
        p = strchr(p, ':');
        if (!p) return defaultVal;
        while (*p == ':' || *p == ' ' || *p == '\t' || *p == '\"') p++;
        return (strncmp(p, "true", 4) == 0 || *p == '1');
    }

    static int ParseInt(const char* content, const char* key, int defaultVal)
    {
        const char* p = strstr(content, key);
        if (!p) return defaultVal;
        p = strchr(p, ':');
        if (!p) return defaultVal;
        while (*p == ':' || *p == ' ' || *p == '\t' || *p == '\"') p++;
        return atoi(p);
    }

    static float ParseFloat(const char* content, const char* key, float defaultVal)
    {
        const char* p = strstr(content, key);
        if (!p) return defaultVal;
        p = strchr(p, ':');
        if (!p) return defaultVal;
        while (*p == ':' || *p == ' ' || *p == '\t' || *p == '\"') p++;
        return static_cast<float>(atof(p));
    }

    bool Save(const char* filename)
    {
        std::string s = SaveToString();
        std::string tmpPath = std::string(filename) + ".tmp";
        FILE* f = fopen(tmpPath.c_str(), "w");
        if (!f)
        {
            f = fopen(filename, "w");
            if (!f)
            {
                Logger::Log("[CONFIG] Erro ao abrir arquivo para salvar: %s", filename);
                return false;
            }
            fputs(s.c_str(), f);
            fclose(f);
            Logger::Log("[CONFIG] Configuracao salva com sucesso (direto) em: %s", filename);
            return true;
        }

        fputs(s.c_str(), f);
        fclose(f);

        if (!MoveFileExA(tmpPath.c_str(), filename, MOVEFILE_REPLACE_EXISTING | MOVEFILE_COPY_ALLOWED))
        {
            DeleteFileA(filename);
            MoveFileA(tmpPath.c_str(), filename);
        }

        Logger::Log("[CONFIG] Configuracao salva com sucesso (atomico) em: %s", filename);
        return true;
    }

    bool LoadFromString(const char* buffer)
    {
        if (!buffer || buffer[0] == '\0')
        {
            Logger::Log("[CONFIG] Falha: Buffer de configuracao vazio.");
            return false;
        }

        // Validação estrutural básica de JSON
        const char* pOpen = strchr(buffer, '{');
        const char* pClose = strrchr(buffer, '}');
        if (!pOpen || !pClose || pClose <= pOpen)
        {
            Logger::Log("[CONFIG] Falha: Estrutura JSON invalida.");
            return false;
        }

        // Parse temporário: g_MenuState só é atualizado se a validação passar
        MenuState tempState = g_MenuState;

        int configVersion = ParseInt(buffer, "\"config_version\"", 1);
        (void)configVersion;

        // Parse Visuals
        const char* pVisuals = strstr(buffer, "\"visuals\"");
        if (pVisuals)
        {
            tempState.visuals.enableESP = ParseBool(pVisuals, "\"enableESP\"", tempState.visuals.enableESP);
            tempState.visuals.boxESP = ParseBool(pVisuals, "\"boxESP\"", tempState.visuals.boxESP);
            tempState.visuals.boxType = ParseInt(pVisuals, "\"boxType\"", tempState.visuals.boxType);
            tempState.visuals.nameESP = ParseBool(pVisuals, "\"nameESP\"", tempState.visuals.nameESP);
            tempState.visuals.healthESP = ParseBool(pVisuals, "\"healthESP\"", tempState.visuals.healthESP);
            tempState.visuals.armorESP = ParseBool(pVisuals, "\"armorESP\"", tempState.visuals.armorESP);
            tempState.visuals.distanceESP = ParseBool(pVisuals, "\"distanceESP\"", tempState.visuals.distanceESP);
            tempState.visuals.bonesESP = ParseBool(pVisuals, "\"bonesESP\"", tempState.visuals.bonesESP);
            tempState.visuals.snaplines = ParseBool(pVisuals, "\"snaplines\"", tempState.visuals.snaplines);
            tempState.visuals.snaplineOrigin = ParseInt(pVisuals, "\"snaplineOrigin\"", tempState.visuals.snaplineOrigin);
            tempState.visuals.maxDistance = ParseInt(pVisuals, "\"maxDistance\"", tempState.visuals.maxDistance);
            tempState.visuals.drawFOVCircle = ParseBool(pVisuals, "\"drawFOVCircle\"", tempState.visuals.drawFOVCircle);
            tempState.visuals.fovCircleRadius = ParseInt(pVisuals, "\"fovCircleRadius\"", tempState.visuals.fovCircleRadius);
            tempState.visuals.customCrosshair = ParseBool(pVisuals, "\"customCrosshair\"", tempState.visuals.customCrosshair);
            tempState.visuals.enemyOnly = ParseBool(pVisuals, "\"enemyOnly\"", tempState.visuals.enemyOnly);
            tempState.visuals.nightMode = ParseBool(pVisuals, "\"nightMode\"", tempState.visuals.nightMode);
            tempState.visuals.weatherChanger = ParseBool(pVisuals, "\"weatherChanger\"", tempState.visuals.weatherChanger);
            tempState.visuals.weatherID = ParseInt(pVisuals, "\"weatherID\"", tempState.visuals.weatherID);
            tempState.visuals.timeChanger = ParseBool(pVisuals, "\"timeChanger\"", tempState.visuals.timeChanger);
            tempState.visuals.timeHour = ParseInt(pVisuals, "\"timeHour\"", tempState.visuals.timeHour);
            tempState.visuals.vehicleESP = ParseBool(pVisuals, "\"vehicleESP\"", tempState.visuals.vehicleESP);
            tempState.visuals.pickupESP = ParseBool(pVisuals, "\"pickupESP\"", tempState.visuals.pickupESP);
            tempState.visuals.objectESP = ParseBool(pVisuals, "\"objectESP\"", tempState.visuals.objectESP);
            tempState.visuals.worldMaxDist = ParseInt(pVisuals, "\"worldMaxDist\"", tempState.visuals.worldMaxDist);
            tempState.visuals.weaponESP = ParseBool(pVisuals, "\"weaponESP\"", tempState.visuals.weaponESP);
            tempState.visuals.offscreenArrows = ParseBool(pVisuals, "\"offscreenArrows\"", tempState.visuals.offscreenArrows);
            tempState.visuals.targetHighlight = ParseBool(pVisuals, "\"targetHighlight\"", tempState.visuals.targetHighlight);
            tempState.visuals.lineOfSight = ParseBool(pVisuals, "\"lineOfSight\"", tempState.visuals.lineOfSight);
            tempState.visuals.lockHour = ParseBool(pVisuals, "\"lockHour\"", tempState.visuals.lockHour);
            tempState.visuals.noFog = ParseBool(pVisuals, "\"noFog\"", tempState.visuals.noFog);
            tempState.visuals.clearSky = ParseBool(pVisuals, "\"clearSky\"", tempState.visuals.clearSky);
            tempState.visuals.fullbright = ParseBool(pVisuals, "\"fullbright\"", tempState.visuals.fullbright);
            tempState.visuals.removeGrass = ParseBool(pVisuals, "\"removeGrass\"", tempState.visuals.removeGrass);
            tempState.visuals.removeRain = ParseBool(pVisuals, "\"removeRain\"", tempState.visuals.removeRain);
            tempState.visuals.clearWater = ParseBool(pVisuals, "\"clearWater\"", tempState.visuals.clearWater);
            tempState.visuals.extendedDrawDist = ParseBool(pVisuals, "\"extendedDrawDist\"", tempState.visuals.extendedDrawDist);
            tempState.visuals.customCameraFOV = ParseBool(pVisuals, "\"customCameraFOV\"", tempState.visuals.customCameraFOV);
            tempState.visuals.cameraFOV = ParseFloat(pVisuals, "\"cameraFOV\"", tempState.visuals.cameraFOV);
            tempState.visuals.noCamShake = ParseBool(pVisuals, "\"noCamShake\"", tempState.visuals.noCamShake);
            tempState.visuals.hideRadar = ParseBool(pVisuals, "\"hideRadar\"", tempState.visuals.hideRadar);
            tempState.visuals.hideHUD = ParseBool(pVisuals, "\"hideHUD\"", tempState.visuals.hideHUD);
            tempState.visuals.showFPS = ParseBool(pVisuals, "\"showFPS\"", tempState.visuals.showFPS);
            tempState.visuals.hitmarker = ParseBool(pVisuals, "\"hitmarker\"", tempState.visuals.hitmarker);
            tempState.visuals.damageInformer = ParseBool(pVisuals, "\"damageInformer\"", tempState.visuals.damageInformer);
        }

        // 1. Parse LegitBot
        const char* pLegit = strstr(buffer, "\"legitbot\"");
        if (!pLegit) pLegit = strstr(buffer, "\"aimbot\"");
        if (pLegit)
        {
            tempState.legitBot.enabled = ParseBool(pLegit, "\"enabled\"", tempState.legitBot.enabled);
            tempState.legitBot.currentWeaponGroup = ParseInt(pLegit, "\"currentWeaponGroup\"", tempState.legitBot.currentWeaponGroup);
            tempState.legitBot.silentAim = ParseBool(pLegit, "\"silentAim\"", tempState.legitBot.silentAim);
            tempState.legitBot.exploitLagPeek = ParseBool(pLegit, "\"exploitLagPeek\"", tempState.legitBot.exploitLagPeek);
            tempState.legitBot.exploitHideShots = ParseBool(pLegit, "\"exploitHideShots\"", tempState.legitBot.exploitHideShots);
            tempState.legitBot.exploitDoubleTap = ParseBool(pLegit, "\"exploitDoubleTap\"", tempState.legitBot.exploitDoubleTap);
            tempState.legitBot.preferBodyAim = ParseBool(pLegit, "\"preferBodyAim\"", tempState.legitBot.preferBodyAim);
            tempState.legitBot.ignoreLimbs = ParseBool(pLegit, "\"ignoreLimbs\"", tempState.legitBot.ignoreLimbs);

            const char* pWeapons = strstr(pLegit, "\"weapons\"");
            if (pWeapons)
            {
                const char* curObj = pWeapons;
                for (int i = 0; i < 4; i++)
                {
                    curObj = strchr(curObj, '{');
                    if (!curObj) break;

                    auto& w = tempState.legitBot.weapons[i];
                    w.enabled = ParseBool(curObj, "\"enabled\"", w.enabled);
                    w.fov = ParseFloat(curObj, "\"fov\"", w.fov);
                    w.smooth = ParseFloat(curObj, "\"smooth\"", w.smooth);
                    w.bone = ParseInt(curObj, "\"bone\"", w.bone);
                    w.maxDistance = ParseFloat(curObj, "\"maxDistance\"", w.maxDistance);
                    w.priority = ParseInt(curObj, "\"priority\"", w.priority);
                    w.teamCheck = ParseBool(curObj, "\"teamCheck\"", w.teamCheck);
                    w.visibilityCheck = ParseBool(curObj, "\"visibilityCheck\"", w.visibilityCheck);
                    w.ignoreDead = ParseBool(curObj, "\"ignoreDead\"", w.ignoreDead);
                    w.drawTargetMarker = ParseBool(curObj, "\"drawTargetMarker\"", w.drawTargetMarker);
                    w.drawTracer = ParseBool(curObj, "\"drawTracer\"", w.drawTracer);
                    w.activationMode = ParseInt(curObj, "\"activationMode\"", w.activationMode);
                    w.drawSmoothVector = ParseBool(curObj, "\"drawSmoothVector\"", w.drawSmoothVector);

                    curObj++;
                }
            }
        }

        // 2. Parse RageBot
        const char* pRage = strstr(buffer, "\"ragebot\"");
        if (pRage)
        {
            tempState.rageBot.enabled = ParseBool(pRage, "\"enabled\"", tempState.rageBot.enabled);
            tempState.rageBot.currentWeaponGroup = ParseInt(pRage, "\"currentWeaponGroup\"", tempState.rageBot.currentWeaponGroup);

            const char* pWeapons = strstr(pRage, "\"weapons\"");
            if (pWeapons)
            {
                const char* curObj = pWeapons;
                for (int i = 0; i < 4; i++)
                {
                    curObj = strchr(curObj, '{');
                    if (!curObj) break;

                    auto& rw = tempState.rageBot.weapons[i];
                    rw.enabled = ParseBool(curObj, "\"enabled\"", rw.enabled);
                    rw.activationMode = ParseInt(curObj, "\"activationMode\"", rw.activationMode);
                    rw.bone = ParseInt(curObj, "\"bone\"", rw.bone);
                    rw.priority = ParseInt(curObj, "\"priority\"", rw.priority);
                    rw.fov = ParseFloat(curObj, "\"fov\"", rw.fov);
                    rw.aggressiveness = ParseFloat(curObj, "\"aggressiveness\"", rw.aggressiveness);
                    rw.maxDistance = ParseFloat(curObj, "\"maxDistance\"", rw.maxDistance);
                    rw.ignoreDead = ParseBool(curObj, "\"ignoreDead\"", rw.ignoreDead);
                    rw.teamCheck = ParseBool(curObj, "\"teamCheck\"", rw.teamCheck);
                    rw.visibilityCheck = ParseBool(curObj, "\"visibilityCheck\"", rw.visibilityCheck);
                    rw.targetIndicator = ParseBool(curObj, "\"targetIndicator\"", rw.targetIndicator);
                    rw.drawFov = ParseBool(curObj, "\"drawFov\"", rw.drawFov);
                    rw.debugVector = ParseBool(curObj, "\"debugVector\"", rw.debugVector);

                    curObj++;
                }
            }
        }

        // Parse Silent Aim
        const char* pSilent = strstr(buffer, "\"silentaim\"");
        if (pSilent)
        {
            tempState.silentAim.enabled = ParseBool(pSilent, "\"enabled\"", tempState.silentAim.enabled);
            tempState.silentAim.currentWeaponGroup = ParseInt(pSilent, "\"currentWeaponGroup\"", tempState.silentAim.currentWeaponGroup);

            const char* pWeapons = strstr(pSilent, "\"weapons\"");
            if (pWeapons)
            {
                const char* curObj = pWeapons;
                for (int i = 0; i < 4; i++)
                {
                    curObj = strchr(curObj, '{');
                    if (!curObj) break;

                    auto& sw = tempState.silentAim.weapons[i];
                    sw.enabled = ParseBool(curObj, "\"enabled\"", sw.enabled);
                    sw.activationMode = ParseInt(curObj, "\"activationMode\"", sw.activationMode);
                    sw.bone = ParseInt(curObj, "\"bone\"", sw.bone);
                    sw.priority = ParseInt(curObj, "\"priority\"", sw.priority);
                    sw.fov = ParseFloat(curObj, "\"fov\"", sw.fov);
                    sw.hitChance = ParseInt(curObj, "\"hitChance\"", sw.hitChance);
                    sw.maxDistance = ParseFloat(curObj, "\"maxDistance\"", sw.maxDistance);
                    sw.ignoreDead = ParseBool(curObj, "\"ignoreDead\"", sw.ignoreDead);
                    sw.teamCheck = ParseBool(curObj, "\"teamCheck\"", sw.teamCheck);
                    sw.visibilityCheck = ParseBool(curObj, "\"visibilityCheck\"", sw.visibilityCheck);
                    sw.targetIndicator = ParseBool(curObj, "\"targetIndicator\"", sw.targetIndicator);
                    sw.drawFov = ParseBool(curObj, "\"drawFov\"", sw.drawFov);
                    sw.drawTracer = ParseBool(curObj, "\"drawTracer\"", sw.drawTracer);

                    curObj++;
                }
            }
        }

        // Parse Triggerbot
        const char* pTrigger = strstr(buffer, "\"triggerbot\"");
        if (pTrigger)
        {
            tempState.triggerBot.enabled = ParseBool(pTrigger, "\"enabled\"", tempState.triggerBot.enabled);
            tempState.triggerBot.reactionDelay = ParseInt(pTrigger, "\"reactionDelay\"", tempState.triggerBot.reactionDelay);
        }

        // 3. Parse Anti-Aim
        const char* pAntiAim = strstr(buffer, "\"antiAim\"");
        if (pAntiAim)
        {
            tempState.antiAim.enabled = ParseBool(pAntiAim, "\"enabled\"", tempState.antiAim.enabled);
            tempState.antiAim.pitchMode = ParseInt(pAntiAim, "\"pitchMode\"", tempState.antiAim.pitchMode);
            tempState.antiAim.yawMode = ParseInt(pAntiAim, "\"yawMode\"", tempState.antiAim.yawMode);
            tempState.antiAim.spinSpeed = ParseInt(pAntiAim, "\"spinSpeed\"", tempState.antiAim.spinSpeed);
            tempState.antiAim.fakeLag = ParseBool(pAntiAim, "\"fakeLag\"", tempState.antiAim.fakeLag);
            tempState.antiAim.fakeLagLimit = ParseInt(pAntiAim, "\"fakeLagLimit\"", tempState.antiAim.fakeLagLimit);
            tempState.antiAim.desync = ParseBool(pAntiAim, "\"desync\"", tempState.antiAim.desync);
            tempState.antiAim.invertebred = ParseBool(pAntiAim, "\"invertebred\"", tempState.antiAim.invertebred);
        }

        // 4. Parse Player
        const char* pPlayer = strstr(buffer, "\"player\"");
        if (pPlayer)
        {
            tempState.player.godmode = ParseBool(pPlayer, "\"godmode\"", tempState.player.godmode);
            tempState.player.infAmmo = ParseBool(pPlayer, "\"infAmmo\"", tempState.player.infAmmo);
            tempState.player.infStamina = ParseBool(pPlayer, "\"infStamina\"", tempState.player.infStamina);
            tempState.player.fastRun = ParseBool(pPlayer, "\"fastRun\"", tempState.player.fastRun);
            tempState.player.megaJump = ParseBool(pPlayer, "\"megaJump\"", tempState.player.megaJump);
            tempState.player.antiStun = ParseBool(pPlayer, "\"antiStun\"", tempState.player.antiStun);
            tempState.player.fastReload = ParseBool(pPlayer, "\"fastReload\"", tempState.player.fastReload);
            tempState.player.autoCBug = ParseBool(pPlayer, "\"autoCBug\"", tempState.player.autoCBug);
            tempState.player.noSpread = ParseBool(pPlayer, "\"noSpread\"", tempState.player.noSpread);
            tempState.player.autoBhop = ParseBool(pPlayer, "\"autoBhop\"", tempState.player.autoBhop);
            tempState.player.fallProof = ParseBool(pPlayer, "\"fallProof\"", tempState.player.fallProof);
            tempState.player.antiHS = ParseBool(pPlayer, "\"antiHS\"", tempState.player.antiHS);
            tempState.player.antiHSDamageCap = ParseFloat(pPlayer, "\"antiHSDamageCap\"", tempState.player.antiHSDamageCap);
        }

        // 5. Parse Vehicle
        const char* pVehicle = strstr(buffer, "\"vehicle\"");
        if (pVehicle)
        {
            tempState.vehicle.engineAlwaysOn = ParseBool(pVehicle, "\"engineAlwaysOn\"", tempState.vehicle.engineAlwaysOn);
            tempState.vehicle.carGodmode = ParseBool(pVehicle, "\"carGodmode\"", tempState.vehicle.carGodmode);
            tempState.vehicle.speedMultiplier = ParseInt(pVehicle, "\"speedMultiplier\"", tempState.vehicle.speedMultiplier);
            tempState.vehicle.autoFlip = ParseBool(pVehicle, "\"autoFlip\"", tempState.vehicle.autoFlip);
            tempState.vehicle.instantRepair = ParseBool(pVehicle, "\"instantRepair\"", tempState.vehicle.instantRepair);
            tempState.vehicle.noBikeFall = ParseBool(pVehicle, "\"noBikeFall\"", tempState.vehicle.noBikeFall);
            tempState.vehicle.superBrake = ParseBool(pVehicle, "\"superBrake\"", tempState.vehicle.superBrake);
            tempState.vehicle.heavyVehicle = ParseBool(pVehicle, "\"heavyVehicle\"", tempState.vehicle.heavyVehicle);
            tempState.vehicle.driftMode = ParseBool(pVehicle, "\"driftMode\"", tempState.vehicle.driftMode);
            tempState.vehicle.unlimitedNitro = ParseBool(pVehicle, "\"unlimitedNitro\"", tempState.vehicle.unlimitedNitro);
            tempState.vehicle.flyCar = ParseBool(pVehicle, "\"flyCar\"", tempState.vehicle.flyCar);
        }

        // 6. Parse KFC Slide
        const char* pKFCSlide = strstr(buffer, "\"kfcSlide\"");
        if (pKFCSlide)
        {
            tempState.kfcSlide.enabled = ParseBool(pKFCSlide, "\"enabled\"", tempState.kfcSlide.enabled);
            tempState.kfcSlide.speed = ParseFloat(pKFCSlide, "\"speed\"", tempState.kfcSlide.speed);
            tempState.kfcSlide.durationMs = ParseInt(pKFCSlide, "\"durationMs\"", tempState.kfcSlide.durationMs);
        }

        // 7. Parse Auto Punch (Auto Soco)
        const char* pAutoPunch = strstr(buffer, "\"autoPunch\"");
        if (pAutoPunch)
        {
            tempState.autoPunch.enabled = ParseBool(pAutoPunch, "\"enabled\"", tempState.autoPunch.enabled);
            tempState.autoPunch.delayMs = ParseInt(pAutoPunch, "\"delayMs\"", tempState.autoPunch.delayMs);
            tempState.autoPunch.cooldownMs = ParseInt(pAutoPunch, "\"cooldownMs\"", tempState.autoPunch.cooldownMs);
        }

        // 8. Parse Player Slap (Tapa Exploit)
        const char* pPlayerSlap = strstr(buffer, "\"playerSlap\"");
        if (pPlayerSlap)
        {
            tempState.playerSlap.enabled = ParseBool(pPlayerSlap, "\"enabled\"", tempState.playerSlap.enabled);
            tempState.playerSlap.mode = ParseInt(pPlayerSlap, "\"mode\"", tempState.playerSlap.mode);
            tempState.playerSlap.force = ParseFloat(pPlayerSlap, "\"force\"", tempState.playerSlap.force);
            tempState.playerSlap.manualTargetId = ParseInt(pPlayerSlap, "\"manualTargetId\"", tempState.playerSlap.manualTargetId);
            tempState.playerSlap.hotkey = ParseInt(pPlayerSlap, "\"hotkey\"", tempState.playerSlap.hotkey);
            tempState.playerSlap.chatCommands = ParseBool(pPlayerSlap, "\"chatCommands\"", tempState.playerSlap.chatCommands);
            tempState.playerSlap.notifyOnExecute = ParseBool(pPlayerSlap, "\"notifyOnExecute\"", tempState.playerSlap.notifyOnExecute);
        }

        // 8.7 Parse Fast Switch
        const char* pFastSwitch = strstr(buffer, "\"fastSwitch\"");
        if (pFastSwitch)
        {
            tempState.fastSwitch.enabled = ParseBool(pFastSwitch, "\"enabled\"", tempState.fastSwitch.enabled);
        }

        // 8.8 Parse Lua Slide
        const char* pLuaSlide = strstr(buffer, "\"luaSlide\"");
        if (pLuaSlide)
        {
            tempState.luaSlide.enabled = ParseBool(pLuaSlide, "\"enabled\"", tempState.luaSlide.enabled);
            tempState.luaSlide.marginSnp = ParseInt(pLuaSlide, "\"marginSnp\"", tempState.luaSlide.marginSnp);
            tempState.luaSlide.marginDesert = ParseInt(pLuaSlide, "\"marginDesert\"", tempState.luaSlide.marginDesert);
            tempState.luaSlide.marginM4 = ParseInt(pLuaSlide, "\"marginM4\"", tempState.luaSlide.marginM4);
            tempState.luaSlide.marginAK = ParseInt(pLuaSlide, "\"marginAK\"", tempState.luaSlide.marginAK);
            tempState.luaSlide.marginShot = ParseInt(pLuaSlide, "\"marginShot\"", tempState.luaSlide.marginShot);
        }

        // 8.9 Parse Hotkeys
        const char* pHotkeys = strstr(buffer, "\"hotkeys\"");
        if (pHotkeys)
        {
            tempState.hotkeys.autoSlideKey = ParseInt(pHotkeys, "\"autoSlideKey\"", tempState.hotkeys.autoSlideKey);
            tempState.hotkeys.kfcSlideKey = ParseInt(pHotkeys, "\"kfcSlideKey\"", tempState.hotkeys.kfcSlideKey);
            tempState.hotkeys.fastSwitchKey = ParseInt(pHotkeys, "\"fastSwitchKey\"", tempState.hotkeys.fastSwitchKey);
            tempState.hotkeys.autoPunchKey = ParseInt(pHotkeys, "\"autoPunchKey\"", tempState.hotkeys.autoPunchKey);
            tempState.hotkeys.silentAimKey = ParseInt(pHotkeys, "\"silentAimKey\"", tempState.hotkeys.silentAimKey);
            tempState.hotkeys.legitBotKey = ParseInt(pHotkeys, "\"legitBotKey\"", tempState.hotkeys.legitBotKey);
            tempState.hotkeys.antiAimKey = ParseInt(pHotkeys, "\"antiAimKey\"", tempState.hotkeys.antiAimKey);
            tempState.hotkeys.antiHSKey = ParseInt(pHotkeys, "\"antiHSKey\"", tempState.hotkeys.antiHSKey);
            tempState.hotkeys.godmodeKey = ParseInt(pHotkeys, "\"godmodeKey\"", tempState.hotkeys.godmodeKey);
        }

        // 9. Parse Misc
        const char* pMisc = strstr(buffer, "\"misc\"");
        if (pMisc)
        {
            tempState.misc.particles = ParseBool(pMisc, "\"particles\"", tempState.misc.particles);
            tempState.misc.watermark = ParseBool(pMisc, "\"watermark\"", tempState.misc.watermark);
            tempState.misc.streamProof = ParseBool(pMisc, "\"streamProof\"", tempState.misc.streamProof);
            tempState.misc.accentColor[0] = ParseFloat(pMisc, "\"accentR\"", tempState.misc.accentColor[0]);
            tempState.misc.accentColor[1] = ParseFloat(pMisc, "\"accentG\"", tempState.misc.accentColor[1]);
            tempState.misc.accentColor[2] = ParseFloat(pMisc, "\"accentB\"", tempState.misc.accentColor[2]);
        }

        // Validação e Clamping defensivo
        for (int c = 0; c < 3; c++)
        {
            if (tempState.misc.accentColor[c] < 0.0f) tempState.misc.accentColor[c] = 0.0f;
            if (tempState.misc.accentColor[c] > 1.0f) tempState.misc.accentColor[c] = 1.0f;
        }
        tempState.misc.accentColor[3] = 1.0f;
        if (tempState.visuals.maxDistance < 10) tempState.visuals.maxDistance = 10;
        if (tempState.visuals.maxDistance > 1000) tempState.visuals.maxDistance = 1000;
        if (tempState.visuals.fovCircleRadius < 5) tempState.visuals.fovCircleRadius = 5;
        if (tempState.visuals.fovCircleRadius > 500) tempState.visuals.fovCircleRadius = 500;
        if (tempState.visuals.timeHour < 0) tempState.visuals.timeHour = 0;
        if (tempState.visuals.timeHour > 23) tempState.visuals.timeHour = 23;
        if (tempState.kfcSlide.speed < 1.0f) tempState.kfcSlide.speed = 1.0f;
        if (tempState.kfcSlide.speed > 15.0f) tempState.kfcSlide.speed = 15.0f;
        if (tempState.kfcSlide.durationMs < 100) tempState.kfcSlide.durationMs = 100;
        if (tempState.kfcSlide.durationMs > 5000) tempState.kfcSlide.durationMs = 5000;

        for (int i = 0; i < 4; i++)
        {
            if (tempState.legitBot.weapons[i].fov < 1.0f) tempState.legitBot.weapons[i].fov = 1.0f;
            if (tempState.legitBot.weapons[i].fov > 100.0f) tempState.legitBot.weapons[i].fov = 100.0f;
            if (tempState.legitBot.weapons[i].smooth < 1.0f) tempState.legitBot.weapons[i].smooth = 1.0f;
            if (tempState.legitBot.weapons[i].smooth > 50.0f) tempState.legitBot.weapons[i].smooth = 50.0f;
            if (tempState.legitBot.weapons[i].bone < 0 || tempState.legitBot.weapons[i].bone > 3) tempState.legitBot.weapons[i].bone = 0;
            if (tempState.legitBot.weapons[i].activationMode < 0 || tempState.legitBot.weapons[i].activationMode > 3) tempState.legitBot.weapons[i].activationMode = 1;

            if (tempState.rageBot.weapons[i].fov < 1.0f) tempState.rageBot.weapons[i].fov = 1.0f;
            if (tempState.rageBot.weapons[i].fov > 100.0f) tempState.rageBot.weapons[i].fov = 100.0f;
            if (tempState.rageBot.weapons[i].aggressiveness < 0.0f) tempState.rageBot.weapons[i].aggressiveness = 0.0f;
            if (tempState.rageBot.weapons[i].aggressiveness > 100.0f) tempState.rageBot.weapons[i].aggressiveness = 100.0f;

            if (tempState.silentAim.weapons[i].hitChance < 0) tempState.silentAim.weapons[i].hitChance = 0;
            if (tempState.silentAim.weapons[i].hitChance > 100) tempState.silentAim.weapons[i].hitChance = 100;
        }

        if (tempState.triggerBot.reactionDelay < 0) tempState.triggerBot.reactionDelay = 0;
        if (tempState.triggerBot.reactionDelay > 200) tempState.triggerBot.reactionDelay = 200;

        // Aplicação atômica do estado validado
        g_MenuState = tempState;
        Theme::SetAccentColor(g_MenuState.misc.accentColor[0], g_MenuState.misc.accentColor[1], g_MenuState.misc.accentColor[2], 1.0f);
        return true;
    }

    bool Load(const char* filename)
    {
        FILE* f = fopen(filename, "r");
        if (!f)
        {
            Logger::Log("[CONFIG] Arquivo %s nao encontrado. Usando padroes.", filename);
            return false;
        }

        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);

        if (sz <= 0 || sz > 1024 * 1024)
        {
            fclose(f);
            return false;
        }

        char* buffer = reinterpret_cast<char*>(malloc(sz + 1));
        if (!buffer)
        {
            fclose(f);
            return false;
        }

        size_t readBytes = fread(buffer, 1, sz, f);
        buffer[readBytes] = '\0';
        fclose(f);

        bool res = LoadFromString(buffer);
        free(buffer);
        if (res)
        {
            Logger::Log("[CONFIG] Configuracao carregada com sucesso de: %s", filename);
        }
        return res;
    }


    // ==========================================================
    //  CLOUD CONFIG VIA KEYAUTH
    // ==========================================================
    static const std::string s_B64Chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    static std::string Base64Encode(const std::string& in)
    {
        std::string out;
        int val = 0, valb = -6;
        for (unsigned char c : in) {
            val = (val << 8) + c;
            valb += 8;
            while (valb >= 0) {
                out.push_back(s_B64Chars[(val >> valb) & 0x3F]);
                valb -= 6;
            }
        }
        if (valb > -6) out.push_back(s_B64Chars[((val << 8) >> (valb + 8)) & 0x3F]);
        while (out.size() % 4) out.push_back('=');
        return out;
    }

    static std::string Base64Decode(const std::string& in)
    {
        std::string out;
        std::vector<int> T(256, -1);
        for (int i = 0; i < 64; i++) T[s_B64Chars[i]] = i;
        int val = 0, valb = -8;
        for (unsigned char c : in) {
            if (T[c] == -1) break;
            val = (val << 6) + T[c];
            valb += 6;
            if (valb >= 0) {
                out.push_back(char((val >> valb) & 0xFF));
                valb -= 8;
            }
        }
        return out;
    }

    static std::string ExtractJsonValue(const std::string& content, const std::string& key)
    {
        std::string search = "\"" + key + "\"";
        size_t pos = content.find(search);
        if (pos == std::string::npos) return "";

        size_t colon = content.find(':', pos + search.length());
        if (colon == std::string::npos) return "";

        size_t start = content.find_first_not_of(" \t\r\n", colon + 1);
        if (start == std::string::npos) return "";

        if (content[start] == '\"')
        {
            size_t end = content.find('\"', start + 1);
            if (end != std::string::npos)
            {
                std::string val = content.substr(start + 1, end - start - 1);
                std::string clean;
                for (size_t i = 0; i < val.length(); ++i)
                {
                    if (val[i] == '\\' && i + 1 < val.length() && val[i + 1] == '\\')
                    {
                        clean += '\\';
                        ++i;
                    }
                    else
                    {
                        clean += val[i];
                    }
                }
                return clean;
            }
        }
        else
        {
            size_t end = content.find_first_of(",}\r\n", start);
            if (end != std::string::npos)
            {
                return content.substr(start, end - start);
            }
        }
        return "";
    }

    static std::string ReadRegString(HKEY hRoot, const char* subKey, const char* valueName)
    {
        HKEY hKey;
        if (RegOpenKeyExA(hRoot, subKey, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
            return "";

        char buf[512] = { 0 };
        DWORD dwType = REG_SZ;
        DWORD dwSize = sizeof(buf) - 1;
        LONG res = RegQueryValueExA(hKey, valueName, NULL, &dwType, reinterpret_cast<LPBYTE>(buf), &dwSize);
        RegCloseKey(hKey);

        if (res == ERROR_SUCCESS)
            return std::string(buf);
        return "";
    }

    AccountInfo GetAccountInfo()
    {
        AccountInfo info;

        // 1. Tenta carregar do Registro do Windows (HKEY_CURRENT_USER\Software\SomaliaClient)
        std::string regSid = ReadRegString(HKEY_CURRENT_USER, "Software\\SomaliaClient", "session_id");
        if (!regSid.empty())
        {
            info.sessionId = regSid;
            std::string u = ReadRegString(HKEY_CURRENT_USER, "Software\\SomaliaClient", "last_username");
            if (!u.empty()) info.username = u;
            std::string sub = ReadRegString(HKEY_CURRENT_USER, "Software\\SomaliaClient", "user_subscription");
            if (!sub.empty()) info.subscription = sub;
            std::string days = ReadRegString(HKEY_CURRENT_USER, "Software\\SomaliaClient", "user_days_left");
            if (!days.empty()) info.daysLeft = days;
            std::string kname = ReadRegString(HKEY_CURRENT_USER, "Software\\SomaliaClient", "keyauth_name");
            if (!kname.empty()) info.keyauthName = kname;
            std::string kowner = ReadRegString(HKEY_CURRENT_USER, "Software\\SomaliaClient", "keyauth_owner");
            if (!kowner.empty()) info.keyauthOwner = kowner;
        }

        // 2. Remove qualquer arquivo somalia_client.json remanescente para proteger credenciais
        std::vector<std::string> fileCandidates;
        char tempPath[MAX_PATH] = { 0 };
        if (GetTempPathA(MAX_PATH, tempPath))
            fileCandidates.push_back(std::string(tempPath) + "somalia_client.json");

        char exePath[MAX_PATH] = { 0 };
        if (GetModuleFileNameA(NULL, exePath, MAX_PATH))
        {
            std::string p = exePath;
            size_t slash = p.find_last_of("\\/");
            if (slash != std::string::npos)
                fileCandidates.push_back(p.substr(0, slash) + "\\somalia_client.json");
        }
        fileCandidates.push_back("somalia_client.json");

        for (const auto& filePath : fileCandidates)
        {
            DeleteFileA(filePath.c_str());
        }

        return info;
    }

    std::string GetSessionId()
    {
        return GetAccountInfo().sessionId;
    }

    static std::string UrlEncode(const std::string& value)
    {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;

        for (char c : value)
        {
            if (isalnum((unsigned char)c) || c == '-' || c == '_' || c == '.' || c == '~')
            {
                escaped << c;
            }
            else
            {
                escaped << '%' << std::setw(2) << ((int)(unsigned char)c);
            }
        }
        return escaped.str();
    }

    static std::string KeyAuthPost(const std::string& postData)
    {
        std::string response;
        HINTERNET hInternet = InternetOpenA("SomaliaClient/1.0", INTERNET_OPEN_TYPE_DIRECT, NULL, NULL, 0);
        if (!hInternet) return "";

        HINTERNET hConnect = InternetConnectA(hInternet, "keyauth.win", INTERNET_DEFAULT_HTTPS_PORT, NULL, NULL, INTERNET_SERVICE_HTTP, 0, 0);
        if (!hConnect)
        {
            InternetCloseHandle(hInternet);
            return "";
        }

        const char* acceptTypes[] = { "*/*", NULL };
        HINTERNET hRequest = HttpOpenRequestA(hConnect, "POST", "/api/1.2/", NULL, NULL, acceptTypes, INTERNET_FLAG_SECURE | INTERNET_FLAG_RELOAD, 0);
        if (!hRequest)
        {
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hInternet);
            return "";
        }

        std::string headers = "Content-Type: application/x-www-form-urlencoded\r\n";
        BOOL sent = HttpSendRequestA(hRequest, headers.c_str(), (DWORD)headers.length(), (LPVOID)postData.c_str(), (DWORD)postData.length());

        if (sent)
        {
            char buffer[8192];
            DWORD bytesRead = 0;
            while (InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0)
            {
                buffer[bytesRead] = '\0';
                response.append(buffer, bytesRead);
            }
        }

        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return response;
    }

#pragma pack(push, 1)
    struct CompactCloudPayload
    {
        char magic[4]; // 'S', 'O', 'M', 'C'
        uint8_t version; // 1

        // Visuals
        uint32_t visualsBools;
        uint8_t boxType;
        uint8_t snaplineOrigin;
        uint16_t maxDistance;
        uint8_t weatherID;
        uint8_t timeHour;
        uint16_t fovCircleRadius;

        // LegitBot
        uint8_t legitBotEnabled;
        uint8_t legitBotWeaponGroup;
        uint8_t legitBotBools;
        struct CompactLegitWeapon {
            uint16_t bools;
            uint16_t fov;
            uint16_t smooth;
            uint8_t bone;
            uint16_t maxDistance;
            uint8_t priority;
            uint8_t activationMode;
        } legitWeapons[4];

        // RageBot
        uint8_t rageBotEnabled;
        uint8_t rageBotWeaponGroup;
        struct CompactRageWeapon {
            uint16_t bools;
            uint8_t activationMode;
            uint8_t bone;
            uint8_t priority;
            uint16_t fov;
            uint16_t aggressiveness;
            uint16_t maxDistance;
        } rageWeapons[4];

        // SilentAim
        uint8_t silentAimEnabled;
        uint8_t silentAimWeaponGroup;
        struct CompactSilentWeapon {
            uint16_t bools;
            uint8_t activationMode;
            uint8_t bone;
            uint8_t priority;
            uint16_t fov;
            uint8_t hitChance;
            uint16_t maxDistance;
        } silentWeapons[4];

        // AntiAim
        uint8_t antiAimEnabled;
        uint8_t antiAimPitchMode;
        uint8_t antiAimYawMode;
        uint8_t antiAimSpinSpeed;
        uint8_t antiAimFakeLag;
        uint8_t antiAimFakeLagLimit;
        uint8_t antiAimDesync;
        uint8_t antiAimInvertebred;

        // Player
        uint16_t playerBools;

        // Vehicle
        uint8_t vehicleBools;
        uint8_t vehicleSpeedMultiplier;

        // KFC Slide
        uint8_t kfcSlideBools;
        uint8_t kfcSlideSpeed;
        uint16_t kfcSlideDurationMs;

        // Misc
        uint8_t miscBools;
    };
#pragma pack(pop)

    static CompactCloudPayload SerializeCompact(const MenuState& state)
    {
        CompactCloudPayload p = {};
        p.magic[0] = 'S'; p.magic[1] = 'O'; p.magic[2] = 'M'; p.magic[3] = 'C';
        p.version = 1;

        // Visuals
        uint32_t vb = 0;
        if (state.visuals.enableESP) vb |= (1 << 0);
        if (state.visuals.boxESP) vb |= (1 << 1);
        if (state.visuals.nameESP) vb |= (1 << 2);
        if (state.visuals.healthESP) vb |= (1 << 3);
        if (state.visuals.armorESP) vb |= (1 << 4);
        if (state.visuals.distanceESP) vb |= (1 << 5);
        if (state.visuals.snaplines) vb |= (1 << 6);
        if (state.visuals.bonesESP) vb |= (1 << 7);
        if (state.visuals.enemyOnly) vb |= (1 << 8);
        if (state.visuals.nightMode) vb |= (1 << 9);
        if (state.visuals.weatherChanger) vb |= (1 << 10);
        if (state.visuals.timeChanger) vb |= (1 << 11);
        if (state.visuals.vehicleESP) vb |= (1 << 12);
        if (state.visuals.pickupESP) vb |= (1 << 13);
        if (state.visuals.objectESP) vb |= (1 << 14);
        if (state.visuals.drawFOVCircle) vb |= (1 << 15);
        if (state.visuals.customCrosshair) vb |= (1 << 16);
        if (state.visuals.hitmarker) vb |= (1 << 17);
        if (state.visuals.damageInformer) vb |= (1 << 18);
        p.visualsBools = vb;

        p.boxType = (uint8_t)state.visuals.boxType;
        p.snaplineOrigin = (uint8_t)state.visuals.snaplineOrigin;
        p.maxDistance = (uint16_t)state.visuals.maxDistance;
        p.weatherID = (uint8_t)state.visuals.weatherID;
        p.timeHour = (uint8_t)state.visuals.timeHour;
        p.fovCircleRadius = (uint16_t)state.visuals.fovCircleRadius;

        // LegitBot
        p.legitBotEnabled = state.legitBot.enabled ? 1 : 0;
        p.legitBotWeaponGroup = (uint8_t)state.legitBot.currentWeaponGroup;
        uint8_t lbb = 0;
        if (state.legitBot.silentAim) lbb |= (1 << 0);
        if (state.legitBot.exploitLagPeek) lbb |= (1 << 1);
        if (state.legitBot.exploitHideShots) lbb |= (1 << 2);
        if (state.legitBot.exploitDoubleTap) lbb |= (1 << 3);
        if (state.legitBot.preferBodyAim) lbb |= (1 << 4);
        if (state.legitBot.ignoreLimbs) lbb |= (1 << 5);
        p.legitBotBools = lbb;

        for (int i = 0; i < 4; ++i)
        {
            const auto& w = state.legitBot.weapons[i];
            uint16_t wb = 0;
            if (w.enabled) wb |= (1 << 0);
            if (w.teamCheck) wb |= (1 << 1);
            if (w.visibilityCheck) wb |= (1 << 2);
            if (w.ignoreDead) wb |= (1 << 3);
            if (w.drawTargetMarker) wb |= (1 << 4);
            if (w.drawTracer) wb |= (1 << 5);
            if (w.drawSmoothVector) wb |= (1 << 6);
            p.legitWeapons[i].bools = wb;
            p.legitWeapons[i].fov = (uint16_t)(w.fov * 10.0f);
            p.legitWeapons[i].smooth = (uint16_t)(w.smooth * 10.0f);
            p.legitWeapons[i].bone = (uint8_t)w.bone;
            p.legitWeapons[i].maxDistance = (uint16_t)(w.maxDistance * 10.0f);
            p.legitWeapons[i].priority = (uint8_t)w.priority;
            p.legitWeapons[i].activationMode = (uint8_t)w.activationMode;
        }

        // RageBot
        p.rageBotEnabled = state.rageBot.enabled ? 1 : 0;
        p.rageBotWeaponGroup = (uint8_t)state.rageBot.currentWeaponGroup;
        for (int i = 0; i < 4; ++i)
        {
            const auto& rw = state.rageBot.weapons[i];
            uint16_t rwb = 0;
            if (rw.enabled) rwb |= (1 << 0);
            if (rw.ignoreDead) rwb |= (1 << 1);
            if (rw.teamCheck) rwb |= (1 << 2);
            if (rw.visibilityCheck) rwb |= (1 << 3);
            if (rw.targetIndicator) rwb |= (1 << 4);
            if (rw.drawFov) rwb |= (1 << 5);
            if (rw.debugVector) rwb |= (1 << 6);
            p.rageWeapons[i].bools = rwb;
            p.rageWeapons[i].activationMode = (uint8_t)rw.activationMode;
            p.rageWeapons[i].bone = (uint8_t)rw.bone;
            p.rageWeapons[i].priority = (uint8_t)rw.priority;
            p.rageWeapons[i].fov = (uint16_t)(rw.fov * 10.0f);
            p.rageWeapons[i].aggressiveness = (uint16_t)(rw.aggressiveness * 10.0f);
            p.rageWeapons[i].maxDistance = (uint16_t)(rw.maxDistance * 10.0f);
        }

        // SilentAim
        p.silentAimEnabled = state.silentAim.enabled ? 1 : 0;
        p.silentAimWeaponGroup = (uint8_t)state.silentAim.currentWeaponGroup;
        for (int i = 0; i < 4; ++i)
        {
            const auto& sw = state.silentAim.weapons[i];
            uint16_t swb = 0;
            if (sw.enabled) swb |= (1 << 0);
            if (sw.ignoreDead) swb |= (1 << 1);
            if (sw.teamCheck) swb |= (1 << 2);
            if (sw.visibilityCheck) swb |= (1 << 3);
            if (sw.targetIndicator) swb |= (1 << 4);
            if (sw.drawFov) swb |= (1 << 5);
            if (sw.drawTracer) swb |= (1 << 6);
            p.silentWeapons[i].bools = swb;
            p.silentWeapons[i].activationMode = (uint8_t)sw.activationMode;
            p.silentWeapons[i].bone = (uint8_t)sw.bone;
            p.silentWeapons[i].priority = (uint8_t)sw.priority;
            p.silentWeapons[i].fov = (uint16_t)(sw.fov * 10.0f);
            p.silentWeapons[i].hitChance = (uint8_t)sw.hitChance;
            p.silentWeapons[i].maxDistance = (uint16_t)(sw.maxDistance * 10.0f);
        }

        // AntiAim
        p.antiAimEnabled = state.antiAim.enabled ? 1 : 0;
        p.antiAimPitchMode = (uint8_t)state.antiAim.pitchMode;
        p.antiAimYawMode = (uint8_t)state.antiAim.yawMode;
        p.antiAimSpinSpeed = (uint8_t)state.antiAim.spinSpeed;
        p.antiAimFakeLag = state.antiAim.fakeLag ? 1 : 0;
        p.antiAimFakeLagLimit = (uint8_t)state.antiAim.fakeLagLimit;
        p.antiAimDesync = state.antiAim.desync ? 1 : 0;
        p.antiAimInvertebred = state.antiAim.invertebred ? 1 : 0;

        // Player
        uint16_t pb = 0;
        if (state.player.godmode) pb |= (1 << 0);
        if (state.player.infAmmo) pb |= (1 << 1);
        if (state.player.infStamina) pb |= (1 << 2);
        if (state.player.fastRun) pb |= (1 << 3);
        if (state.player.megaJump) pb |= (1 << 4);
        if (state.player.antiStun) pb |= (1 << 5);
        if (state.player.fastReload) pb |= (1 << 6);
        if (state.player.autoCBug) pb |= (1 << 7);
        if (state.player.noSpread) pb |= (1 << 8);
        if (state.player.antiHS) pb |= (1 << 9);
        p.playerBools = pb;

        // Vehicle
        uint8_t vb2 = 0;
        if (state.vehicle.engineAlwaysOn) vb2 |= (1 << 0);
        if (state.vehicle.carGodmode) vb2 |= (1 << 1);
        if (state.vehicle.autoFlip) vb2 |= (1 << 2);
        if (state.vehicle.flyCar) vb2 |= (1 << 3);
        if (state.vehicle.instantRepair) vb2 |= (1 << 4);
        if (state.vehicle.noBikeFall) vb2 |= (1 << 5);
        p.vehicleBools = vb2;
        p.vehicleSpeedMultiplier = (uint8_t)state.vehicle.speedMultiplier;

        // KFC Slide
        uint8_t kfcb = 0;
        if (state.kfcSlide.enabled) kfcb |= (1 << 0);
        p.kfcSlideBools = kfcb;
        p.kfcSlideSpeed = (uint8_t)(state.kfcSlide.speed * 10.0f);
        p.kfcSlideDurationMs = (uint16_t)state.kfcSlide.durationMs;

        // Misc
        uint8_t mb = 0;
        if (state.misc.watermark) mb |= (1 << 0);
        if (state.misc.particles) mb |= (1 << 1);
        p.miscBools = mb;

        return p;
    }

    static void DeserializeCompact(MenuState& state, const CompactCloudPayload& p)
    {
        // Visuals
        state.visuals.enableESP = (p.visualsBools & (1 << 0)) != 0;
        state.visuals.boxESP = (p.visualsBools & (1 << 1)) != 0;
        state.visuals.nameESP = (p.visualsBools & (1 << 2)) != 0;
        state.visuals.healthESP = (p.visualsBools & (1 << 3)) != 0;
        state.visuals.armorESP = (p.visualsBools & (1 << 4)) != 0;
        state.visuals.distanceESP = (p.visualsBools & (1 << 5)) != 0;
        state.visuals.snaplines = (p.visualsBools & (1 << 6)) != 0;
        state.visuals.bonesESP = (p.visualsBools & (1 << 7)) != 0;
        state.visuals.enemyOnly = (p.visualsBools & (1 << 8)) != 0;
        state.visuals.nightMode = (p.visualsBools & (1 << 9)) != 0;
        state.visuals.weatherChanger = (p.visualsBools & (1 << 10)) != 0;
        state.visuals.timeChanger = (p.visualsBools & (1 << 11)) != 0;
        state.visuals.vehicleESP = (p.visualsBools & (1 << 12)) != 0;
        state.visuals.pickupESP = (p.visualsBools & (1 << 13)) != 0;
        state.visuals.objectESP = (p.visualsBools & (1 << 14)) != 0;
        state.visuals.drawFOVCircle = (p.visualsBools & (1 << 15)) != 0;
        state.visuals.customCrosshair = (p.visualsBools & (1 << 16)) != 0;
        state.visuals.hitmarker = (p.visualsBools & (1 << 17)) != 0;
        state.visuals.damageInformer = (p.visualsBools & (1 << 18)) != 0;

        state.visuals.boxType = p.boxType;
        state.visuals.snaplineOrigin = p.snaplineOrigin;
        state.visuals.maxDistance = p.maxDistance;
        state.visuals.weatherID = p.weatherID;
        state.visuals.timeHour = p.timeHour;
        state.visuals.fovCircleRadius = p.fovCircleRadius;

        // LegitBot
        state.legitBot.enabled = (p.legitBotEnabled != 0);
        state.legitBot.currentWeaponGroup = p.legitBotWeaponGroup;
        state.legitBot.silentAim = (p.legitBotBools & (1 << 0)) != 0;
        state.legitBot.exploitLagPeek = (p.legitBotBools & (1 << 1)) != 0;
        state.legitBot.exploitHideShots = (p.legitBotBools & (1 << 2)) != 0;
        state.legitBot.exploitDoubleTap = (p.legitBotBools & (1 << 3)) != 0;
        state.legitBot.preferBodyAim = (p.legitBotBools & (1 << 4)) != 0;
        state.legitBot.ignoreLimbs = (p.legitBotBools & (1 << 5)) != 0;

        for (int i = 0; i < 4; ++i)
        {
            auto& w = state.legitBot.weapons[i];
            uint16_t wb = p.legitWeapons[i].bools;
            w.enabled = (wb & (1 << 0)) != 0;
            w.teamCheck = (wb & (1 << 1)) != 0;
            w.visibilityCheck = (wb & (1 << 2)) != 0;
            w.ignoreDead = (wb & (1 << 3)) != 0;
            w.drawTargetMarker = (wb & (1 << 4)) != 0;
            w.drawTracer = (wb & (1 << 5)) != 0;
            w.drawSmoothVector = (wb & (1 << 6)) != 0;
            w.fov = p.legitWeapons[i].fov / 10.0f;
            w.smooth = p.legitWeapons[i].smooth / 10.0f;
            w.bone = p.legitWeapons[i].bone;
            w.maxDistance = p.legitWeapons[i].maxDistance / 10.0f;
            w.priority = p.legitWeapons[i].priority;
            w.activationMode = p.legitWeapons[i].activationMode;
        }

        // RageBot
        state.rageBot.enabled = (p.rageBotEnabled != 0);
        state.rageBot.currentWeaponGroup = p.rageBotWeaponGroup;
        for (int i = 0; i < 4; ++i)
        {
            auto& rw = state.rageBot.weapons[i];
            uint16_t rwb = p.rageWeapons[i].bools;
            rw.enabled = (rwb & (1 << 0)) != 0;
            rw.ignoreDead = (rwb & (1 << 1)) != 0;
            rw.teamCheck = (rwb & (1 << 2)) != 0;
            rw.visibilityCheck = (rwb & (1 << 3)) != 0;
            rw.targetIndicator = (rwb & (1 << 4)) != 0;
            rw.drawFov = (rwb & (1 << 5)) != 0;
            rw.debugVector = (rwb & (1 << 6)) != 0;
            rw.activationMode = p.rageWeapons[i].activationMode;
            rw.bone = p.rageWeapons[i].bone;
            rw.priority = p.rageWeapons[i].priority;
            rw.fov = p.rageWeapons[i].fov / 10.0f;
            rw.aggressiveness = p.rageWeapons[i].aggressiveness / 10.0f;
            rw.maxDistance = p.rageWeapons[i].maxDistance / 10.0f;
        }

        // SilentAim
        state.silentAim.enabled = (p.silentAimEnabled != 0);
        state.silentAim.currentWeaponGroup = p.silentAimWeaponGroup;
        for (int i = 0; i < 4; ++i)
        {
            auto& sw = state.silentAim.weapons[i];
            uint16_t swb = p.silentWeapons[i].bools;
            sw.enabled = (swb & (1 << 0)) != 0;
            sw.ignoreDead = (swb & (1 << 1)) != 0;
            sw.teamCheck = (swb & (1 << 2)) != 0;
            sw.visibilityCheck = (swb & (1 << 3)) != 0;
            sw.targetIndicator = (swb & (1 << 4)) != 0;
            sw.drawFov = (swb & (1 << 5)) != 0;
            sw.drawTracer = (swb & (1 << 6)) != 0;
            sw.activationMode = p.silentWeapons[i].activationMode;
            sw.bone = p.silentWeapons[i].bone;
            sw.priority = p.silentWeapons[i].priority;
            sw.fov = p.silentWeapons[i].fov / 10.0f;
            sw.hitChance = p.silentWeapons[i].hitChance;
            sw.maxDistance = p.silentWeapons[i].maxDistance / 10.0f;
        }

        // AntiAim
        state.antiAim.enabled = (p.antiAimEnabled != 0);
        state.antiAim.pitchMode = p.antiAimPitchMode;
        state.antiAim.yawMode = p.antiAimYawMode;
        state.antiAim.spinSpeed = p.antiAimSpinSpeed;
        state.antiAim.fakeLag = (p.antiAimFakeLag != 0);
        state.antiAim.fakeLagLimit = p.antiAimFakeLagLimit;
        state.antiAim.desync = (p.antiAimDesync != 0);
        state.antiAim.invertebred = (p.antiAimInvertebred != 0);

        // Player
        uint16_t pb = p.playerBools;
        state.player.godmode = (pb & (1 << 0)) != 0;
        state.player.infAmmo = (pb & (1 << 1)) != 0;
        state.player.infStamina = (pb & (1 << 2)) != 0;
        state.player.fastRun = (pb & (1 << 3)) != 0;
        state.player.megaJump = (pb & (1 << 4)) != 0;
        state.player.antiStun = (pb & (1 << 5)) != 0;
        state.player.fastReload = (pb & (1 << 6)) != 0;
        state.player.autoCBug = (pb & (1 << 7)) != 0;
        state.player.noSpread = (pb & (1 << 8)) != 0;
        state.player.antiHS = (pb & (1 << 9)) != 0;

        // Vehicle
        uint8_t vb2 = p.vehicleBools;
        state.vehicle.engineAlwaysOn = (vb2 & (1 << 0)) != 0;
        state.vehicle.carGodmode = (vb2 & (1 << 1)) != 0;
        state.vehicle.autoFlip = (vb2 & (1 << 2)) != 0;
        state.vehicle.flyCar = (vb2 & (1 << 3)) != 0;
        state.vehicle.instantRepair = (vb2 & (1 << 4)) != 0;
        state.vehicle.noBikeFall = (vb2 & (1 << 5)) != 0;
        state.vehicle.speedMultiplier = p.vehicleSpeedMultiplier;

        // KFC Slide
        uint8_t kfcb = p.kfcSlideBools;
        state.kfcSlide.enabled = (kfcb & (1 << 0)) != 0;
        state.kfcSlide.speed = p.kfcSlideSpeed / 10.0f;
        state.kfcSlide.durationMs = p.kfcSlideDurationMs;

        // Misc
        uint8_t mb = p.miscBools;
        state.misc.watermark = (mb & (1 << 0)) != 0;
        state.misc.particles = (mb & (1 << 1)) != 0;
    }

    bool SaveToCloud(const std::string& configName, std::string& outMsg)
    {
        AccountInfo acc = GetAccountInfo();
        if (acc.sessionId.empty())
        {
            outMsg = "Sessao nao encontrada. Faca login pelo loader primeiro.";
            return false;
        }

        // Serializacao compacta binaria (aprox. 180 bytes -> ~240 caracteres Base64, respeitando o limite de 500 do KeyAuth)
        CompactCloudPayload payload = SerializeCompact(g_MenuState);
        std::string rawData(reinterpret_cast<const char*>(&payload), sizeof(payload));
        std::string b64 = Base64Encode(rawData);
        std::string encodedData = UrlEncode(b64);
        std::string encodedVar = UrlEncode("cfg_" + configName);

        std::string name = acc.keyauthName.empty() ? "somalia" : acc.keyauthName;
        std::string owner = acc.keyauthOwner.empty() ? "5bU1fK1ki3" : acc.keyauthOwner;

        std::string postData = "type=setvar&var=" + encodedVar + "&data=" + encodedData +
                               "&sessionid=" + acc.sessionId + "&name=" + name + "&ownerid=" + owner;

        std::string resp = KeyAuthPost(postData);
        if (resp.find("\"success\":true") != std::string::npos || resp.find("\"success\": true") != std::string::npos)
        {
            outMsg = "Configuracao salva na Nuvem com sucesso!";
            return true;
        }

        std::string apiMsg = ExtractJsonValue(resp, "message");
        if (!apiMsg.empty())
            outMsg = "KeyAuth: " + apiMsg;
        else
            outMsg = "Falha ao salvar na Nuvem. Verifique sua conexao.";

        return false;
    }

    bool LoadFromCloud(const std::string& configName, std::string& outMsg)
    {
        AccountInfo acc = GetAccountInfo();
        if (acc.sessionId.empty())
        {
            outMsg = "Sessao nao encontrada. Faca login pelo loader primeiro.";
            return false;
        }

        std::string encodedVar = UrlEncode("cfg_" + configName);
        std::string name = acc.keyauthName.empty() ? "somalia" : acc.keyauthName;
        std::string owner = acc.keyauthOwner.empty() ? "5bU1fK1ki3" : acc.keyauthOwner;

        std::string postData = "type=getvar&var=" + encodedVar +
                               "&sessionid=" + acc.sessionId + "&name=" + name + "&ownerid=" + owner;

        std::string resp = KeyAuthPost(postData);
        if (resp.find("\"success\":true") != std::string::npos || resp.find("\"success\": true") != std::string::npos)
        {
            std::string b64 = ExtractJsonValue(resp, "response");
            if (!b64.empty())
            {
                std::string decoded = Base64Decode(b64);
                // 1. Tenta deserializar o formato binario compacto SOMC
                if (decoded.size() >= sizeof(CompactCloudPayload) &&
                    decoded[0] == 'S' && decoded[1] == 'O' && decoded[2] == 'M' && decoded[3] == 'C')
                {
                    DeserializeCompact(g_MenuState, *reinterpret_cast<const CompactCloudPayload*>(decoded.data()));
                    outMsg = "Configuracao carregada da Nuvem com sucesso!";
                    return true;
                }
                // 2. Fallback para formato legado JSON
                else if (!decoded.empty() && LoadFromString(decoded.c_str()))
                {
                    outMsg = "Configuracao carregada da Nuvem com sucesso!";
                    return true;
                }
            }
        }

        std::string apiMsg = ExtractJsonValue(resp, "message");
        if (!apiMsg.empty())
            outMsg = "KeyAuth: " + apiMsg;
        else
            outMsg = "Configuracao nao encontrada na Nuvem.";

        return false;
    }
}

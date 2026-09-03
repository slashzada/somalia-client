#include "Config.h"
#include "../Core/Logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include <wininet.h>

#pragma comment(lib, "wininet.lib")

MenuState g_MenuState;

namespace Config
{
    void ResetToDefaults()
    {
        g_MenuState.visuals = VisualsConfig();
        g_MenuState.legitBot = LegitBotConfig();
        g_MenuState.rageBot = RageBotConfig();
        g_MenuState.antiAim = AntiAimConfig();
        g_MenuState.player = PlayerConfig();
        g_MenuState.vehicle = VehicleConfig();
        g_MenuState.misc = MiscConfig();
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
        AppendFmt(out, "    \"timeHour\": %d,\n", g_MenuState.visuals.timeHour);
        AppendFmt(out, "    \"vehicleESP\": %s,\n", g_MenuState.visuals.vehicleESP ? "true" : "false");
        AppendFmt(out, "    \"pickupESP\": %s,\n", g_MenuState.visuals.pickupESP ? "true" : "false");
        AppendFmt(out, "    \"objectESP\": %s,\n", g_MenuState.visuals.objectESP ? "true" : "false");
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
        AppendFmt(out, "    \"noSpread\": %s\n", g_MenuState.player.noSpread ? "true" : "false");
        AppendFmt(out, "  },\n");

        // 5. VEHICLE
        AppendFmt(out, "  \"vehicle\": {\n");
        AppendFmt(out, "    \"engineAlwaysOn\": %s,\n", g_MenuState.vehicle.engineAlwaysOn ? "true" : "false");
        AppendFmt(out, "    \"carGodmode\": %s,\n", g_MenuState.vehicle.carGodmode ? "true" : "false");
        AppendFmt(out, "    \"speedMultiplier\": %d,\n", g_MenuState.vehicle.speedMultiplier);
        AppendFmt(out, "    \"autoFlip\": %s,\n", g_MenuState.vehicle.autoFlip ? "true" : "false");
        AppendFmt(out, "    \"instantRepair\": %s,\n", g_MenuState.vehicle.instantRepair ? "true" : "false");
        AppendFmt(out, "    \"noBikeFall\": %s,\n", g_MenuState.vehicle.noBikeFall ? "true" : "false");
        AppendFmt(out, "    \"flyCar\": %s\n", g_MenuState.vehicle.flyCar ? "true" : "false");
        AppendFmt(out, "  },\n");

        // 6. SLIDE
        AppendFmt(out, "  \"slide\": {\n");
        AppendFmt(out, "    \"enabled\": %s,\n", g_MenuState.slide.enabled ? "true" : "false");
        AppendFmt(out, "    \"cSlideActive\": %s,\n", g_MenuState.slide.cSlideActive ? "true" : "false");
        AppendFmt(out, "    \"autoSlideActive\": %s,\n", g_MenuState.slide.autoSlideActive ? "true" : "false");
        AppendFmt(out, "    \"durationC\": %d,\n", g_MenuState.slide.durationC);
        AppendFmt(out, "    \"delayTroca\": %d,\n", g_MenuState.slide.delayTroca);
        AppendFmt(out, "    \"slideBoost\": %.2f,\n", g_MenuState.slide.slideBoost);
        AppendFmt(out, "    \"marginDeagle\": %d,\n", g_MenuState.slide.marginDeagle);
        AppendFmt(out, "    \"marginShotgun\": %d,\n", g_MenuState.slide.marginShotgun);
        AppendFmt(out, "    \"marginSniper\": %d,\n", g_MenuState.slide.marginSniper);
        AppendFmt(out, "    \"marginM4\": %d,\n", g_MenuState.slide.marginM4);
        AppendFmt(out, "    \"marginAK47\": %d\n", g_MenuState.slide.marginAK47);
        AppendFmt(out, "  },\n");

        // 7. MISC
        AppendFmt(out, "  \"misc\": {\n");
        AppendFmt(out, "    \"particles\": %s,\n", g_MenuState.misc.particles ? "true" : "false");
        AppendFmt(out, "    \"watermark\": %s\n", g_MenuState.misc.watermark ? "true" : "false");
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
            tempState.vehicle.flyCar = ParseBool(pVehicle, "\"flyCar\"", tempState.vehicle.flyCar);
        }

        // 6. Parse Slide
        const char* pSlide = strstr(buffer, "\"slide\"");
        if (pSlide)
        {
            tempState.slide.enabled = ParseBool(pSlide, "\"enabled\"", tempState.slide.enabled);
            tempState.slide.cSlideActive = ParseBool(pSlide, "\"cSlideActive\"", tempState.slide.cSlideActive);
            tempState.slide.autoSlideActive = ParseBool(pSlide, "\"autoSlideActive\"", tempState.slide.autoSlideActive);
            tempState.slide.durationC = ParseInt(pSlide, "\"durationC\"", tempState.slide.durationC);
            tempState.slide.delayTroca = ParseInt(pSlide, "\"delayTroca\"", tempState.slide.delayTroca);
            tempState.slide.slideBoost = ParseFloat(pSlide, "\"slideBoost\"", tempState.slide.slideBoost);
            tempState.slide.marginDeagle = ParseInt(pSlide, "\"marginDeagle\"", tempState.slide.marginDeagle);
            tempState.slide.marginShotgun = ParseInt(pSlide, "\"marginShotgun\"", tempState.slide.marginShotgun);
            tempState.slide.marginSniper = ParseInt(pSlide, "\"marginSniper\"", tempState.slide.marginSniper);
            tempState.slide.marginM4 = ParseInt(pSlide, "\"marginM4\"", tempState.slide.marginM4);
            tempState.slide.marginAK47 = ParseInt(pSlide, "\"marginAK47\"", tempState.slide.marginAK47);
        }

        // 7. Parse Misc
        const char* pMisc = strstr(buffer, "\"misc\"");
        if (pMisc)
        {
            tempState.misc.particles = ParseBool(pMisc, "\"particles\"", tempState.misc.particles);
            tempState.misc.watermark = ParseBool(pMisc, "\"watermark\"", tempState.misc.watermark);
        }

        // Validação e Clamping defensivo
        if (tempState.visuals.maxDistance < 10) tempState.visuals.maxDistance = 10;
        if (tempState.visuals.maxDistance > 1000) tempState.visuals.maxDistance = 1000;
        if (tempState.visuals.fovCircleRadius < 5) tempState.visuals.fovCircleRadius = 5;
        if (tempState.visuals.fovCircleRadius > 500) tempState.visuals.fovCircleRadius = 500;
        if (tempState.visuals.timeHour < 0) tempState.visuals.timeHour = 0;
        if (tempState.visuals.timeHour > 23) tempState.visuals.timeHour = 23;

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

        // Aplicação atômica do estado validado
        g_MenuState = tempState;
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

    static std::string ReadSessionIdFromClientJson()
    {
        std::ifstream f("somalia_client.json");
        if (!f.is_open()) return "";
        std::string str((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        size_t p = str.find("\"session_id\"");
        if (p == std::string::npos) return "";
        size_t colon = str.find(':', p);
        if (colon == std::string::npos) return "";
        size_t start = str.find('\"', colon);
        if (start == std::string::npos) return "";
        size_t end = str.find('\"', start + 1);
        if (end == std::string::npos) return "";
        return str.substr(start + 1, end - start - 1);
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

    bool SaveToCloud(const std::string& configName, std::string& outMsg)
    {
        std::string sid = ReadSessionIdFromClientJson();
        if (sid.empty())
        {
            outMsg = "Sessao nao encontrada. Faca login pelo loader primeiro.";
            return false;
        }

        std::string jsonStr = SaveToString();
        std::string b64 = Base64Encode(jsonStr);

        std::string postData = "type=setvar&var=cfg_" + configName + "&data=" + b64 +
                               "&sessionid=" + sid + "&name=somalia&ownerid=5bU1fK1ki3";

        std::string resp = KeyAuthPost(postData);
        if (resp.find("\"success\":true") != std::string::npos || resp.find("\"success\": true") != std::string::npos)
        {
            outMsg = "Configuracao salva na Nuvem com sucesso!";
            return true;
        }

        outMsg = "Falha ao salvar na Nuvem. Verifique sua conexao.";
        return false;
    }

    bool LoadFromCloud(const std::string& configName, std::string& outMsg)
    {
        std::string sid = ReadSessionIdFromClientJson();
        if (sid.empty())
        {
            outMsg = "Sessao nao encontrada. Faca login pelo loader primeiro.";
            return false;
        }

        std::string postData = "type=getvar&var=cfg_" + configName +
                               "&sessionid=" + sid + "&name=somalia&ownerid=5bU1fK1ki3";

        std::string resp = KeyAuthPost(postData);
        if (resp.find("\"success\":true") != std::string::npos || resp.find("\"success\": true") != std::string::npos)
        {
            size_t p = resp.find("\"response\"");
            if (p != std::string::npos)
            {
                size_t colon = resp.find(':', p);
                if (colon != std::string::npos)
                {
                    size_t start = resp.find('\"', colon);
                    if (start != std::string::npos)
                    {
                        size_t end = resp.find('\"', start + 1);
                        if (end != std::string::npos)
                        {
                            std::string b64 = resp.substr(start + 1, end - start - 1);
                            std::string decoded = Base64Decode(b64);
                            if (!decoded.empty() && LoadFromString(decoded.c_str()))
                            {
                                outMsg = "Configuracao carregada da Nuvem com sucesso!";
                                return true;
                            }
                        }
                    }
                }
            }
        }

        outMsg = "Configuracao nao encontrada na Nuvem.";
        return false;
    }
}

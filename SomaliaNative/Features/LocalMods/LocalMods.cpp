#include "LocalMods.h"
#include "../../Config/Config.h"
#include "../../Core/Logger.h"
#include "../../Core/RuntimeState.h"
#include "../../Engine/SAMP/SAMP.h"
#include <math.h>

namespace LocalMods
{
    static ULONGLONG s_lastRepairTick = 0;
    static bool s_wasInfAmmoActive = false;
    static bool s_wasNoBikeFallActive = false;
    static bool s_wasWeatherActive = false;
    static ULONGLONG s_LastCBugShotTick = 0;
    static bool s_CBugCrouched = false;
    static ULONGLONG s_CBugCrouchTick = 0;

    void Reset()
    {
        __try
        {
            if (s_wasInfAmmoActive)
            {
                *reinterpret_cast<uint8_t*>(0x0096C008) = 0;
                s_wasInfAmmoActive = false;
            }

            if (s_wasNoBikeFallActive)
            {
                *reinterpret_cast<uint8_t*>(0x00B6F03C) = 0;
                s_wasNoBikeFallActive = false;
            }

            if (s_wasWeatherActive)
            {
                *reinterpret_cast<int16_t*>(0x00C81320) = -1;
                s_wasWeatherActive = false;
            }

            if (s_CBugCrouched)
            {
                keybd_event('C', 0, KEYEVENTF_KEYUP, 0);
                s_CBugCrouched = false;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    void UpdatePlayerMods()
    {
        if (!RuntimeState::IsPlayerAlive())
            return;

        __try
        {
            void* pLocalPed = RuntimeState::GetLocalPed();
            if (!pLocalPed)
                return;

            uintptr_t pedAddr = reinterpret_cast<uintptr_t>(pLocalPed);

            // 1. INFINITE STAMINA (Trava estamina em 100.0f)
            if (g_MenuState.player.infStamina)
            {
                *reinterpret_cast<float*>(0x00B7CEE4) = 100.0f;
            }

            // 2. INFINITE AMMO (Flag de trapaca nativa do GTA SA 1.0 US: 0x0096C008)
            if (g_MenuState.player.infAmmo)
            {
                *reinterpret_cast<uint8_t*>(0x0096C008) = 1;
                s_wasInfAmmoActive = true;
            }
            else if (s_wasInfAmmoActive)
            {
                *reinterpret_cast<uint8_t*>(0x0096C008) = 0;
                s_wasInfAmmoActive = false;
            }

            // 3. NO BIKE FALL (Impede queda de motocicletas e bicicletas: 0x00B6F03C)
            if (g_MenuState.vehicle.noBikeFall)
            {
                *reinterpret_cast<uint8_t*>(0x00B6F03C) = 1;
                s_wasNoBikeFallActive = true;
            }
            else if (s_wasNoBikeFallActive)
            {
                *reinterpret_cast<uint8_t*>(0x00B6F03C) = 0;
                s_wasNoBikeFallActive = false;
            }

            // 4. LOCAL GODMODE (Imunidade fisica a tiros, fogo, explosao, colisoes e porrada: +0x42)
            if (g_MenuState.player.godmode)
            {
                *reinterpret_cast<uint8_t*>(pedAddr + 0x42) |= 0xFC; // bBulletProof, bFireProof, bCollisionProof, bMeleeProof, bInvulnerable, bExplosionProof

                float* pHealth = reinterpret_cast<float*>(pedAddr + 0x540);
                if (*pHealth < 100.0f && *pHealth > 0.0f)
                {
                    *pHealth = 100.0f;
                }
            }

            // 5. FAST SPRINT (Acelera a velocidade de corrida sem disparar anti-cheat de speedhack)
            if (g_MenuState.player.fastRun)
            {
                float* pMoveSpeedX = reinterpret_cast<float*>(pedAddr + 0x44);
                float* pMoveSpeedY = reinterpret_cast<float*>(pedAddr + 0x48);
                float speed2D = sqrtf((*pMoveSpeedX) * (*pMoveSpeedX) + (*pMoveSpeedY) * (*pMoveSpeedY));
                if (speed2D > 0.03f && speed2D < 0.26f)
                {
                    *pMoveSpeedX *= 1.15f;
                    *pMoveSpeedY *= 1.15f;
                }
            }

            // 6. MEGA JUMP (Impulso vertical ampliado com amortecimento nativo)
            if (g_MenuState.player.megaJump)
            {
                if (GetAsyncKeyState(VK_SPACE) & 0x8000)
                {
                    float* pMoveSpeedZ = reinterpret_cast<float*>(pedAddr + 0x4C);
                    if (*pMoveSpeedZ > 0.04f && *pMoveSpeedZ < 0.20f)
                    {
                        *pMoveSpeedZ += 0.22f;
                    }
                }
            }

            // 7. ANTI-STUN (Imunidade a travamento de tropeco, sem interferir na morte)
            if (g_MenuState.player.antiStun)
            {
                float hp = *reinterpret_cast<float*>(pedAddr + 0x540);
                if (hp > 0.0f)
                {
                    uint32_t* pPedState = reinterpret_cast<uint32_t*>(pedAddr + 0x530);
                    if (*pPedState == 0x38) // STUMBLE only - JAMAIS altera 0x36 ou 0x37 (DEAD/DIE)!
                    {
                        *pPedState = 1; // PED_STATE_IDLE
                    }
                }
            }

            // 8. ANTI-HS (Protecao local contra dano letal excessivo de tiro na cabeca)
            if (g_MenuState.player.antiHS)
            {
                if (SAMP::IsLoaded())
                {
                    SAMP::EnsureSendTakeDamageHook();
                }

                float* pHealth = reinterpret_cast<float*>(pedAddr + 0x540);
                if (pHealth && !IsBadReadPtr(pHealth, sizeof(float)))
                {
                    static float s_LastHealth = 100.0f;
                    float curHealth = *pHealth;

                    // Se a vida aumentou (cura, comida, spawn)
                    if (curHealth > s_LastHealth)
                    {
                        s_LastHealth = curHealth;
                    }
                    // Se houve dano
                    else if (curHealth < s_LastHealth)
                    {
                        float lostHp = s_LastHealth - curHealth;
                        uint32_t* pPedState = reinterpret_cast<uint32_t*>(pedAddr + 0x530);

                        // Se a vida zerou ou ped entrou em estado de morte repentina por headshot crítico
                        if (curHealth <= 0.0f || (pPedState && (*pPedState == 0x36 || *pPedState == 0x37)))
                        {
                            *pHealth = (s_LastHealth > g_MenuState.player.antiHSDamageCap) ?
                                       (s_LastHealth - g_MenuState.player.antiHSDamageCap) : 15.0f;
                            if (*pHealth <= 0.0f) *pHealth = 15.0f;

                            if (pPedState && (*pPedState == 0x36 || *pPedState == 0x37))
                            {
                                *pPedState = 1; // PED_STATE_IDLE
                            }
                            Logger::Log("[SOMALIA][ANTI-HS] Morte local impedida! Vida restaurada para %.1f HP", *pHealth);
                        }
                        else if (lostHp > g_MenuState.player.antiHSDamageCap)
                        {
                            *pHealth = s_LastHealth - g_MenuState.player.antiHSDamageCap;
                            Logger::Log("[SOMALIA][ANTI-HS] Dano local regulado! Perda de %.1f HP limitada para %.1f HP. Vida restante: %.1f",
                                lostHp, g_MenuState.player.antiHSDamageCap, *pHealth);
                        }
                    }

                    // Se a vida restante for crítica (<= 5 HP), amortece para evitar morte imediata
                    if (*pHealth > 0.0f && *pHealth <= 5.0f)
                    {
                        *pHealth = 15.0f;
                    }

                    s_LastHealth = *pHealth;
                }
            }

            // 9. FALL PROOF (Imunidade a dano de queda)
            if (g_MenuState.player.fallProof)
            {
                // Protege contra queda sem corromper moveSpeed
                float* pMoveZ = reinterpret_cast<float*>(pedAddr + 0x4C);
                if (pMoveZ && *pMoveZ < -0.40f)
                {
                    *pMoveZ = -0.15f; // Amortece velocidade terminal de queda
                }
            }

            // 10. AUTO BHOP (Salto automatico continuo)
            if (g_MenuState.player.autoBhop && !g_MenuState.player.megaJump)
            {
                if (GetAsyncKeyState(VK_SPACE) & 0x8000)
                {
                    float* pMoveSpeedZ = reinterpret_cast<float*>(pedAddr + 0x4C);
                    if (fabsf(*pMoveSpeedZ) < 0.02f)
                    {
                        *pMoveSpeedZ = 0.14f;
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    void UpdateCombatHelpers()
    {
        if (!RuntimeState::IsPlayerAlive())
            return;

        __try
        {
            void* pLocalPed = RuntimeState::GetLocalPed();
            if (!pLocalPed)
                return;

            uintptr_t pedAddr = reinterpret_cast<uintptr_t>(pLocalPed);

            // 1. FAST WEAPON RELOAD (Recarrega instantaneamente o pente)
            if (g_MenuState.player.fastReload)
            {
                uint8_t slot = *reinterpret_cast<uint8_t*>(pedAddr + 0x718);
                uintptr_t weaponPtr = pedAddr + 0x5A0 + slot * 0x1C;
                uint32_t weaponType = *reinterpret_cast<uint32_t*>(weaponPtr);

                if (weaponType > 0)
                {
                    uint32_t* pAmmoInClip = reinterpret_cast<uint32_t*>(weaponPtr + 0x8);
                    uint32_t* pTotalAmmo = reinterpret_cast<uint32_t*>(weaponPtr + 0xC);
                    uint32_t* pWeaponState = reinterpret_cast<uint32_t*>(weaponPtr + 0x10);

                    if ((*pWeaponState == 3 || *pAmmoInClip == 0) && *pTotalAmmo > 0)
                    {
                        uint32_t clipCap = 30;
                        if (weaponType == 24) clipCap = 7;       // Deagle
                        else if (weaponType == 25) clipCap = 1;  // Shotgun
                        else if (weaponType == 26) clipCap = 2;  // Sawnoff
                        else if (weaponType == 27) clipCap = 7;  // Combat Shotgun
                        else if (weaponType == 34) clipCap = 1;  // Sniper Rifle
                        else if (weaponType == 29) clipCap = 30; // MP5

                        uint32_t reloadAmount = (*pTotalAmmo < clipCap) ? *pTotalAmmo : clipCap;
                        *pAmmoInClip = reloadAmount;
                        *pWeaponState = 1; // Pronto para atirar
                    }
                }
            }

            // 2. AUTOMATIC C-BUG HELPER (Sequenciamento de cancelamento de animacao pos-disparo)
            if (g_MenuState.player.autoCBug && !g_MenuState.menuOpen)
            {
                bool isAiming = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
                bool isShooting = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
                ULONGLONG currentTick = GetTickCount64();

                if (isAiming && isShooting)
                {
                    if (currentTick - s_LastCBugShotTick > 450)
                    {
                        s_LastCBugShotTick = currentTick;
                        s_CBugCrouched = true;
                        s_CBugCrouchTick = currentTick;
                        keybd_event('C', 0, 0, 0); // Pressiona C
                    }
                }

                if (s_CBugCrouched && (currentTick - s_CBugCrouchTick > 35))
                {
                    keybd_event('C', 0, KEYEVENTF_KEYUP, 0); // Solta C
                    s_CBugCrouched = false;
                }
            }

            // 3. NO SPREAD (Trava dispersao conica e mantem precisao cirurgica nas armas)
            if (g_MenuState.player.noSpread)
            {
                for (int wId = 22; wId <= 34; wId++)
                {
                    uintptr_t pWepInfo = 0x00C8AAB8 + (wId * 0x70);
                    float* pAccuracy = reinterpret_cast<float*>(pWepInfo + 0x20);
                    if (pAccuracy)
                    {
                        *pAccuracy = 1.0f; // Maxima precisao
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    void UpdateVehicleMods()
    {
        if (!RuntimeState::IsPlayerAlive())
            return;

        __try
        {
            void* pLocalPed = RuntimeState::GetLocalPed();
            if (!pLocalPed)
                return;

            uintptr_t pedAddr = reinterpret_cast<uintptr_t>(pLocalPed);

            void* pVehicle = *reinterpret_cast<void**>(pedAddr + 0x58C);
            if (!pVehicle)
                return;

            uintptr_t vehAddr = reinterpret_cast<uintptr_t>(pVehicle);

            // 1. ENGINE ALWAYS ON
            if (g_MenuState.vehicle.engineAlwaysOn)
            {
                *reinterpret_cast<uint8_t*>(vehAddr + 0x428) |= 0x10;
            }

            // 2. CAR GODMODE
            if (g_MenuState.vehicle.carGodmode)
            {
                *reinterpret_cast<uint8_t*>(vehAddr + 0x48) |= 0x1F;

                float* pVehHealth = reinterpret_cast<float*>(vehAddr + 0x4C0);
                if (*pVehHealth < 1000.0f && *pVehHealth > 0.0f)
                {
                    *pVehHealth = 1000.0f;
                }
            }

            // 3. INSTANT REPAIR
            if (g_MenuState.vehicle.instantRepair)
            {
                ULONGLONG currentTick = GetTickCount64();
                if ((GetAsyncKeyState('R') & 0x8000) && (currentTick - s_lastRepairTick > 800))
                {
                    s_lastRepairTick = currentTick;
                    *reinterpret_cast<float*>(vehAddr + 0x4C0) = 1000.0f;
                    *reinterpret_cast<uint8_t*>(vehAddr + 0x428) |= 0x10;
                    Logger::Log("[SOMALIA][LOCALMODS] Veiculo reparado instantaneamente (1000.0 HP)");
                }
            }

            // 4. AUTO FLIP
            if (g_MenuState.vehicle.autoFlip)
            {
                uintptr_t pMatrix = *reinterpret_cast<uintptr_t*>(vehAddr + 0x14);
                if (pMatrix)
                {
                    float upZ = *reinterpret_cast<float*>(pMatrix + 0x28);
                    if (upZ < -0.15f)
                    {
                        *reinterpret_cast<float*>(pMatrix + 0x20) = 0.0f;
                        *reinterpret_cast<float*>(pMatrix + 0x24) = 0.0f;
                        *reinterpret_cast<float*>(pMatrix + 0x28) = 1.0f;
                        *reinterpret_cast<float*>(vehAddr + 0x50) = 0.0f;
                        *reinterpret_cast<float*>(vehAddr + 0x54) = 0.0f;
                        *reinterpret_cast<float*>(vehAddr + 0x58) = 0.0f;
                        *reinterpret_cast<float*>(pMatrix + 0x38) += 0.08f;
                    }
                }
            }

            // 5. SPEED MULTIPLIER
            if (g_MenuState.vehicle.speedMultiplier > 1 && !g_MenuState.vehicle.flyCar)
            {
                if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
                {
                    float* pMoveSpeedX = reinterpret_cast<float*>(vehAddr + 0x44);
                    float* pMoveSpeedY = reinterpret_cast<float*>(vehAddr + 0x48);

                    float currentSpeed2D = sqrtf((*pMoveSpeedX) * (*pMoveSpeedX) + (*pMoveSpeedY) * (*pMoveSpeedY));
                    if (currentSpeed2D > 0.05f)
                    {
                        float factor = 1.0f + (g_MenuState.vehicle.speedMultiplier - 1) * 0.015f;
                        *pMoveSpeedX *= factor;
                        *pMoveSpeedY *= factor;
                    }
                }
            }

            // 6. FLY CAR MODE
            if (g_MenuState.vehicle.flyCar)
            {
                float* pMoveSpeedX = reinterpret_cast<float*>(vehAddr + 0x44);
                float* pMoveSpeedY = reinterpret_cast<float*>(vehAddr + 0x48);
                float* pMoveSpeedZ = reinterpret_cast<float*>(vehAddr + 0x4C);

                if (*pMoveSpeedZ < 0.0f)
                {
                    *pMoveSpeedZ = 0.003f;
                }

                if (GetAsyncKeyState(VK_SPACE) & 0x8000)
                {
                    *pMoveSpeedZ = 0.28f;
                }
                else if ((GetAsyncKeyState(VK_LSHIFT) & 0x8000) || (GetAsyncKeyState(VK_LCONTROL) & 0x8000))
                {
                    *pMoveSpeedZ = -0.20f;
                }

                if (GetAsyncKeyState('W') & 0x8000)
                {
                    uintptr_t pMatrix = *reinterpret_cast<uintptr_t*>(vehAddr + 0x14);
                    if (pMatrix)
                    {
                        float fwdX = *reinterpret_cast<float*>(pMatrix + 0x10);
                        float fwdY = *reinterpret_cast<float*>(pMatrix + 0x14);
                        *pMoveSpeedX = fwdX * 0.70f;
                        *pMoveSpeedY = fwdY * 0.70f;
                    }
                }
                else if (GetAsyncKeyState('S') & 0x8000)
                {
                    *pMoveSpeedX *= 0.88f;
                    *pMoveSpeedY *= 0.88f;
                }

                uintptr_t pMatrix = *reinterpret_cast<uintptr_t*>(vehAddr + 0x14);
                if (pMatrix)
                {
                    *reinterpret_cast<float*>(pMatrix + 0x20) = 0.0f;
                    *reinterpret_cast<float*>(pMatrix + 0x24) = 0.0f;
                    *reinterpret_cast<float*>(pMatrix + 0x28) = 1.0f;
                    *reinterpret_cast<float*>(vehAddr + 0x50) = 0.0f;
                    *reinterpret_cast<float*>(vehAddr + 0x54) = 0.0f;
                    *reinterpret_cast<float*>(vehAddr + 0x58) = 0.0f;
                }
            }

            // 7. SUPER BRAKE (Freio instantaneo com barra de espaco ou 'S')
            if (g_MenuState.vehicle.superBrake)
            {
                if ((GetAsyncKeyState(VK_SPACE) & 0x8000) || (GetAsyncKeyState('S') & 0x8000))
                {
                    float* pMoveSpeedX = reinterpret_cast<float*>(vehAddr + 0x44);
                    float* pMoveSpeedY = reinterpret_cast<float*>(vehAddr + 0x48);
                    *pMoveSpeedX *= 0.82f;
                    *pMoveSpeedY *= 0.82f;
                }
            }

            // 8. HEAVY VEHICLE (Massa extrema para colisoes)
            if (g_MenuState.vehicle.heavyVehicle)
            {
                *reinterpret_cast<float*>(vehAddr + 0x8C) = 50000.0f;
            }

            // 9. DRIFT MODE (Reduz tracao para permitir derrapagens controladas)
            if (g_MenuState.vehicle.driftMode)
            {
                static float s_OrigTraction = -1.0f;
                uintptr_t pHandling = *reinterpret_cast<uintptr_t*>(vehAddr + 0x384);
                if (pHandling)
                {
                    float* pTraction = reinterpret_cast<float*>(pHandling + 0xA8);
                    if (s_OrigTraction < 0.0f)
                        s_OrigTraction = *pTraction;
                    *pTraction = 0.35f; // Tracao reduzida para drift
                }
            }

            // 10. UNLIMITED NITRO (Flag nativa de NOS infinito do GTA SA)
            if (g_MenuState.vehicle.unlimitedNitro)
            {
                *reinterpret_cast<uint8_t*>(0x00969165) = 1;
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    void UpdateWorldMods()
    {
        __try
        {
            // 1. NIGHT MODE
            if (g_MenuState.visuals.nightMode)
            {
                *reinterpret_cast<int16_t*>(0x00C81320) = 22;
                *reinterpret_cast<uint8_t*>(0x00B70153) = 0;
                *reinterpret_cast<uint8_t*>(0x00B70152) = 0;
                s_wasWeatherActive = true;
            }
            else
            {
                // 2. WEATHER CHANGER
                if (g_MenuState.visuals.weatherChanger)
                {
                    int16_t targetWeather = 0;
                    switch (g_MenuState.visuals.weatherID)
                    {
                    case 0: targetWeather = 0;  break;
                    case 1: targetWeather = 9;  break;
                    case 2: targetWeather = 8;  break;
                    case 3: targetWeather = 22; break;
                    case 4: targetWeather = 10; break;
                    default: targetWeather = static_cast<int16_t>(g_MenuState.visuals.weatherID); break;
                    }

                    *reinterpret_cast<int16_t*>(0x00C81320) = targetWeather;
                    s_wasWeatherActive = true;
                }
                else if (s_wasWeatherActive)
                {
                    *reinterpret_cast<int16_t*>(0x00C81320) = -1;
                    s_wasWeatherActive = false;
                }

                // 3. TIME CHANGER / LOCK HOUR
                if (g_MenuState.visuals.timeChanger || g_MenuState.visuals.lockHour)
                {
                    uint8_t targetHour = static_cast<uint8_t>(g_MenuState.visuals.timeHour % 24);
                    *reinterpret_cast<uint8_t*>(0x00B70153) = targetHour;
                    *reinterpret_cast<uint8_t*>(0x00B70152) = 0;
                }
            }

            // 4. NO FOG / EXTENDED DRAW DISTANCE (Remove nevoa do horizonte)
            if (g_MenuState.visuals.noFog || g_MenuState.visuals.extendedDrawDist)
            {
                *reinterpret_cast<float*>(0x00B79038) = 3500.0f; // FarClip
                *reinterpret_cast<float*>(0x00B7903C) = 3500.0f; // FogClip
            }

            // 5. FULLBRIGHT / AMBIENT BOOST
            if (g_MenuState.visuals.fullbright)
            {
                *reinterpret_cast<float*>(0x00B79E40) = 0.90f;
                *reinterpret_cast<float*>(0x00B79E44) = 0.90f;
                *reinterpret_cast<float*>(0x00B79E48) = 0.90f;
            }

            // 6. CAMERA FOV & NO CAM SHAKE
            if (g_MenuState.visuals.customCameraFOV)
            {
                *reinterpret_cast<float*>(0x00B6F028 + 0x60) = g_MenuState.visuals.cameraFOV;
            }
            if (g_MenuState.visuals.noCamShake)
            {
                *reinterpret_cast<float*>(0x00B6F028 + 0x8C) = 0.0f;
            }

            // 7. REMOVE GRASS (NOP CPlantMgr::Render — 0x005DD840)
            {
                static uint8_t s_OrigGrass = 0;
                static bool s_GrassPatched = false;
                if (g_MenuState.visuals.removeGrass && !s_GrassPatched)
                {
                    DWORD oldProt;
                    if (VirtualProtect(reinterpret_cast<void*>(0x005DD840), 1, PAGE_EXECUTE_READWRITE, &oldProt))
                    {
                        s_OrigGrass = *reinterpret_cast<uint8_t*>(0x005DD840);
                        *reinterpret_cast<uint8_t*>(0x005DD840) = 0xC3; // RET
                        VirtualProtect(reinterpret_cast<void*>(0x005DD840), 1, oldProt, &oldProt);
                        s_GrassPatched = true;
                    }
                }
                else if (!g_MenuState.visuals.removeGrass && s_GrassPatched)
                {
                    DWORD oldProt;
                    if (VirtualProtect(reinterpret_cast<void*>(0x005DD840), 1, PAGE_EXECUTE_READWRITE, &oldProt))
                    {
                        *reinterpret_cast<uint8_t*>(0x005DD840) = s_OrigGrass;
                        VirtualProtect(reinterpret_cast<void*>(0x005DD840), 1, oldProt, &oldProt);
                        s_GrassPatched = false;
                    }
                }
            }

            // 8. REMOVE RAIN (NOP CWeather::RenderRainStreaks — 0x0072C430)
            {
                static uint8_t s_OrigRain = 0;
                static bool s_RainPatched = false;
                if (g_MenuState.visuals.removeRain && !s_RainPatched)
                {
                    DWORD oldProt;
                    if (VirtualProtect(reinterpret_cast<void*>(0x0072C430), 1, PAGE_EXECUTE_READWRITE, &oldProt))
                    {
                        s_OrigRain = *reinterpret_cast<uint8_t*>(0x0072C430);
                        *reinterpret_cast<uint8_t*>(0x0072C430) = 0xC3;
                        VirtualProtect(reinterpret_cast<void*>(0x0072C430), 1, oldProt, &oldProt);
                        s_RainPatched = true;
                    }
                }
                else if (!g_MenuState.visuals.removeRain && s_RainPatched)
                {
                    DWORD oldProt;
                    if (VirtualProtect(reinterpret_cast<void*>(0x0072C430), 1, PAGE_EXECUTE_READWRITE, &oldProt))
                    {
                        *reinterpret_cast<uint8_t*>(0x0072C430) = s_OrigRain;
                        VirtualProtect(reinterpret_cast<void*>(0x0072C430), 1, oldProt, &oldProt);
                        s_RainPatched = false;
                    }
                }
            }

            // 9. CLEAR WATER (Transparencia da agua — CWaterLevel alpha)
            {
                static float s_OrigWaterAlpha = -1.0f;
                static bool s_WaterPatched = false;
                if (g_MenuState.visuals.clearWater && !s_WaterPatched)
                {
                    DWORD oldProt;
                    if (VirtualProtect(reinterpret_cast<void*>(0x008D37D0), 4, PAGE_EXECUTE_READWRITE, &oldProt))
                    {
                        s_OrigWaterAlpha = *reinterpret_cast<float*>(0x008D37D0);
                        *reinterpret_cast<float*>(0x008D37D0) = 0.3f;
                        VirtualProtect(reinterpret_cast<void*>(0x008D37D0), 4, oldProt, &oldProt);
                        s_WaterPatched = true;
                    }
                }
                else if (!g_MenuState.visuals.clearWater && s_WaterPatched)
                {
                    DWORD oldProt;
                    if (VirtualProtect(reinterpret_cast<void*>(0x008D37D0), 4, PAGE_EXECUTE_READWRITE, &oldProt))
                    {
                        *reinterpret_cast<float*>(0x008D37D0) = s_OrigWaterAlpha;
                        VirtualProtect(reinterpret_cast<void*>(0x008D37D0), 4, oldProt, &oldProt);
                        s_WaterPatched = false;
                    }
                }
            }

            // 10. CLEAR SKY (NOP CClouds::Render — 0x00714190)
            {
                static uint8_t s_OrigClouds = 0;
                static bool s_CloudsPatched = false;
                if (g_MenuState.visuals.clearSky && !s_CloudsPatched)
                {
                    DWORD oldProt;
                    if (VirtualProtect(reinterpret_cast<void*>(0x00714190), 1, PAGE_EXECUTE_READWRITE, &oldProt))
                    {
                        s_OrigClouds = *reinterpret_cast<uint8_t*>(0x00714190);
                        *reinterpret_cast<uint8_t*>(0x00714190) = 0xC3;
                        VirtualProtect(reinterpret_cast<void*>(0x00714190), 1, oldProt, &oldProt);
                        s_CloudsPatched = true;
                    }
                }
                else if (!g_MenuState.visuals.clearSky && s_CloudsPatched)
                {
                    DWORD oldProt;
                    if (VirtualProtect(reinterpret_cast<void*>(0x00714190), 1, PAGE_EXECUTE_READWRITE, &oldProt))
                    {
                        *reinterpret_cast<uint8_t*>(0x00714190) = s_OrigClouds;
                        VirtualProtect(reinterpret_cast<void*>(0x00714190), 1, oldProt, &oldProt);
                        s_CloudsPatched = false;
                    }
                }
            }

            // 11. HIDE RADAR (NOP CRadar::DrawMap — 0x0058A330)
            {
                static uint8_t s_OrigRadar = 0;
                static bool s_RadarPatched = false;
                if (g_MenuState.visuals.hideRadar && !s_RadarPatched)
                {
                    DWORD oldProt;
                    if (VirtualProtect(reinterpret_cast<void*>(0x0058A330), 1, PAGE_EXECUTE_READWRITE, &oldProt))
                    {
                        s_OrigRadar = *reinterpret_cast<uint8_t*>(0x0058A330);
                        *reinterpret_cast<uint8_t*>(0x0058A330) = 0xC3;
                        VirtualProtect(reinterpret_cast<void*>(0x0058A330), 1, oldProt, &oldProt);
                        s_RadarPatched = true;
                    }
                }
                else if (!g_MenuState.visuals.hideRadar && s_RadarPatched)
                {
                    DWORD oldProt;
                    if (VirtualProtect(reinterpret_cast<void*>(0x0058A330), 1, PAGE_EXECUTE_READWRITE, &oldProt))
                    {
                        *reinterpret_cast<uint8_t*>(0x0058A330) = s_OrigRadar;
                        VirtualProtect(reinterpret_cast<void*>(0x0058A330), 1, oldProt, &oldProt);
                        s_RadarPatched = false;
                    }
                }
            }

            // 12. HIDE HUD (NOP CHud::Draw — 0x00589190)
            {
                static uint8_t s_OrigHud = 0;
                static bool s_HudPatched = false;
                if (g_MenuState.visuals.hideHUD && !s_HudPatched)
                {
                    DWORD oldProt;
                    if (VirtualProtect(reinterpret_cast<void*>(0x00589190), 1, PAGE_EXECUTE_READWRITE, &oldProt))
                    {
                        s_OrigHud = *reinterpret_cast<uint8_t*>(0x00589190);
                        *reinterpret_cast<uint8_t*>(0x00589190) = 0xC3;
                        VirtualProtect(reinterpret_cast<void*>(0x00589190), 1, oldProt, &oldProt);
                        s_HudPatched = true;
                    }
                }
                else if (!g_MenuState.visuals.hideHUD && s_HudPatched)
                {
                    DWORD oldProt;
                    if (VirtualProtect(reinterpret_cast<void*>(0x00589190), 1, PAGE_EXECUTE_READWRITE, &oldProt))
                    {
                        *reinterpret_cast<uint8_t*>(0x00589190) = s_OrigHud;
                        VirtualProtect(reinterpret_cast<void*>(0x00589190), 1, oldProt, &oldProt);
                        s_HudPatched = false;
                    }
                }
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
        }
    }

    void Update()
    {
        UpdatePlayerMods();
        UpdateCombatHelpers();
        UpdateVehicleMods();
        UpdateWorldMods();
    }
}

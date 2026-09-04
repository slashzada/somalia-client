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
    static float s_OrigAccuracy[13] = { 0.0f };
    static bool s_OrigAccuracySaved = false;
    static bool s_wasNoSpreadActive = false;

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

            if (s_wasNoSpreadActive || s_OrigAccuracySaved)
            {
                for (int wId = 22; wId <= 34; wId++)
                {
                    uintptr_t pWepInfo = 0x00C8AAB8 + (wId * 0x70);
                    float* pAccuracy = reinterpret_cast<float*>(pWepInfo + 0x20);
                    if (pAccuracy)
                    {
                        *pAccuracy = s_OrigAccuracy[wId - 22];
                    }
                }
                s_wasNoSpreadActive = false;
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

            // 4. LOCAL GODMODE (Imunidade fisica a tiros, fogo, explosao, colisoes e porrada: +0x48)
            if (g_MenuState.player.godmode)
            {
                *reinterpret_cast<uint8_t*>(pedAddr + 0x48) |= 0x1F;

                float* pHealth = reinterpret_cast<float*>(pedAddr + 0x540);
                if (*pHealth < 100.0f && *pHealth > 0.0f)
                {
                    *pHealth = 100.0f;
                }
            }

            // 5. FAST SPRINT (Acelera a velocidade de corrida)
            if (g_MenuState.player.fastRun)
            {
                float* pMoveSpeedX = reinterpret_cast<float*>(pedAddr + 0x44);
                float* pMoveSpeedY = reinterpret_cast<float*>(pedAddr + 0x48);
                float speed2D = sqrtf((*pMoveSpeedX) * (*pMoveSpeedX) + (*pMoveSpeedY) * (*pMoveSpeedY));
                if (speed2D > 0.03f && speed2D < 0.45f)
                {
                    *pMoveSpeedX *= 1.25f;
                    *pMoveSpeedY *= 1.25f;
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
                if (slot < 13)
                {
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
            }

            // 2. AUTOMATIC C-BUG HELPER (Sequenciamento de cancelamento de animacao pos-disparo)
            if (g_MenuState.player.autoCBug && !g_MenuState.menuOpen && !SAMP::HasActiveCursor())
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
                if (!s_OrigAccuracySaved)
                {
                    for (int wId = 22; wId <= 34; wId++)
                    {
                        uintptr_t pWepInfo = 0x00C8AAB8 + (wId * 0x70);
                        float* pAccuracy = reinterpret_cast<float*>(pWepInfo + 0x20);
                        if (pAccuracy)
                        {
                            s_OrigAccuracy[wId - 22] = *pAccuracy;
                        }
                    }
                    s_OrigAccuracySaved = true;
                }

                for (int wId = 22; wId <= 34; wId++)
                {
                    uintptr_t pWepInfo = 0x00C8AAB8 + (wId * 0x70);
                    float* pAccuracy = reinterpret_cast<float*>(pWepInfo + 0x20);
                    if (pAccuracy)
                    {
                        *pAccuracy = 1.0f; // Maxima precisao
                    }
                }
                s_wasNoSpreadActive = true;
            }
            else if (s_wasNoSpreadActive)
            {
                if (s_OrigAccuracySaved)
                {
                    for (int wId = 22; wId <= 34; wId++)
                    {
                        uintptr_t pWepInfo = 0x00C8AAB8 + (wId * 0x70);
                        float* pAccuracy = reinterpret_cast<float*>(pWepInfo + 0x20);
                        if (pAccuracy)
                        {
                            *pAccuracy = s_OrigAccuracy[wId - 22];
                        }
                    }
                }
                s_wasNoSpreadActive = false;
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

                // 3. TIME CHANGER
                if (g_MenuState.visuals.timeChanger)
                {
                    uint8_t targetHour = static_cast<uint8_t>(g_MenuState.visuals.timeHour % 24);
                    *reinterpret_cast<uint8_t*>(0x00B70153) = targetHour;
                    *reinterpret_cast<uint8_t*>(0x00B70152) = 0;
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

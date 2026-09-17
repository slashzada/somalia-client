#include "PlayerSlap.h"
#include "../../Config/Config.h"
#include "../../Engine/SAMP/SAMP.h"
#include "../../Engine/GTA/GTA.h"
#include "../../Core/Logger.h"
#include "../../Core/Main.h"
#include "../../Render/ImGui/imgui.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

namespace PlayerSlap
{
    static NotificationInfo s_Notification;

    void Initialize()
    {
        s_Notification.active = false;
        Logger::Log("[PLAYER_SLAP] Modulo CLEO slapxx.cs inicializado.");
    }

    void Reset()
    {
        s_Notification.active = false;
    }

    void ShowToast(const std::string& msg, uint32_t color, uint32_t durationMs)
    {
        s_Notification.active = true;
        s_Notification.text = msg;
        s_Notification.color = color;
        s_Notification.expireTime = GetTickCount64() + durationMs;
        Logger::Log("[TOAST] %s", msg.c_str());
    }

    const NotificationInfo& GetCurrentNotification()
    {
        return s_Notification;
    }

    // Procura o script slapxx.cs na lista de scripts ativos do GTA para sincronizar com o toggle do menu
    static bool SyncCleoActiveScripts(bool enable)
    {
        bool bAnyFound = false;
        __try
        {
            void** ppActiveScripts = reinterpret_cast<void**>(0x00A8B42C);
            if (!ppActiveScripts || !*ppActiveScripts) return false;

            // Assinatura única dos primeiros 10 bytes de slapxx.cs: "62 06 0E 0B 'b' 'y' ' ' 'w' 'o' 'k'"
            static const uint8_t slapSignature[] = { 0x62, 0x06, 0x0E, 0x0B, 0x62, 0x79, 0x20, 0x77, 0x6F, 0x6B };
            void* pCurrent = *ppActiveScripts;

            while (pCurrent)
            {
                uintptr_t scriptAddr = reinterpret_cast<uintptr_t>(pCurrent);
                bool isMatch = false;

                char* szName = reinterpret_cast<char*>(scriptAddr + 0x08);
                if (szName && !IsBadReadPtr(szName, 8))
                {
                    if (_strnicmp(szName, "slap", 4) == 0)
                        isMatch = true;
                }

                if (!isMatch)
                {
                    uint8_t* pBaseIP = *reinterpret_cast<uint8_t**>(scriptAddr + 0x10);
                    if (pBaseIP && !IsBadReadPtr(pBaseIP, sizeof(slapSignature)))
                    {
                        if (memcmp(pBaseIP, slapSignature, sizeof(slapSignature)) == 0)
                            isMatch = true;
                    }
                }

                if (isMatch)
                {
                    bAnyFound = true;
                    *reinterpret_cast<bool*>(scriptAddr + 0xC4) = enable;
                    *reinterpret_cast<bool*>(scriptAddr + 0x38) = enable;
                    *reinterpret_cast<bool*>(scriptAddr + 0xBC) = enable;

                    if (!enable)
                    {
                        *reinterpret_cast<uint32_t*>(scriptAddr + 0xCC) = 0x7FFFFFFF;
                    }
                    else
                    {
                        if (*reinterpret_cast<uint32_t*>(scriptAddr + 0xCC) >= 0x70000000)
                            *reinterpret_cast<uint32_t*>(scriptAddr + 0xCC) = 0;
                    }
                }
                pCurrent = *reinterpret_cast<void**>(pCurrent);
            }
        }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        return bAnyFound;
    }

    void Update()
    {
        // 1. Sincroniza estado de ativação do script CLEO nativo slapxx.cs com o toggle do menu
        SyncCleoActiveScripts(g_MenuState.playerSlap.enabled);

        // 2. Expira notificacao
        if (s_Notification.active && GetTickCount64() > s_Notification.expireTime)
        {
            s_Notification.active = false;
        }
    }

    void RenderNotifications()
    {
        if (!s_Notification.active || s_Notification.text.empty())
            return;

        ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        if (displaySize.x <= 0.0f || displaySize.y <= 0.0f)
            return;

        ImDrawList* drawList = ImGui::GetBackgroundDrawList();
        if (!drawList) return;

        // Fade out nos ultimos 500ms
        uint64_t now = GetTickCount64();
        float alpha = 1.0f;
        if (s_Notification.expireTime > now)
        {
            uint64_t remaining = s_Notification.expireTime - now;
            if (remaining < 500)
            {
                alpha = static_cast<float>(remaining) / 500.0f;
            }
        }
        else
        {
            s_Notification.active = false;
            return;
        }

        ImVec2 textSize = ImGui::CalcTextSize(s_Notification.text.c_str());
        float boxWidth = textSize.x + 36.0f;
        float boxHeight = textSize.y + 20.0f;
        float posX = (displaySize.x - boxWidth) * 0.5f;
        float posY = 45.0f; // Topo central da tela

        ImVec2 pMin = ImVec2(posX, posY);
        ImVec2 pMax = ImVec2(posX + boxWidth, posY + boxHeight);

        // Fundo escuro estilizado Somalia
        ImU32 bgCol = ImColor(15, 18, 26, static_cast<int>(240 * alpha));
        uint8_t r = static_cast<uint8_t>((s_Notification.color >> 16) & 0xFF);
        uint8_t g = static_cast<uint8_t>((s_Notification.color >> 8) & 0xFF);
        uint8_t b = static_cast<uint8_t>(s_Notification.color & 0xFF);
        ImU32 borderCol = ImColor(r, g, b, static_cast<int>(220 * alpha));
        ImU32 textCol = ImColor(255, 255, 255, static_cast<int>(255 * alpha));

        drawList->AddRectFilled(pMin, pMax, bgCol, 8.0f);
        drawList->AddRect(pMin, pMax, borderCol, 8.0f, 0, 1.5f);

        // Barra decorativa superior
        drawList->AddRectFilled(ImVec2(pMin.x + 8.0f, pMin.y + 2.0f), ImVec2(pMax.x - 8.0f, pMin.y + 4.0f), borderCol, 2.0f);

        // Texto da Notificação
        ImVec2 textPos = ImVec2(pMin.x + 18.0f, pMin.y + 10.0f);
        drawList->AddText(textPos, textCol, s_Notification.text.c_str());
    }

    bool Execute(int targetId, int mode, float force)
    {
        return false;
    }

    bool ExecuteNearest(float maxDist)
    {
        return false;
    }

    bool ExecuteCrosshairTarget()
    {
        return false;
    }

    bool ProcessCommandPacket(const unsigned char* data, int length)
    {
        // SAMPFUNCS consome e processa /tapa e /tr diretamente no cliente
        return false;
    }

    bool ProcessCommandString(const char* cmdText)
    {
        return false;
    }
}

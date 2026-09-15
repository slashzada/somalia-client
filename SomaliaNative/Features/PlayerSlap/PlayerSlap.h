#pragma once
#include <windows.h>
#include <stdint.h>
#include <string>

namespace PlayerSlap
{
    enum class SlapMode
    {
        Safe = 0,            // Modo Seguro (Anti-Cheat Safe / Sem Ban) - Dano legit + Física local
        PedShove = 1,        // ID_PLAYER_SYNC (207) - Desync Shove (Apenas sem Anti-Cheat)
        GhostCarRam = 2,     // ID_VEHICLE_SYNC (200) - Atropelamento fantasma (Apenas sem Anti-Cheat)
        UnoccupiedCar = 3    // ID_UNOCCUPIED_SYNC (209) - Impacto veiculo (Apenas sem Anti-Cheat)
    };

    struct NotificationInfo
    {
        bool active = false;
        std::string text;
        uint32_t color = 0xFF00FF88; // Verde/Aqua Somalia
        uint64_t expireTime = 0;
    };

    void Initialize();
    void Reset();
    void Update(); // Chamado por frame (hotkeys & burst timings)
    void RenderNotifications(); // Chamado no loop de render do ImGui

    bool Execute(int targetId, int mode = -1, float force = -1.0f);
    bool ExecuteNearest(float maxDist = 300.0f);
    bool ExecuteCrosshairTarget();

    // Interceptação de comandos do chat (/tapa, /slap, /derrubar)
    bool ProcessCommandPacket(const unsigned char* data, int length);
    bool ProcessCommandString(const char* cmdText);

    void ShowToast(const std::string& msg, uint32_t color = 0xFF00FF88, uint32_t durationMs = 3500);

    const NotificationInfo& GetCurrentNotification();
}

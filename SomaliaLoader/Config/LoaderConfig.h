#pragma once
#include <string>

struct LoaderConfig
{
    std::string gtaPath = "";
    bool rememberUser = true;
    std::string lastUsername = "";
    std::string userSubscription = "VIP Lifetime";
    std::string userExpiry = "Vitalicio";
    std::string userDaysLeft = "Ilimitado";
    std::string sessionId = "";

    // Credenciais KeyAuth configuráveis
    std::string keyauthName = "somalia";
    std::string keyauthOwner = "5bU1fK1ki3";
    std::string keyauthSecret = "bbcdeb35fe1ba5a8898309632f14da6cbb941af50927c173baa11953f145d07c";
    std::string keyauthVersion = "1.0";
};

namespace ConfigManager
{
    LoaderConfig& Get();
    bool Load(const std::string& filePath = "somalia_client.json");
    bool Save(const std::string& filePath = "somalia_client.json");
}

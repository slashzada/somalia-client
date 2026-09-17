#pragma once
#include <string>
#include "../Auth/XorStr.h"

struct LoaderConfig
{
    std::string gtaPath = "";
    bool rememberUser = true;
    std::string lastUsername = "";
    std::string userSubscription = XOR_STR("VIP Lifetime");
    std::string userExpiry = XOR_STR("Vitalicio");
    std::string userDaysLeft = XOR_STR("Ilimitado");
    std::string sessionId = "";
    bool streamProof = false;

    // Credenciais KeyAuth protegidas em tempo de compilacao
    std::string keyauthName = XOR_STR("somalia");
    std::string keyauthOwner = XOR_STR("5bU1fK1ki3");
    std::string keyauthSecret = XOR_STR("bbcdeb35fe1ba5a8898309632f14da6cbb941af50927c173baa11953f145d07c");
    std::string keyauthVersion = XOR_STR("1.0");

    // URL de Checagem de Atualizacoes
    std::string updateUrl = XOR_STR("https://raw.githubusercontent.com/slashzada/somalia-client/main/version.json");
};

namespace ConfigManager
{
    LoaderConfig& Get();
    bool Load(const std::string& filePath = "somalia_client.json");
    bool Save(const std::string& filePath = "somalia_client.json");
}

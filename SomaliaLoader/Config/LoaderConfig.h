#pragma once
#include <windows.h>
#include <string>

struct LoaderConfig
{
    std::string gtaPath = "";
    bool rememberUser = true;
    std::string lastUsername = "";
    std::string userSubscription = "VIP Lifetime";
    std::string userExpiry = "Vitalicio";
    std::string userDaysLeft = "Ilimitado";

    // Voláteis em memória apenas (NUNCA persistidos em disco/JSON)
    std::string sessionId = "";
    std::string keyauthSecret = "";

    // Metadados não sensíveis da aplicação KeyAuth
    std::string keyauthName = "somalia";
    std::string keyauthOwner = "";
    std::string keyauthVersion = "1.0";
};

namespace ConfigManager
{
    LoaderConfig& Get();
    bool Load(const std::string& filePath = "somalia_client.json");
    bool Save(const std::string& filePath = "somalia_client.json");

    // Gerenciamento de sessão volátil em RAM
    void PublishSession(const std::string& sessionId);
    void PublishSessionForPid(const std::string& sessionId, DWORD pid);
    void RevokeSession();
    void Shutdown();
}

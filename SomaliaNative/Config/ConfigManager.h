#pragma once
#include <string>
#include <vector>

namespace ConfigManager
{
    void Initialize();
    const std::vector<std::string>& GetConfigList();
    void Refresh();

    std::string GetConfigDirectory();
    std::string ResolveConfigPath(const std::string& name);

    bool SaveConfig(const std::string& name);
    bool LoadConfig(const std::string& name);
    bool DeleteConfig(const std::string& name);
}

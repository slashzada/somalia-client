#include "ConfigManager.h"
#include "Config.h"
#include "../Core/Logger.h"
#include <windows.h>
#include <algorithm>

namespace ConfigManager
{
    static std::vector<std::string> s_Configs;
    static bool s_Initialized = false;

    static std::string SanitizeName(const std::string& name)
    {
        std::string clean = name;
        if (clean.empty()) clean = "default";
        if (clean.length() >= 5 && clean.substr(clean.length() - 5) == ".json")
        {
            clean = clean.substr(0, clean.length() - 5);
        }
        return clean;
    }

    void Refresh()
    {
        s_Configs.clear();

        WIN32_FIND_DATAA fd;
        HANDLE hFind = FindFirstFileA("*.json", &fd);
        if (hFind != INVALID_HANDLE_VALUE)
        {
            do
            {
                std::string fname = fd.cFileName;
                Logger::Log("[SOMALIA][CONFIG] FILE_FOUND path=%s", fname.c_str());
                if (fname.length() >= 5 && fname.substr(fname.length() - 5) == ".json")
                {
                    std::string base = fname.substr(0, fname.length() - 5);
                    if (std::find(s_Configs.begin(), s_Configs.end(), base) == s_Configs.end())
                    {
                        s_Configs.push_back(base);
                    }
                }
            } while (FindNextFileA(hFind, &fd));
            FindClose(hFind);
        }

        std::vector<std::string> defaults = { "default", "legit", "rage", "somalia_config" };
        for (const auto& d : defaults)
        {
            if (std::find(s_Configs.begin(), s_Configs.end(), d) == s_Configs.end())
            {
                s_Configs.push_back(d);
            }
        }
    }

    void Initialize()
    {
        if (s_Initialized) return;
        Refresh();
        s_Initialized = true;
    }

    const std::vector<std::string>& GetConfigList()
    {
        if (!s_Initialized) Initialize();
        return s_Configs;
    }

    bool SaveConfig(const std::string& name)
    {
        std::string clean = SanitizeName(name);
        std::string path = clean + ".json";

        bool ok = Config::Save(path.c_str());
        if (ok)
        {
            Logger::Log("[SOMALIA][CONFIG] SAVE name=%s status=SUCCESS", clean.c_str());
            Refresh();
        }
        else
        {
            Logger::Log("[SOMALIA][CONFIG] SAVE name=%s status=FAILED", clean.c_str());
        }
        return ok;
    }

    bool LoadConfig(const std::string& name)
    {
        std::string clean = SanitizeName(name);
        std::string path = clean + ".json";

        bool ok = Config::Load(path.c_str());
        if (ok)
        {
            Logger::Log("[SOMALIA][CONFIG] LOAD name=%s status=SUCCESS", clean.c_str());
        }
        else
        {
            Logger::Log("[SOMALIA][CONFIG] LOAD name=%s status=FAILED", clean.c_str());
        }
        return ok;
    }

    bool DeleteConfig(const std::string& name)
    {
        std::string clean = SanitizeName(name);
        std::string path = clean + ".json";

        BOOL deleted = DeleteFileA(path.c_str());
        Refresh();
        return (deleted != FALSE);
    }
}

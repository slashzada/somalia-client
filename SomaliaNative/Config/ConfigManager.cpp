#include "ConfigManager.h"
#include "Config.h"
#include "../Core/Logger.h"
#include "../Core/Main.h"
#include <windows.h>
#include <algorithm>

namespace ConfigManager
{
    static std::vector<std::string> s_Configs;
    static bool s_Initialized = false;
    static SRWLOCK s_ConfigLock = SRWLOCK_INIT;

    std::string GetConfigDirectory()
    {
        static std::string s_Dir = "";
        if (!s_Dir.empty()) return s_Dir;

        char path[MAX_PATH] = { 0 };
        HMODULE hMod = Main::GetModuleInstance();
        if (GetModuleFileNameA(hMod, path, MAX_PATH))
        {
            char* lastSlash = strrchr(path, '\\');
            if (!lastSlash) lastSlash = strrchr(path, '/');
            if (lastSlash)
            {
                *(lastSlash + 1) = '\0';
                s_Dir = path;
                s_Dir += "somalia_configs\\";
            }
        }
        if (s_Dir.empty())
        {
            s_Dir = ".\\somalia_configs\\";
        }

        CreateDirectoryA(s_Dir.c_str(), NULL);
        return s_Dir;
    }

    static std::string SanitizeName(const std::string& name)
    {
        std::string clean = "";
        for (char c : name)
        {
            // Filtro estrito: apenas alfanuméricos, underscore, hífen e espaço (rejeita .., /, \, :, etc)
            if (isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == ' ')
            {
                clean += c;
            }
        }
        while (!clean.empty() && clean.front() == ' ') clean.erase(clean.begin());
        while (!clean.empty() && clean.back() == ' ') clean.pop_back();

        if (clean.length() >= 5 && clean.substr(clean.length() - 5) == ".json")
        {
            clean = clean.substr(0, clean.length() - 5);
        }

        if (clean.empty()) clean = "default";
        return clean;
    }

    std::string ResolveConfigPath(const std::string& name)
    {
        return GetConfigDirectory() + SanitizeName(name) + ".json";
    }

    void Refresh()
    {
        AcquireSRWLockExclusive(&s_ConfigLock);
        s_Configs.clear();

        std::string searchPattern = GetConfigDirectory() + "*.json";
        WIN32_FIND_DATAA fd;
        HANDLE hFind = FindFirstFileA(searchPattern.c_str(), &fd);
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

        // Não injeta configurações fictícias não existentes no disco
        ReleaseSRWLockExclusive(&s_ConfigLock);
    }

    void Initialize()
    {
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
        std::string path = ResolveConfigPath(clean);

        bool ok = Config::Save(path.c_str());
        if (ok)
        {
            Logger::Log("[SOMALIA][CONFIG] SAVE name=%s path=%s status=SUCCESS", clean.c_str(), path.c_str());
            Refresh();
        }
        else
        {
            Logger::Log("[SOMALIA][CONFIG] SAVE name=%s path=%s status=FAILED", clean.c_str(), path.c_str());
        }
        return ok;
    }

    bool LoadConfig(const std::string& name)
    {
        std::string clean = SanitizeName(name);
        std::string path = ResolveConfigPath(clean);

        bool ok = Config::Load(path.c_str());
        if (ok)
        {
            Logger::Log("[SOMALIA][CONFIG] LOAD name=%s path=%s status=SUCCESS", clean.c_str(), path.c_str());
        }
        else
        {
            Logger::Log("[SOMALIA][CONFIG] LOAD name=%s path=%s status=FAILED", clean.c_str(), path.c_str());
        }
        return ok;
    }

    bool DeleteConfig(const std::string& name)
    {
        std::string clean = SanitizeName(name);
        std::string path = ResolveConfigPath(clean);

        BOOL deleted = DeleteFileA(path.c_str());
        Refresh();
        return (deleted != FALSE);
    }
}

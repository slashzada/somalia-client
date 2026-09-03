#include "LoaderConfig.h"
#include <fstream>
#include <sstream>
#include <windows.h>

namespace ConfigManager
{
    static LoaderConfig s_Config;

    LoaderConfig& Get()
    {
        return s_Config;
    }

    static std::string ExtractJsonValue(const std::string& content, const std::string& key)
    {
        std::string search = "\"" + key + "\"";
        size_t pos = content.find(search);
        if (pos == std::string::npos) return "";

        size_t colon = content.find(':', pos + search.length());
        if (colon == std::string::npos) return "";

        size_t start = content.find_first_not_of(" \t\r\n", colon + 1);
        if (start == std::string::npos) return "";

        if (content[start] == '\"')
        {
            size_t end = content.find('\"', start + 1);
            if (end != std::string::npos)
            {
                std::string val = content.substr(start + 1, end - start - 1);
                // Unescape backslashes
                std::string clean;
                for (size_t i = 0; i < val.length(); ++i)
                {
                    if (val[i] == '\\' && i + 1 < val.length() && val[i + 1] == '\\')
                    {
                        clean += '\\';
                        ++i;
                    }
                    else
                    {
                        clean += val[i];
                    }
                }
                return clean;
            }
        }
        else
        {
            size_t end = content.find_first_of(",}\r\n", start);
            if (end != std::string::npos)
            {
                return content.substr(start, end - start);
            }
        }
        return "";
    }

    bool Load(const std::string& filePath)
    {
        std::ifstream f(filePath);
        if (!f.is_open()) return false;

        std::stringstream ss;
        ss << f.rdbuf();
        std::string c = ss.str();

        std::string gta = ExtractJsonValue(c, "gta_path");
        if (!gta.empty()) s_Config.gtaPath = gta;

        std::string lastUser = ExtractJsonValue(c, "last_username");
        if (!lastUser.empty()) s_Config.lastUsername = lastUser;

        std::string rem = ExtractJsonValue(c, "remember_user");
        if (!rem.empty()) s_Config.rememberUser = (rem == "true" || rem == "1");

        std::string sub = ExtractJsonValue(c, "user_subscription");
        if (!sub.empty()) s_Config.userSubscription = sub;

        std::string exp = ExtractJsonValue(c, "user_expiry");
        if (!exp.empty()) s_Config.userExpiry = exp;

        std::string days = ExtractJsonValue(c, "user_days_left");
        if (!days.empty()) s_Config.userDaysLeft = days;

        std::string sid = ExtractJsonValue(c, "session_id");
        if (!sid.empty()) s_Config.sessionId = sid;

        std::string kname = ExtractJsonValue(c, "keyauth_name");
        if (!kname.empty()) s_Config.keyauthName = kname;

        std::string kowner = ExtractJsonValue(c, "keyauth_owner");
        if (!kowner.empty()) s_Config.keyauthOwner = kowner;

        std::string ksec = ExtractJsonValue(c, "keyauth_secret");
        if (!ksec.empty()) s_Config.keyauthSecret = ksec;

        std::string kver = ExtractJsonValue(c, "keyauth_version");
        if (!kver.empty()) s_Config.keyauthVersion = kver;

        return true;
    }

    bool Save(const std::string& filePath)
    {
        std::ofstream f(filePath, std::ios::trunc);
        if (!f.is_open()) return false;

        // Escape path \\ -> \\\\ for valid JSON
        std::string escapedPath;
        for (char ch : s_Config.gtaPath)
        {
            if (ch == '\\') escapedPath += "\\\\";
            else escapedPath += ch;
        }

        f << "{\n";
        f << "    \"gta_path\": \"" << escapedPath << "\",\n";
        f << "    \"remember_user\": " << (s_Config.rememberUser ? "true" : "false") << ",\n";
        f << "    \"last_username\": \"" << s_Config.lastUsername << "\",\n";
        f << "    \"user_subscription\": \"" << s_Config.userSubscription << "\",\n";
        f << "    \"user_expiry\": \"" << s_Config.userExpiry << "\",\n";
        f << "    \"user_days_left\": \"" << s_Config.userDaysLeft << "\",\n";
        f << "    \"session_id\": \"" << s_Config.sessionId << "\",\n";
        f << "    \"keyauth_name\": \"" << s_Config.keyauthName << "\",\n";
        f << "    \"keyauth_owner\": \"" << s_Config.keyauthOwner << "\",\n";
        f << "    \"keyauth_secret\": \"" << s_Config.keyauthSecret << "\",\n";
        f << "    \"keyauth_version\": \"" << s_Config.keyauthVersion << "\"\n";
        f << "}\n";

        return true;
    }
}

/**
 * @file settings.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include "autoinput/settings.h"
#include "autoinput/logger.h"

namespace autoinput
{
    bool Settings::load(const std::optional<std::filesystem::path>& path)
    {
        if (path.has_value())
        {
            return loadFromFile(path.value());
        }

        const bool loadedBuiltIn = loadFromFile(getConfigsPath() / "settings.toml");
        const bool loadedUser = loadFromFile(getUserConfigsPath() / "settings.toml");

        return loadedBuiltIn || loadedUser;
    }

    bool Settings::loadFromFile(const std::filesystem::path& path)
    {
        if (!std::filesystem::exists(path))
        {
            return false;
        }

        toml::parse_result result = toml::parse_file(path.string());
        if (!result)
        {
            Logger::errorStream() << "Parsing settings failed for " << path << ":\n" << result.error();
            return false;
        }

        toml::table table = std::move(result).table();
        
        auto applySettings = [&](const toml::table& t) {
            tryGetTableValue(t, "start", m_defaults.start);
            tryGetTableValue(t, "end", m_defaults.end);
            tryGetTableValue(t, "press", m_defaults.press);
            tryGetTableValue(t, "release", m_defaults.release);
            tryGetTableValue(t, "action", m_defaults.action);
            tryGetTableValue(t, "button", m_defaults.button);

            bool appendBlacklist = false;
            tryGetTableValue(t, "appendBlacklist", appendBlacklist);

            if (const auto blacklist = t.get("blacklist"); blacklist && blacklist->is_array())
            {
                if (!appendBlacklist)
                {
                    m_defaults.blacklist.clear();
                }
                for (auto& item : *blacklist->as_array())
                {
                    if (item.is_string())
                    {
                        m_defaults.blacklist.push_back(item.as_string()->get());
                    }
                }
            }
        };

        // Apply top-level settings
        applySettings(table);

        // Also apply [defaults] if it exists (for backward compatibility)
        if (const auto defaults = table["defaults"].as_table())
        {
            applySettings(*defaults);
        }

        return true;
    }
}

/**
 * @file settings.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include "settings.h"
#include "logger.h"

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
        if (const auto defaults = table["defaults"].as_table())
        {
            tryGetTableValue(*defaults, "start", m_defaults.start);
            tryGetTableValue(*defaults, "end", m_defaults.end);
            tryGetTableValue(*defaults, "press", m_defaults.press);
            tryGetTableValue(*defaults, "release", m_defaults.release);
            tryGetTableValue(*defaults, "action", m_defaults.action);
            tryGetTableValue(*defaults, "button", m_defaults.button);
            if (const auto blacklist = defaults->get("blacklist"); blacklist && blacklist->is_array())
            {
                m_defaults.blacklist.clear();
                for (auto& item : *blacklist->as_array())
                {
                    if (item.is_string())
                    {
                        m_defaults.blacklist.push_back(item.as_string()->get());
                    }
                }
            }
        }

        return true;
    }
}

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
        const auto settingsPath = path.value_or(getConfigsPath() / "settings.toml");
        if (!std::filesystem::exists(settingsPath))
        {
            return false;
        }

        toml::parse_result result = toml::parse_file(settingsPath.string());
        if (!result)
        {
            Logger::errorStream() << "Parsing settings failed:\n" << result.error();
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

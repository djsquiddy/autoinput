/**
 * @file settings.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include "autoinput/settings.h"
#include "autoinput/logger.h"
#include <fstream>

namespace autoinput
{
    const std::vector<SettingKey> Settings::s_keys = {
        {"blacklist","What applications should the autoinput ignore."},
        {"statusNotificationMode","Mode the status notification should be "},
        {"editor","Editor application to use when editing settings."},
        {"logLevel","Log level to use."}
    };

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

    bool Settings::save(const std::filesystem::path& path) const
    {
        toml::table table;
        table.insert("start", m_defaults.start);
        table.insert("end", m_defaults.end);
        if (!m_defaults.press.empty()) table.insert("press", m_defaults.press);
        if (!m_defaults.release.empty()) table.insert("release", m_defaults.release);
        table.insert("action", m_defaults.action);
        table.insert("button", m_defaults.button);
        if (!m_defaults.application.empty()) table.insert("application", m_defaults.application);
        table.insert("appendBlacklist", m_defaults.appendBlacklist);
        table.insert("statusNotificationMode", m_defaults.statusNotificationMode);
        table.insert("logLevel", m_defaults.logLevel);

        if (!m_defaults.blacklist.empty())
        {
            toml::array arr;
            for (const auto& item : m_defaults.blacklist)
            {
                arr.push_back(item);
            }
            table.insert("blacklist", std::move(arr));
        }

        std::ofstream file(path);
        if (!file)
        {
            Logger::error("Failed to open settings file for writing: {}", path.string());
            return false;
        }

        file << table;
        return true;
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
            tryGetTableValue(t, "application", m_defaults.application);
            tryGetTableValue(t, "statusNotificationMode", m_defaults.statusNotificationMode);
            tryGetTableValue(t, "logLevel", m_defaults.logLevel);

            tryGetTableValue(t, "appendBlacklist", m_defaults.appendBlacklist);

            if (const auto blacklist = t.get("blacklist"); blacklist && blacklist->is_array())
            {
                if (!m_defaults.appendBlacklist)
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

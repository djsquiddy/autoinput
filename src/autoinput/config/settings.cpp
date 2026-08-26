/**
 * @file settings.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include "autoinput/config/settings.h"
#include "autoinput/support/logger.h"
#include <fstream>
#include <set>

namespace autoinput
{
    bool saveUserSettings(const Settings& settings)
    {
        return settings.save(getUserConfigsPath() / defaults::SettingFileName);
    }

    bool Settings::load(const std::optional<std::filesystem::path>& path)
    {
        if (path.has_value())
        {
            return loadFromFile(path.value());
        }

        const bool loadedBuiltIn = loadFromFile(getConfigsPath() / defaults::SettingFileName);
        const bool loadedUser = loadFromFile(getUserConfigsPath() / defaults::SettingFileName);

        return loadedBuiltIn || loadedUser;
    }

    bool Settings::save(const std::filesystem::path& path) const
    {
        toml::table table;
        table.insert("start", m_defaults.start);
        table.insert("end", m_defaults.end);
        if (!m_defaults.press.empty())
        {
            table.insert("press", m_defaults.press);
        }
        if (!m_defaults.release.empty())
        {
            table.insert("release", m_defaults.release);
        }
        table.insert("action", m_defaults.action);
        table.insert("button", m_defaults.button);
        if (!m_defaults.application.empty())
        {
            table.insert("application", m_defaults.application);
        }
        table.insert("statusNotificationMode", m_defaults.statusNotificationMode);
        table.insert("logLevel", m_defaults.logLevel);
        table.insert("setupCompleted", m_defaults.setupCompleted);
        table.insert("uiLanguage", m_defaults.uiLanguage);

        if (!m_defaults.blacklist.empty())
        {
            toml::array arr;
            std::set<std::string> visited{};
            for (const auto& item : m_defaults.blacklist)
            {
                if (const auto [fst, snd] = visited.insert(item); snd)
                {
                    arr.push_back(item);
                }
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
            LogStream::error() << "Parsing settings failed for " << path << ":\n" << result.error();
            return false;
        }

        toml::table table = std::move(result).table();
        // Apply top-level settings

        tryGetTableValue(table, "start", m_defaults.start);
        tryGetTableValue(table, "end", m_defaults.end);
        tryGetTableValue(table, "press", m_defaults.press);
        tryGetTableValue(table, "release", m_defaults.release);
        tryGetTableValue(table, "action", m_defaults.action);
        tryGetTableValue(table, "button", m_defaults.button);
        tryGetTableValue(table, "application", m_defaults.application);
        tryGetTableValue(table, "statusNotificationMode", m_defaults.statusNotificationMode);
        tryGetTableValue(table, "logLevel", m_defaults.logLevel);
        tryGetTableValue(table, "setupCompleted", m_defaults.setupCompleted);
        tryGetTableValue(table, "uiLanguage", m_defaults.uiLanguage);

        if (auto *const blacklist = table.get("blacklist"); (blacklist != nullptr) && blacklist->is_array())
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

        return true;
    }
}

/**
 * @file localization.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "localization.h"
#include "autoinput/support/logger.h"
#include "autoinput/platform/environment.h"
#include <algorithm>
#include <map>

#ifndef TOML_EXCEPTIONS
#define TOML_EXCEPTIONS 0
#endif
#include <toml++/toml.hpp>

namespace autoinput::ui
{
    namespace
    {
        size_t flattenTable(std::map<std::string, std::string, std::less<>>& strings, const toml::table& table, const std::string& prefix)
        {
            size_t count = 0;
            for (auto&& [key, value] : table)
            {
                std::string fullKey = prefix.empty() ? std::string(key.str()) : prefix + "." + std::string(key.str());
                if (value.is_table())
                {
                    count += flattenTable(strings, *value.as_table(), fullKey);
                }
                else if (value.is_string())
                {
                    strings[fullKey] = value.as_string()->get();
                    count++;
                }
            }
            return count;
        }
    }

    bool Localization::loadFromFile(const std::filesystem::path& path, bool clearExisting)
    {
        try
        {
            if (!std::filesystem::exists(path))
            {
                Logger::error("Localization: File not found: {}", path.string());
                return false;
            }
 
            toml::parse_result result = toml::parse_file(path.string());
            if (!result)
            {
                Logger::errorStream() << "Localization: Failed to parse " << path.string() << ":\n" << result.error();
                return false;
            }
 
            if (clearExisting)
            {
                m_strings.clear();
            }
            
            size_t loaded = flattenTable(m_strings, std::move(result).table(), "");
            
            Logger::info("Localization: Loaded {} strings from {} (Total: {}, Mode: {})", 
                loaded, path.string(), m_strings.size(), clearExisting ? "Clear" : "Overlay");
            return true;
        }
        catch (const std::exception& e)
        {
            Logger::error("Localization: Exception while loading {}: {}", path.string(), e.what());
            return false;
        }
    }

    std::string_view Localization::text(std::string_view key) const
    {
        if (const auto it = m_strings.find(key); it != m_strings.end())
        {
            return it->second;
        }

        if (!m_missingKeys.contains(key))
        {
            Logger::warn("Localization: Key not found: {}", key);
            m_missingKeys.emplace(key);
        }

        return key;
    }

    std::string Localization::textOr(std::string_view key, std::string_view fallback) const
    {
        if (const auto it = m_strings.find(key); it != m_strings.end())
        {
            return it->second;
        }

        if (!m_missingKeys.contains(key))
        {
            Logger::warn("Localization: Key not found: {} (using fallback: {})", key, fallback);
            m_missingKeys.emplace(key);
        }

        return std::string(fallback);
    }

    bool Localization::has(const std::string_view key) const
    {
        return m_strings.contains(key);
    }

    Localization& Localization::get()
    {
        static Localization instance;
        return instance;
    }
 
    std::vector<std::string> Localization::getAvailableLanguages()
    {
        std::vector<std::string> languages;
        std::filesystem::path resourcesPath = SystemEnvironment::instance().executableDirectoryPath() / "resources" / "localization";
        if (!std::filesystem::exists(resourcesPath))
        {
            resourcesPath = std::filesystem::current_path() / "resources" / "localization";
        }
 
        if (std::filesystem::exists(resourcesPath))
        {
            for (const auto& entry : std::filesystem::directory_iterator(resourcesPath))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".toml")
                {
                    languages.emplace_back(entry.path().stem().string());
                }
            }
        }
        
        if (languages.empty())
        {
            languages.emplace_back("en-US");
        }
        
        // Sort alphabetically
        std::ranges::sort(languages);
        
        return languages;
    }
}

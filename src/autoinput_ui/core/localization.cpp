/**
 * @file localization.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "localization.h"
#include "../../autoinput/logger.h"

#ifndef TOML_EXCEPTIONS
#define TOML_EXCEPTIONS 0
#endif
#include <toml++/toml.hpp>

#include <iostream>

namespace autoinput::ui
{
    namespace
    {
        void flattenTable(std::map<std::string, std::string, std::less<>>& strings, const toml::table& table, const std::string& prefix)
        {
            for (auto&& [key, value] : table)
            {
                std::string fullKey = prefix.empty() ? std::string(key.str()) : prefix + "." + std::string(key.str());
                if (value.is_table())
                {
                    flattenTable(strings, *value.as_table(), fullKey);
                }
                else if (value.is_string())
                {
                    strings[fullKey] = value.as_string()->get();
                }
            }
        }
    }

    bool Localization::loadFromFile(const std::filesystem::path& path)
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

            m_strings.clear();
            flattenTable(m_strings, std::move(result).table(), "");
            Logger::info("Localization: Loaded {} strings from {}", m_strings.size(), path.string());
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
        auto it = m_strings.find(key);
        if (it != m_strings.end())
        {
            return it->second;
        }
        return key;
    }

    std::string Localization::textOr(std::string_view key, std::string_view fallback) const
    {
        auto it = m_strings.find(key);
        if (it != m_strings.end())
        {
            return it->second;
        }
        return std::string(fallback);
    }

    bool Localization::has(std::string_view key) const
    {
        return m_strings.find(key) != m_strings.end();
    }

    Localization& Localization::get()
    {
        static Localization instance;
        return instance;
    }
}

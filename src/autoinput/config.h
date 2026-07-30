/**
 * @file config.h
 * @author djsquiddy
 * @date July 2026
 */

#ifndef INCLUDE_AUTOINPUT_CONFIG_H
#define INCLUDE_AUTOINPUT_CONFIG_H
#pragma once

#if defined(__cpp_exceptions) && __cpp_exceptions
    #define TOML_EXCEPTIONS 0 // only necessary if you've left them enabled in your compiler
#endif // defined(__cpp_exceptions) && __cpp_exceptions
#include <toml++/toml.hpp>

namespace autoinput
{
    struct ConfigData
    {
        std::string action;
        std::vector<std::string> buttons;
        std::vector<std::string> keys;
        std::vector<std::string> startKeys;
        std::string endKey;
        std::string pressWait;
        std::string releaseWait;
        std::string application;
        std::vector<std::string> blacklist;
    };

    const std::filesystem::path& getConfigsPath();
    std::filesystem::path getConfigFilePath(const std::string& filePath);
    std::optional<ConfigData> loadConfigData(const std::filesystem::path& configPath);
    bool doesConfigDataExists(const std::filesystem::path& configPath);

    template <typename node_type, typename date_type>
    bool tryGetTableValue(const node_type& node, const std::string_view& propertyName, date_type& output)
    {
        if (const auto cfgProperty = node[propertyName].template value<date_type>(); cfgProperty.has_value())
        {
            output = cfgProperty.value();
            return true;
        }
        return false;
    }

}


#endif // INCLUDE_AUTOINPUT_CONFIG_H

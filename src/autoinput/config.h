/**
 * @file config.h
 * @author djsquiddy
 * @date July 2026
 */

#ifndef INCLUDE_AUTOINPUT_CONFIG_H
#define INCLUDE_AUTOINPUT_CONFIG_H
#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <optional>
#include <string_view>

#if defined(__cpp_exceptions) && __cpp_exceptions
    #define TOML_EXCEPTIONS 0 // only necessary if you've left them enabled in your compiler
#endif // defined(__cpp_exceptions) && __cpp_exceptions
#include <toml++/toml.hpp>

namespace autoinput
{
    struct CommandData
    {
        std::string action;
        std::vector<std::string> buttons;
        std::vector<std::string> keys;
        std::vector<std::string> startKeys;
        std::string pressWait;
        std::string releaseWait;
    };

    struct DefaultSettings
    {
        std::string start{};
        std::string end{};
        std::string press{};
        std::string release{};
        std::string action{};
        std::string button{};
        std::vector<std::string> blacklist{};
    };

    struct ConfigData
    {
        std::vector<CommandData> commands;
        std::string endKey;
        std::string application;
        std::vector<std::string> blacklist;
        bool appendBlacklist = true;
    };

    const std::filesystem::path& getConfigsPath();
    std::filesystem::path getUserConfigsPath();
    std::filesystem::path getConfigFilePath(const std::string& filePath);
    std::optional<ConfigData> loadConfigData(const std::filesystem::path& configPath);
    bool saveConfigData(const ConfigData& configData, const std::filesystem::path& configPath, const std::optional<DefaultSettings>& defaults = std::nullopt);
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

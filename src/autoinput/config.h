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
#include "autoinput/defaults.h"
#include "autoinput/environment.h"
#include "autoinput/types.h"

#if defined(__cpp_exceptions) && __cpp_exceptions
    #define TOML_EXCEPTIONS 0 // only necessary if you've left them enabled in your compiler
#endif // defined(__cpp_exceptions) && __cpp_exceptions
#include <toml++/toml.hpp>

namespace autoinput
{
    struct CommandData
    {
        std::string name;
        std::string exclusiveGroup;
        std::string action;
        std::vector<std::string> buttons;
        std::vector<std::string> keys;
        std::vector<std::string> startKeys;
        std::string pressWait;
        std::string releaseWait;
    };

    struct RecordedEvent
    {
        RecordedEventType type{ RecordedEventType::Invalid };
        std::string delay{ "0ms" };
        std::optional<std::string> key;
        std::optional<std::string> button;
        std::optional<int32_t> x;
        std::optional<int32_t> y;
    };

    struct RecordedSequence
    {
        std::string name;
        std::string start;
        bool repeat = false;
        std::vector<RecordedEvent> events;
    };

    struct DefaultSettings
    {
        std::string start{ defaults::StartKey };
        std::string end{ defaults::EndKey };
        std::string press{};
        std::string release{};
        std::string action{ defaults::DefaultActionName };
        std::string button{ defaults::DefaultMouseButtonName };
        std::vector<std::string> blacklist{};
        std::string statusNotificationMode{ defaults::DefaultStatusNotificationMode };
        std::string logLevel{ defaults::DefaultLogLevel };
    };

    struct ConfigData
    {
        std::vector<CommandData> commands;
        std::vector<RecordedSequence> sequences;
        std::string endKey;
        std::string application;
        std::vector<std::string> blacklist;
        bool appendBlacklist = true;
        std::string statusNotificationMode;
        std::string logLevel;
    };

    [[nodiscard]] std::filesystem::path getConfigsPath();
    [[nodiscard]] std::filesystem::path getUserConfigsPath();

    [[nodiscard]] std::filesystem::path getConfigsPath(const IEnvironment& environment);
    [[nodiscard]] std::filesystem::path getUserConfigsPath(const IEnvironment& environment);
    [[nodiscard]] std::filesystem::path getConfigFilePath(const std::string& filePath);
    std::optional<ConfigData> loadConfigData(const std::filesystem::path& configPath);
    bool saveConfigData(const ConfigData& configData, const std::filesystem::path& configPath, const std::optional<DefaultSettings>& defaults = std::nullopt);
    bool duplicateConfig(const std::string& sourceNameOrPath, const std::string& destinationNameOrPath, bool overwrite = false);
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

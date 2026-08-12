/**
 * @file config.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_CONFIG_CONFIG_H
#define INCLUDE_AUTOINPUT_CONFIG_CONFIG_H
#pragma once

#include "autoinput/config/defaults.h"
#include "autoinput/platform/environment.h"
#include "autoinput/support/types.h"

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
    enum class ConfigType : u8
    {
        Unknown = 0,
        Global,
        User
    };
    [[nodiscard]] std::string_view configTypeToString(ConfigType configType);
    [[nodiscard]] std::filesystem::path configTypeToPath(ConfigType configType);
    [[nodiscard]] std::filesystem::path configTypeToPath(ConfigType configType, const IEnvironment& environment);

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
        bool repeat{ false };
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
        std::string application{};
        std::vector<std::string> blacklist{};
        std::string statusNotificationMode{ defaults::DefaultStatusNotificationMode };
        std::string logLevel{ defaults::DefaultLogLevel };
        bool setupCompleted{ false };
        std::string uiLanguage{ defaults::DefaultUiLanguage.data() };
    };

    struct ConfigData
    {
        std::vector<CommandData> commands;
        std::vector<RecordedSequence> sequences;
        std::string endKey;
        std::string application;
        std::vector<std::string> blacklist;
        std::string statusNotificationMode;
        std::string logLevel;
    };

    /**
     * @brief Gets the path to the system configs directory.
     * @return The path to the configs directory.
     */
    [[nodiscard]] std::filesystem::path getConfigsPath();

    /**
     * @brief Gets the path to the user-specific configs directory.
     * @return The path to the user configs directory.
     */
    [[nodiscard]] std::filesystem::path getUserConfigsPath();

    /**
     * @brief Gets the path to the system configs directory using a specific environment.
     * @param environment The environment to use for path detection.
     * @return The path to the configs directory.
     */
    [[nodiscard]] std::filesystem::path getConfigsPath(const IEnvironment& environment);

    /**
     * @brief Gets the path to the user-specific configs directory using a specific environment.
     * @param environment The environment to use for path detection.
     * @return The path to the user configs directory.
     */
    [[nodiscard]] std::filesystem::path getUserConfigsPath(const IEnvironment& environment);

    /**
     * @brief Resolves a full configuration file path from a relative path or name.
     * @param filePath The relative path or config name.
     * @return The resolved absolute path.
     */
    [[nodiscard]] std::filesystem::path getConfigFilePath(const std::string& filePath);

    /**
     * @brief Resolves a configuration name or path to a full file path using a specific environment.
     * @param filePath The configuration name (e.g. "default") or path.
     * @param environment The environment to use for path detection.
     * @return The resolved filesystem path.
     */
    [[nodiscard]] std::filesystem::path getConfigFilePath(const std::string& filePath, const IEnvironment& environment);

    /**
     * @brief Loads configuration data from a file.
     * @param configPath The path to the configuration file.
     * @return An optional ConfigData object if successful.
     */
    std::optional<ConfigData> loadConfigData(const std::filesystem::path& configPath);

    /**
     * @brief Saves configuration data to a file.
     * @param configData The data to save.
     * @param configPath The destination path.
     * @param defaults Optional default settings to omit from saving if they match.
     * @return True if successful.
     */
    bool saveConfigData(const ConfigData& configData, const std::filesystem::path& configPath, const std::optional<DefaultSettings>& defaults = std::nullopt);

    /**
     * @brief Duplicates a configuration file.
     * @param sourceNameOrPath Source config name or path.
     * @param destinationNameOrPath Destination config name or path.
     * @param overwrite Whether to overwrite the destination if it exists.
     * @return True if successful.
     */
    bool duplicateConfig(const std::string& sourceNameOrPath, const std::string& destinationNameOrPath, bool overwrite = false);

    /**
     * @brief Checks if a configuration file exists at the given path.
     * @param configPath The path to check.
     * @return True if it exists.
     */
    bool doesConfigDataExists(const std::filesystem::path& configPath);

    /**
     * @brief Lists all available configuration names from both system and user directories.
     * @return A vector of configuration names (without extension).
     */
    [[nodiscard]] std::vector<std::string> listAvailableConfigs();

    /**
     * @brief Tries to get a value from a TOML node.
     * @tparam node_type The type of the TOML node.
     * @tparam date_type The type of the value to retrieve.
     * @param node The TOML node to read from.
     * @param propertyName The name of the property to retrieve.
     * @param output Reference to the variable where the value will be stored.
     * @return True if the property was found and retrieved.
     */
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


#endif // INCLUDE_AUTOINPUT_CONFIG_CONFIG_H

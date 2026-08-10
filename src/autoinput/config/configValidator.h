/**
 * @file configValidator.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_CONFIG_CONFIGVALIDATOR_H
#define INCLUDE_AUTOINPUT_CONFIG_CONFIGVALIDATOR_H
#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace autoinput
{
    struct ConfigData;
    struct CommandData;
    struct RuntimeConfig;
    struct DefaultSettings;

    enum class ValidationSeverity : uint8_t
    {
        Info,
        Warning,
        Error
    };

    struct ValidationError
    {
        std::string message;
        ValidationSeverity severity{ ValidationSeverity::Error };
        std::string section;
        std::string field;
        std::string suggestedFix;
    };

    struct ValidationResult
    {
        bool isValid{ false };
        std::string configPath;
        std::vector<ValidationError> errors{};
    };

    /**
     * @brief Validates DefaultSettings (global settings).
     * @param settings The settings to validate.
     * @return A vector of validation errors. Empty if valid.
     */
    std::vector<ValidationError> validateSettings(const DefaultSettings& settings);

    /**
     * @brief Validates ConfigData (raw TOML data).
     * @param configData The raw config data.
     * @return A vector of validation errors. Empty if valid.
     */
    std::vector<ValidationError> validateConfigData(const ConfigData& configData);

    /**
     * @brief Validates a single CommandData.
     * @param command The command data to validate.
     * @param commandIndex The index of the command (used for error messages).
     * @return A vector of validation errors. Empty if valid.
     */
    std::vector<ValidationError> validateCommandData(const CommandData& command, size_t commandIndex = 0);

    /**
     * @brief Validates RuntimeConfig.
     * @param runtimeConfig The processed runtime config.
     * @return A vector of validation errors. Empty if valid.
     */
    std::vector<ValidationError> validateRuntimeConfig(const RuntimeConfig& runtimeConfig);

    /**
     * @brief Prints the validation results as JSON to stdout.
     * @param valid Whether the configuration is valid.
     * @param configPath The resolved path to the configuration file.
     * @param errors The validation errors.
     */
    void printValidationJson(bool valid, const std::string& configPath, std::vector<ValidationError>&& errors);
    void printValidationJson(const ValidationResult& validationResult);
}

#endif // INCLUDE_AUTOINPUT_CONFIG_CONFIGVALIDATOR_H

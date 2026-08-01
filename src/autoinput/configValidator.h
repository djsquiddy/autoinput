/**
 * @file configValidator.h
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_CONFIG_VALIDATOR_H
#define INCLUDE_AUTOINPUT_CONFIG_VALIDATOR_H
#pragma once

#include <string>
#include <vector>

namespace autoinput
{
    struct ConfigData;
    struct CommandData;
    struct RuntimeConfig;

    struct ValidationError
    {
        std::string message;
    };

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
}

#endif // INCLUDE_AUTOINPUT_CONFIG_VALIDATOR_H

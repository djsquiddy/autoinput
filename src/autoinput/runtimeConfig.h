/**
 * @file runtimeConfig.h
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_RUNTIME_CONFIG_H
#define INCLUDE_AUTOINPUT_RUNTIME_CONFIG_H
#pragma once

#include <string>
#include <vector>
#include <optional>
#include <variant>
#include "autoinput/types.h"
#include "autoinput/waitDelay.h"

namespace autoinput
{
    struct ConfigData;
    struct CommandData;
    class ProgramArguments;

    struct ValidationError
    {
        std::string message;
    };

    struct RuntimeCommand
    {
        ActionState action = ActionState::INVALID;
        std::vector<Mouse> buttons;
        std::vector<Key> keys;
        std::vector<Key> startKeys;
        WaitDelayData wait;
    };

    struct RuntimeConfig
    {
        std::vector<RuntimeCommand> commands;
        Key endKey;
        std::string application;
        std::vector<std::string> blacklist;
    };

    /**
     * @brief Converts ConfigData to RuntimeConfig with validation.
     * @param config The raw config data from TOML.
     * @return A RuntimeConfig or a ValidationError.
     */
    std::variant<RuntimeConfig, ValidationError> convertConfigToRuntime(const ConfigData& config);

    /**
     * @brief Converts a single CommandData to RuntimeCommand with validation.
     * @param command The raw command data.
     * @return A RuntimeCommand or a ValidationError.
     */
    std::variant<RuntimeCommand, ValidationError> convertCommandToRuntime(const CommandData& command);

    /**
     * @brief Converts ProgramArguments to RuntimeConfig with validation.
     * @param arguments The program arguments.
     * @return A RuntimeConfig or a ValidationError.
     */
    std::variant<RuntimeConfig, ValidationError> convertArgumentsToRuntime(const ProgramArguments& arguments);
}

#endif // INCLUDE_AUTOINPUT_RUNTIME_CONFIG_H

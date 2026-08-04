/**
 * @file runtimeConfig.cpp
 * @author djsquiddy
 * @date August 2026
 */

#include "autoinput/runtimeConfig.h"
#include "autoinput/arguments.h"
#include "autoinput/configValidator.h"
#include <format>
#include <algorithm>

namespace autoinput
{
    std::variant<RuntimeCommand, ValidationError> convertCommandToRuntime(const CommandData& command)
    {
        if (const auto errors = validateCommandData(command); !errors.empty())
        {
            return errors[0];
        }

        RuntimeCommand runtimeCommand;

        runtimeCommand.action = actionStateFromArguments(command.action);

        for (const auto& buttonStr : command.buttons)
        {
            runtimeCommand.buttons.push_back(Mouse::fromString(buttonStr));
        }

        for (const auto& keyStr : command.keys)
        {
            runtimeCommand.keys.push_back(Key::fromString(keyStr));
        }

        for (const auto& startKeyStr : command.startKeys)
        {
            runtimeCommand.startKeys.push_back(Key::fromString(startKeyStr));
        }

        if (!command.pressWait.empty())
        {
            runtimeCommand.wait.parseWaitTimeDelay(command.pressWait, true);
        }

        if (!command.releaseWait.empty())
        {
            runtimeCommand.wait.parseWaitTimeDelay(command.releaseWait, false);
        }

        return runtimeCommand;
    }

    std::variant<RuntimeConfig, ValidationError> convertConfigToRuntime(const ConfigData& config)
    {
        RuntimeConfig runtimeConfig;

        for (const auto& commandData : config.commands)
        {
            auto result = convertCommandToRuntime(commandData);
            if (std::holds_alternative<ValidationError>(result))
            {
                return std::get<ValidationError>(result);
            }
            runtimeConfig.commands.push_back(std::get<RuntimeCommand>(result));
        }

        if (config.endKey.empty())
        {
            return ValidationError{ "Invalid end key: ''" };
        }

        runtimeConfig.endKey = Key::fromString(config.endKey);
        if (runtimeConfig.endKey.character.empty())
        {
            return ValidationError{ std::format("Invalid end key: '{}'", config.endKey) };
        }

        runtimeConfig.application = config.application;
        runtimeConfig.blacklist = config.blacklist;

        return runtimeConfig;
    }

    std::variant<RuntimeConfig, ValidationError> convertArgumentsToRuntime(const ProgramArguments& arguments)
    {
        return convertConfigToRuntime(arguments.toConfigData());
    }
}

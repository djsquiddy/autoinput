/**
 * @file runtimeConfig.cpp
 * @author djsquiddy
 * @date August 2026
 */

#include "autoinput/runtimeConfig.h"
#include "autoinput/arguments.h"
#include <format>
#include <algorithm>

namespace autoinput
{
    namespace
    {
        bool isValidWaitDelay(std::string_view wait)
        {
            if (wait.empty()) return false;

            auto checkPart = [](std::string_view part) {
                if (part.empty()) return false;
                size_t i = 0;
                while (i < part.size() && std::isdigit(part[i])) i++;
                if (i == 0) return false; // No digits

                std::string_view unit = part.substr(i);
                return unit.empty() || unit == "ms" || unit == "s" || unit == "m";
            };

            if (wait.contains(".."))
            {
                size_t pos = wait.find("..");
                return checkPart(wait.substr(0, pos)) && checkPart(wait.substr(pos + 2));
            }

            return checkPart(wait);
        }
    }

    std::variant<RuntimeCommand, ValidationError> convertCommandToRuntime(const CommandData& command)
    {
        RuntimeCommand runtimeCommand;

        // Action validation
        runtimeCommand.action = actionStateFromArguments(command.action);
        if (runtimeCommand.action == ActionState::INVALID)
        {
            return ValidationError{ std::format("Invalid action: '{}'", command.action) };
        }

        // Buttons validation
        for (const auto& buttonStr : command.buttons)
        {
            Mouse mouse = Mouse::fromString(buttonStr);
            if (mouse.button == MouseButton::NONE)
            {
                return ValidationError{ std::format("Invalid mouse button: '{}'", buttonStr) };
            }
            runtimeCommand.buttons.push_back(mouse);
        }

        // Keys validation
        for (const auto& keyStr : command.keys)
        {
            if (keyStr.empty())
            {
                return ValidationError{ std::format("Invalid key: '{}'", keyStr) };
            }
            Key key = Key::fromString(keyStr);
            if (key.character.empty())
            {
                return ValidationError{ std::format("Invalid key: '{}'", keyStr) };
            }
            runtimeCommand.keys.push_back(key);
        }

        // Start keys validation
        for (const auto& startKeyStr : command.startKeys)
        {
            if (startKeyStr.empty())
            {
                return ValidationError{ std::format("Invalid start key: '{}'", startKeyStr) };
            }
            Key key = Key::fromString(startKeyStr);
            if (key.character.empty())
            {
                return ValidationError{ std::format("Invalid start key: '{}'", startKeyStr) };
            }
            runtimeCommand.startKeys.push_back(key);
        }

        // Wait delay validation
        if (!command.pressWait.empty())
        {
            if (!isValidWaitDelay(command.pressWait) || !runtimeCommand.wait.parseWaitTimeDelay(command.pressWait, true))
            {
                return ValidationError{ std::format("Invalid press wait delay: '{}'", command.pressWait) };
            }
        }

        if (!command.releaseWait.empty())
        {
            if (!isValidWaitDelay(command.releaseWait) || !runtimeCommand.wait.parseWaitTimeDelay(command.releaseWait, false))
            {
                return ValidationError{ std::format("Invalid release wait delay: '{}'", command.releaseWait) };
            }
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

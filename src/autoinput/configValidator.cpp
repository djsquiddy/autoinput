/**
 * @file configValidator.cpp
 * @author djsquiddy
 * @date August 2026
 */

#include "autoinput/configValidator.h"
#include "autoinput/config.h"
#include "autoinput/runtimeConfig.h"
#include "autoinput/types.h"
#include "autoinput/waitDelay.h"
#include <format>
#include <algorithm>
#include <cctype>
#include <string_view>

namespace autoinput
{
    std::vector<ValidationError> validateConfigData(const ConfigData& configData)
    {
        std::vector<ValidationError> errors;

        if (configData.commands.empty())
        {
            errors.push_back(ValidationError{ "Command list is empty." });
        }

        for (size_t i = 0; i < configData.commands.size(); ++i)
        {
            auto commandErrors = validateCommandData(configData.commands[i], i);
            for (auto& err : commandErrors)
            {
                err.message = std::format("Command {}: {}", i, err.message);
            }
            errors.insert(errors.end(), commandErrors.begin(), commandErrors.end());
        }

        if (configData.endKey.empty() || Key::fromString(configData.endKey).character.empty())
        {
            errors.push_back(ValidationError{ std::format("Invalid end key: '{}'", configData.endKey) });
        }

        return errors;
    }

    std::vector<ValidationError> validateCommandData(const CommandData& command, size_t /*commandIndex*/)
    {
        std::vector<ValidationError> errors;

        // Action validation
        if (actionStateFromArguments(command.action) == ActionState::INVALID)
        {
            errors.push_back(ValidationError{ std::format("Invalid action: '{}'", command.action) });
        }

        // Buttons validation
        for (const auto& buttonStr : command.buttons)
        {
            if (Mouse::fromString(buttonStr).button == MouseButton::NONE)
            {
                errors.push_back(ValidationError{ std::format("Invalid mouse button: '{}'", buttonStr) });
            }
        }

        // Keys validation
        for (const auto& keyStr : command.keys)
        {
            if (keyStr.empty() || Key::fromString(keyStr).character.empty())
            {
                errors.push_back(ValidationError{ std::format("Invalid key: '{}'", keyStr) });
            }
        }

        // Start keys validation
        for (const auto& startKeyStr : command.startKeys)
        {
            if (startKeyStr.empty())
            {
                errors.push_back(ValidationError{ std::format("Invalid start key: '{}'", startKeyStr) });
            }
            else
            {
                Key key = Key::fromString(startKeyStr);
                if (key.character.empty())
                {
                    errors.push_back(ValidationError{ std::format("Invalid start key: '{}'", startKeyStr) });
                }
                else if (key.character.length() > 1 && !(static_cast<bool>(key.modifier & KeyModifier::Function)))
                {
                    // Check if it's a known special key
                    static const std::vector<std::string_view> specialKeys = {
                        "esc", "escape", "space", "tab", "enter", "return", "backspace", "ins", "insert",
                        "del", "delete", "home", "end", "pageup", "pgup", "pagedown", "pgdn", "up", "down",
                        "left", "right", "capslock", "numlock", "scrolllock", "printscreen", "prtsc", "pause"
                    };
                    if (std::find(specialKeys.begin(), specialKeys.end(), key.character) == specialKeys.end())
                    {
                        errors.push_back(ValidationError{ std::format("Invalid start key: '{}'", startKeyStr) });
                    }
                }
            }
        }

        // Wait strings validation
        if (!command.pressWait.empty())
        {
            if (!isValidWaitDelay(command.pressWait))
            {
                errors.push_back(ValidationError{ std::format("Invalid press wait delay: '{}'", command.pressWait) });
            }
        }

        if (!command.releaseWait.empty())
        {
            if (!isValidWaitDelay(command.releaseWait))
            {
                errors.push_back(ValidationError{ std::format("Invalid release wait delay: '{}'", command.releaseWait) });
            }
        }

        return errors;
    }

    std::vector<ValidationError> validateRuntimeConfig(const RuntimeConfig& runtimeConfig)
    {
        std::vector<ValidationError> errors;

        if (runtimeConfig.commands.empty())
        {
            errors.push_back(ValidationError{ "Runtime command list is empty." });
        }

        for (size_t i = 0; i < runtimeConfig.commands.size(); ++i)
        {
            const auto& command = runtimeConfig.commands[i];
            if (command.action == ActionState::INVALID)
            {
                errors.push_back(ValidationError{ std::format("Runtime command {}: Invalid action.", i) });
            }
            
            for (const auto& button : command.buttons)
            {
                if (button.button == MouseButton::NONE)
                {
                    errors.push_back(ValidationError{ std::format("Runtime command {}: Invalid mouse button.", i) });
                }
            }

            for (const auto& key : command.keys)
            {
                if (key.character.empty())
                {
                    errors.push_back(ValidationError{ std::format("Runtime command {}: Invalid key.", i) });
                }
            }

            for (const auto& key : command.startKeys)
            {
                if (key.character.empty())
                {
                    errors.push_back(ValidationError{ std::format("Runtime command {}: Invalid start key.", i) });
                }
            }
        }

        if (runtimeConfig.endKey.character.empty())
        {
            errors.push_back(ValidationError{ "Runtime end key is invalid." });
        }

        return errors;
    }
}

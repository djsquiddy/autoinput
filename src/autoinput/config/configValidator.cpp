/**
 * @file configValidator.cpp
 * @author djsquiddy
 * @date August 2026
 */

#include "autoinput/config/configValidator.h"
#include "autoinput/config/config.h"
#include "autoinput/config/runtimeConfig.h"
#include "autoinput/support/types.h"
#include "autoinput/input/waitDelay.h"
#include "autoinput/config/configMetadata.h"
#include "autoinput/support/utils.h"
#include <format>
#include <algorithm>
#include <string_view>

#include "autoinput/support/logger.h"

namespace autoinput
{
    /**
     * @brief Validates whether a string represents a valid trigger (keyboard key or mouse button).
     * @param triggerStr The trigger string to validate.
     * @return True if valid, false otherwise.
     */
    bool isValidTrigger(const std::string& triggerStr)
    {
        if (triggerStr.empty())
        {
            return false;
        }

        // 1. First check whether the trigger string maps to a valid mouse button.
        // We only allow back and forward buttons as triggers by default to avoid accidental triggers 
        // with primary mouse buttons, but the requirement suggests reusing existing parsing logic.
        const Mouse mouse = Mouse::fromString(triggerStr);
        if (mouse.button != MouseButton::None)
        {
            return true;
        }

        // 2. If not a mouse button, validate as a keyboard key.
        const Key key = Key::fromString(triggerStr);
        if (key.character.empty())
        {
            return false;
        }

        if (key.character.length() > 1 && !(static_cast<bool>(key.modifier & KeyModifier::Function)))
        {
            // Check if it's a known special key
            const auto specialKeys = ConfigMetadata::validSpecialKeyNames();
            if (std::find(specialKeys.begin(), specialKeys.end(), key.character) == specialKeys.end())
            {
                return false;
            }
        }

        return true;
    }

    std::vector<ValidationError> validateSettings(const DefaultSettings& settings)
    {
        std::vector<ValidationError> errors;

        if (settings.start.empty())
        {
            errors.push_back(ValidationError{ "Start key is required.", ValidationSeverity::Error, "settings", "start", "Set a valid key for 'start'." });
        }
        else if (!isValidTrigger(settings.start))
        {
            errors.push_back(ValidationError{ std::format("Invalid start key: '{}'", settings.start), ValidationSeverity::Error, "settings", "start", "Use a valid key name." });
        }

        if (settings.end.empty())
        {
            errors.push_back(ValidationError{ "End key is required.", ValidationSeverity::Error, "settings", "end", "Set a valid key for 'end'." });
        }
        else if (!isValidTrigger(settings.end))
        {
            errors.push_back(ValidationError{ std::format("Invalid end key: '{}'", settings.end), ValidationSeverity::Error, "settings", "end", "Use a valid key name." });
        }

        if (!settings.press.empty() && !isValidWaitDelay(settings.press))
        {
            errors.push_back(ValidationError{ std::format("Invalid press wait delay: '{}'", settings.press), ValidationSeverity::Error, "settings", "press", "Use a valid duration format (e.g., '10ms', '1s')." });
        }

        if (!settings.release.empty() && !isValidWaitDelay(settings.release))
        {
            errors.push_back(ValidationError{ std::format("Invalid release wait delay: '{}'", settings.release), ValidationSeverity::Error, "settings", "release", "Use a valid duration format (e.g., '10ms', '1s')." });
        }

        return errors;
    }

    std::vector<ValidationError> validateRecordedSequence(const RecordedSequence& sequence, size_t index)
    {
        std::vector<ValidationError> errors;
        std::string section = std::format("sequence[{}]", index);

        if (sequence.start.empty())
        {
            errors.push_back(ValidationError{ "Sequence start key is required.", ValidationSeverity::Error, section, "start", "Set a valid key for 'start'." });
        }
        else if (!isValidTrigger(sequence.start))
        {
            errors.push_back(ValidationError{ std::format("Invalid sequence start key: '{}'", sequence.start), ValidationSeverity::Error, section, "start", "Use a valid key name." });
        }

        for (size_t i = 0; i < sequence.events.size(); ++i)
        {
            const auto& event = sequence.events[i];
            std::string field = std::format("events[{}]", i);
            if (event.type == RecordedEventType::Invalid)
            {
                errors.push_back(ValidationError{ std::format("Event {}: Unknown event type.", i), ValidationSeverity::Error, section, field, "Correct the event type." });
            }

            if (!event.delay.empty() && !isValidWaitDelay(event.delay))
            {
                errors.push_back(ValidationError{ std::format("Event {}: Invalid delay '{}'.", i, event.delay), ValidationSeverity::Error, section, field, "Use a valid duration format (e.g., '10ms', '1s')." });
            }

            if (event.type == RecordedEventType::KeyDown || event.type == RecordedEventType::KeyUp)
            {
                if (!event.key || event.key->empty() || Key::fromString(*event.key).character.empty())
                {
                    errors.push_back(ValidationError{ std::format("Event {}: Key is required for key event.", i), ValidationSeverity::Error, section, field, "Specify a valid key." });
                }
            }
            else if (event.type == RecordedEventType::MouseDown || event.type == RecordedEventType::MouseUp)
            {
                if (!event.button || event.button->empty() || Mouse::fromString(*event.button).button == MouseButton::None)
                {
                    errors.push_back(ValidationError{ std::format("Event {}: Button is required for mouse button event.", i), ValidationSeverity::Error, section, field, "Specify a valid mouse button." });
                }
            }
            else if (event.type == RecordedEventType::MouseMove)
            {
                if (!event.x || !event.y)
                {
                    errors.push_back(ValidationError{ std::format("Event {}: x and y are required for mouse move event.", i), ValidationSeverity::Error, section, field, "Specify both x and y coordinates." });
                }
            }
        }

        return errors;
    }

    std::vector<ValidationError> validateConfigData(const ConfigData& configData)
    {
        std::vector<ValidationError> errors;

        if (configData.commands.empty() && configData.sequences.empty())
        {
            errors.push_back(ValidationError{ "Config is empty (no commands or sequences).", ValidationSeverity::Warning, "config", "", "Add at least one command or sequence." });
        }

        std::vector<std::string> names;
        for (size_t i = 0; i < configData.commands.size(); ++i)
        {
            const auto& cmd = configData.commands[i];
            if (!cmd.name.empty())
            {
                if (std::find(names.begin(), names.end(), cmd.name) != names.end())
                {
                    errors.push_back(ValidationError{ std::format("Duplicate command name: '{}'", cmd.name), ValidationSeverity::Error, std::format("command[{}]", i), "name", "Choose a unique name." });
                }
                names.push_back(cmd.name);
            }

            auto commandErrors = validateCommandData(cmd, i);
            errors.insert(errors.end(), commandErrors.begin(), commandErrors.end());
        }

        for (size_t i = 0; i < configData.sequences.size(); ++i)
        {
            const auto& seq = configData.sequences[i];
            if (!seq.name.empty())
            {
                if (std::find(names.begin(), names.end(), seq.name) != names.end())
                {
                    errors.push_back(ValidationError{ std::format("Duplicate sequence name: '{}'", seq.name), ValidationSeverity::Error, std::format("sequence[{}]", i), "name", "Choose a unique name." });
                }
                names.push_back(seq.name);
            }

            auto sequenceErrors = validateRecordedSequence(seq, i);
            errors.insert(errors.end(), sequenceErrors.begin(), sequenceErrors.end());
        }

        if (!isValidTrigger(configData.endKey))
        {
            errors.push_back(ValidationError{ std::format("Invalid end key: '{}'", configData.endKey), ValidationSeverity::Error, "config", "endKey", "Use a valid key name." });
        }

        return errors;
    }

    std::vector<ValidationError> validateCommandData(const CommandData& command, size_t commandIndex)
    {
        std::vector<ValidationError> errors;
        std::string section = std::format("command[{}]", commandIndex);

        // Action validation
        if (actionStateFromArguments(command.action) == ActionState::INVALID)
        {
            errors.push_back(ValidationError{ std::format("Invalid action: '{}'", command.action), ValidationSeverity::Error, section, "action", "Use a valid action name (e.g., 'click', 'press', 'type')." });
        }

        // Buttons validation
        for (const auto& buttonStr : command.buttons)
        {
            if (Mouse::fromString(buttonStr).button == MouseButton::None)
            {
                errors.push_back(ValidationError{ std::format("Invalid mouse button: '{}'", buttonStr), ValidationSeverity::Error, section, "buttons", "Use a valid mouse button name." });
            }
        }

        // Keys validation
        for (const auto& keyStr : command.keys)
        {
            if (keyStr.empty() || Key::fromString(keyStr).character.empty())
            {
                errors.push_back(ValidationError{ std::format("Invalid key: '{}'", keyStr), ValidationSeverity::Error, section, "keys", "Use a valid key name." });
            }
        }

        // Start keys validation
        for (const auto& startKeyStr : command.startKeys)
        {
            if (!isValidTrigger(startKeyStr))
            {
                errors.push_back(ValidationError{ std::format("Invalid start key: '{}'", startKeyStr), ValidationSeverity::Error, section, "startKeys", "Use a valid key name." });
            }
        }

        // Controls validation
        for (size_t cIdx = 0; cIdx < command.controls.size(); ++cIdx)
        {
            const auto& ctrl = command.controls[cIdx];
            const std::string ctrlSection = std::format("{}.controls[{}]", section, cIdx);
            if (controlActionFromString(ctrl.action) == ControlAction::Invalid)
            {
                errors.push_back(ValidationError{
                    std::format("Invalid control action: '{}'", ctrl.action),
                    ValidationSeverity::Error,
                    ctrlSection,
                    "action",
                    "Use a valid control action (start, toggle, stop, cancel, pause, resume, toggle-pause, stop-all, exit)."
                });
            }

            if (ctrl.input.empty() || !isValidTrigger(ctrl.input))
            {
                errors.push_back(ValidationError{
                    std::format("Invalid control input: '{}'", ctrl.input),
                    ValidationSeverity::Error,
                    ctrlSection,
                    "input",
                    "Use a valid key or mouse button trigger."
                });
            }
        }

        // Wait strings validation
        if (!command.pressWait.empty())
        {
            if (!isValidWaitDelay(command.pressWait))
            {
                errors.push_back(ValidationError{ std::format("Invalid press wait delay: '{}'", command.pressWait), ValidationSeverity::Error, section, "pressWait", "Use a valid duration format (e.g., '10ms', '1s')." });
            }
        }

        if (!command.releaseWait.empty())
        {
            if (!isValidWaitDelay(command.releaseWait))
            {
                errors.push_back(ValidationError{ std::format("Invalid release wait delay: '{}'", command.releaseWait), ValidationSeverity::Error, section, "releaseWait", "Use a valid duration format (e.g., '10ms', '1s')." });
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
                if (button.button == MouseButton::None)
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

    void printValidationJson(const bool valid, const std::string& configPath, std::vector<ValidationError>&& errors)
    {
        printValidationJson({
            .isValid = valid,
            .configPath = configPath,
            .errors = std::move(errors)
        });
    }

    void printValidationJson(const ValidationResult& validationResult)
    {
        Logger::print("{\n");
        Logger::print("  \"valid\": {},\n", validationResult.isValid ? "true" : "false");
        Logger::print("  \"configPath\": \"{}\",\n", escapeJsonString(validationResult.configPath));
        Logger::print("  \"errors\": [");
        const auto errorCount = validationResult.errors.size();
        for (size_t i = 0; i < errorCount; ++i)
        {
            const auto& error = validationResult.errors[i];
            Logger::print("\n    {\n");
            Logger::print("      \"message\": \"{}\",\n", escapeJsonString(error.message));
            
            std::string severityStr = "error";
            if (error.severity == ValidationSeverity::Info) severityStr = "info";
            else if (error.severity == ValidationSeverity::Warning) severityStr = "warning";
            
            Logger::print("      \"severity\": \"{}\",\n", severityStr);
            Logger::print("      \"section\": \"{}\",\n", escapeJsonString(error.section));
            Logger::print("      \"field\": \"{}\",\n", escapeJsonString(error.field));
            Logger::print("      \"suggestedFix\": \"{}\"\n", escapeJsonString(error.suggestedFix));
            Logger::print("    }");
            if (i < errorCount - 1)
            {
                Logger::print(",");
            }
        }
        if (errorCount > 0)
        {
            Logger::print("\n  ");
        }
        Logger::print("]\n");
        Logger::print("}\n");
    }
}

/**
 * @file configMetadata.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/config/configMetadata.h"
#include "autoinput/config/defaults.h"
#include "autoinput/support/utils.h"

namespace autoinput
{
    std::vector<std::string_view> ConfigMetadata::validActionNames()
    {
        return { "click", "hold" };
    }

    std::vector<std::string_view> ConfigMetadata::validActionAliases()
    {
        return { "click", "c", "hold", "h" };
    }

    std::string ConfigMetadata::validActionChoices()
    {
        return "{" + join(validActionAliases(), ",") + "}";
    }

    std::vector<std::string_view> ConfigMetadata::validControlActionNames()
    {
        return { "start", "toggle", "stop", "cancel", "pause", "resume", "toggle-pause", "stop-all", "exit" };
    }

    std::vector<std::string_view> ConfigMetadata::validControlActionAliases()
    {
        return { "start", "toggle", "stop", "cancel", "pause", "resume", "toggle-pause", "toggle_pause", "stop-all", "stop_all", "exit" };
    }

    std::string ConfigMetadata::validControlActionChoices()
    {
        return "{" + join(validControlActionAliases(), ",") + "}";
    }

    std::string_view ConfigMetadata::defaultControlActionName()
    {
        return "toggle";
    }

    std::vector<std::string_view> ConfigMetadata::validMouseButtonNames()
    {
        return { "left", "right", "middle", "back", "forward" };
    }

    std::vector<std::string_view> ConfigMetadata::validMouseButtonAliases()
    {
        return { "left", "l", "right", "r", "middle", "m", "back", "forward" };
    }

    std::string ConfigMetadata::validMouseButtonChoices()
    {
        return "{" + join(validMouseButtonAliases(), ",") + "}";
    }

    std::string_view ConfigMetadata::defaultMouseButtonName()
    {
        return defaults::DefaultMouseButtonName;
    }

    std::string_view ConfigMetadata::defaultActionName()
    {
        return defaults::DefaultActionName;
    }

    std::string_view ConfigMetadata::defaultStartKey()
    {
        return defaults::StartKey;
    }

    std::string_view ConfigMetadata::defaultEndKey()
    {
        return defaults::EndKey;
    }

    std::vector<std::string_view> ConfigMetadata::validLogLevelNames()
    {
        return { "debug", "info", "warning", "warn", "error", "fatal" };
    }

    std::vector<std::string_view> ConfigMetadata::validLogLevelAliases()
    {
        return { "d", "debug", "i", "info", "w", "warn", "warning", "e", "error", "f", "fatal" };
    }

    std::string ConfigMetadata::validLogLevelChoices()
    {
        return "{" + join(validLogLevelAliases(), ",") + "}";
    }

    std::vector<std::string_view> ConfigMetadata::validSpecialKeyNames()
    {
        return {
            "esc", "escape", "space", "tab", "enter", "return", "backspace", "ins", "insert",
            "del", "delete", "home", "end", "pageup", "pgup", "pagedown", "pgdn", "up", "down",
            "left", "right", "capslock", "numlock", "scrolllock", "printscreen", "prtsc", "pause"
        };
    }

    std::vector<std::string_view> ConfigMetadata::validWildcardInputNames()
    {
        return { "mouse.all", "keys.all", "input.all" };
    }

    std::vector<std::string_view> ConfigMetadata::validWildcardInputAliases()
    {
        return {
            "mouse.all", "mouse.*", "mouse.any", "mouse_all", "mouse_any", "mouse-all", "mouse-any",
            "keys.all", "key.all", "keys.*", "key.*", "keys.any", "key.any", "keys_all", "key_all",
            "keys-all", "key-all", "keys_any", "key_any", "keys-any", "key-any", "keyboard.all",
            "keyboard.*", "keyboard.any",
            "input.all", "inputs.all", "input.*", "inputs.*", "input.any", "inputs.any",
            "input_all", "inputs_all", "input-all", "inputs-all", "input_any", "inputs_any",
            "input-any", "inputs-any", "all", "any"
        };
    }

    bool ConfigMetadata::isMouseAllTrigger(const std::string_view str)
    {
        const std::string s = toLowerCase(str);
        return s == "mouse.all" || s == "mouse.*" || s == "mouse.any" ||
               s == "mouse_all" || s == "mouse_any" || s == "mouse-all" || s == "mouse-any" ||
               s == "mouse:all" || s == "mouse:any";
    }

    bool ConfigMetadata::isKeysAllTrigger(const std::string_view str)
    {
        const std::string s = toLowerCase(str);
        return s == "keys.all" || s == "key.all" || s == "keys.*" || s == "key.*" ||
               s == "keys.any" || s == "key.any" || s == "keys_all" || s == "key_all" ||
               s == "keys-all" || s == "key-all" || s == "keys_any" || s == "key_any" ||
               s == "keys-any" || s == "key-any" || s == "keys:all" || s == "key:all" ||
               s == "keyboard.all" || s == "keyboard.*" || s == "keyboard.any" ||
               s == "keys" || s == "key";
    }

    bool ConfigMetadata::isInputAllTrigger(const std::string_view str)
    {
        const std::string s = toLowerCase(str);
        return s == "input.all" || s == "inputs.all" || s == "input.*" || s == "inputs.*" ||
               s == "input.any" || s == "inputs.any" || s == "input_all" || s == "inputs_all" ||
               s == "input-all" || s == "inputs-all" || s == "input_any" || s == "inputs_any" ||
               s == "input-any" || s == "inputs-any" || s == "input:all" || s == "inputs:all" ||
               s == "all" || s == "any";
    }

    bool ConfigMetadata::isWildcardTrigger(const std::string_view str)
    {
        return isMouseAllTrigger(str) || isKeysAllTrigger(str) || isInputAllTrigger(str);
    }
}

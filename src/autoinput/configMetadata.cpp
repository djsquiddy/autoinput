/**
 * @file configMetadata.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/configMetadata.h"
#include "autoinput/defaults.h"
#include "autoinput/utils.h"

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
}

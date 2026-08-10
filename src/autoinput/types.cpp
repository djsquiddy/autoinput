/**
 * @file types.cpp
 * @author djsquiddy
 * @date March 2026
 */
#include "autoinput/types.h"
#include "autoinput/defaults.h"
#include "autoinput/logger.h"
#include "autoinput/utils.h"
#include <algorithm>
#include <ranges>
#include <cctype>
#include <charconv>
#include <system_error>

namespace autoinput
{


    std::string toString(const KeyModifier modifier)
    {
        std::vector<std::string> result;
        if (static_cast<bool>(modifier & KeyModifier::Ctrl))
        {
            result.emplace_back("ctrl");
        }
        if (static_cast<bool>(modifier & KeyModifier::Alt))
        {
            result.emplace_back("alt");
        }
        if (static_cast<bool>(modifier & KeyModifier::Shift))
        {
            result.emplace_back("shift");
        }
        if (static_cast<bool>(modifier & KeyModifier::Meta))
        {
            result.emplace_back("meta");
        }

        // ReSharper disable once CppRedundantQualifier
        std::string modifierStr = autoinput::join(result, "+");
        if (static_cast<bool>(modifier & KeyModifier::Function))
        {
            return modifierStr.empty() ? "f" : modifierStr + "+f";
        }
        return modifierStr;
    }

    Mouse Mouse::fromString(const std::string_view& keyValue)
    {
        const std::vector<std::string> result = keyValue
            | std::views::split('+')
            | std::views::transform([](auto&& range) {
                std::string s(range.begin(), range.end());
                std::ranges::transform(s, s.begin(), [](unsigned char c) {
                    return std::tolower(c);
                });
                return s;
            })
            | std::ranges::to<std::vector>();

        Mouse mouse{};
        for (const std::string& s : result)
        {
            if (s == "ctrl")
            {
                mouse.modifier |= KeyModifier::Ctrl;
            }
            else if (s == "shift")
            {
                mouse.modifier |= KeyModifier::Shift;
            }
            else if (s == "alt")
            {
                mouse.modifier |= KeyModifier::Alt;
            }
            else if (s == "meta")
            {
                mouse.modifier |= KeyModifier::Meta;
            }
            else
            {
                const auto button = mouseButtonFromArguments(s);
                if (button != MouseButton::None)
                {
                    mouse.button = button;
                }
            }
        }
        return mouse;
    }

    std::string Mouse::toString() const
    {
        const std::string modifiers = autoinput::toString(modifier);
        std::string buttonStr = mouseButtonToString(button);
        if (modifiers.empty())
        {
            return buttonStr;
        }

        return modifiers + '+' + buttonStr;
    }

    std::vector<std::string_view> getAllStatusNotificationModes()
    {
        return {
            statusNotificationModeToString(StatusNotificationMode::Off),
            statusNotificationModeToString(StatusNotificationMode::Console),
            statusNotificationModeToString(StatusNotificationMode::Desktop),
            statusNotificationModeToString(StatusNotificationMode::Both)
        };
    }

    StatusNotificationMode statusNotificationModeFromString(const std::string_view str)
    {
        const std::string lowerStr = toLowerCase(str);
        if (lowerStr == "off") return StatusNotificationMode::Off;
        if (lowerStr == "console") return StatusNotificationMode::Console;
        if (lowerStr == "desktop") return StatusNotificationMode::Desktop;
        if (lowerStr == "both") return StatusNotificationMode::Both;
        return StatusNotificationMode::Console; // Default
    }

    std::string_view statusNotificationModeToString(const StatusNotificationMode mode)
    {
        switch (mode)
        {
        case StatusNotificationMode::Off: return "off";
        case StatusNotificationMode::Console: return "console";
        case StatusNotificationMode::Desktop: return "desktop";
        case StatusNotificationMode::Both: return "both";
        default: return "console";
        }
    }

    NotificationSeverity notificationSeverityFromString(std::string_view str)
    {
        if (str == "success") return NotificationSeverity::Success;
        if (str == "warning") return NotificationSeverity::Warning;
        if (str == "error")   return NotificationSeverity::Error;
        return NotificationSeverity::Info;
    }

    std::string_view notificationSeverityToString(NotificationSeverity severity)
    {
        switch (severity)
        {
        case NotificationSeverity::Info:    return "info";
        case NotificationSeverity::Success: return "success";
        case NotificationSeverity::Warning: return "warning";
        case NotificationSeverity::Error:   return "error";
        default:                            return "info";
        }
    }

    std::string_view recordedEventTypeToString(const RecordedEventType type)
    {
        switch (type)
        {
        case RecordedEventType::KeyDown: return "key_down";
        case RecordedEventType::KeyUp: return "key_up";
        case RecordedEventType::MouseDown: return "mouse_down";
        case RecordedEventType::MouseUp: return "mouse_up";
        case RecordedEventType::MouseMove: return "mouse_move";
        default: return "invalid";
        }
    }

    RecordedEventType recordedEventTypeFromString(const std::string_view str)
    {
        const std::string lowerStr = toLowerCase(str);
        if (lowerStr == "key_down") return RecordedEventType::KeyDown;
        if (lowerStr == "key_up") return RecordedEventType::KeyUp;
        if (lowerStr == "mouse_down") return RecordedEventType::MouseDown;
        if (lowerStr == "mouse_up") return RecordedEventType::MouseUp;
        if (lowerStr == "mouse_move") return RecordedEventType::MouseMove;
        return RecordedEventType::Invalid;
    }

    Key Key::fromString(const std::string_view& keyValue)
    {
        // 1. Split the view
        // 2. Transform each subrange into a lowercase std::string
        const std::vector<std::string> result = keyValue
            | std::views::split('+')
            | std::views::transform([](auto&& range) {
                // Construct std::string from the view subrange
                std::string s(range.begin(), range.end());

                // Convert to lowercase in-place
                std::ranges::transform(s, s.begin(), [](unsigned char c) {
                    return std::tolower(c);
                });
                return s;
            })
            | std::ranges::to<std::vector>(); // C++23 helper, or use constructor below

        Key key{};
        for (const std::string& s : result)
        {
            if (s == "ctrl")
            {
                key.modifier |= KeyModifier::Ctrl;
            }
            else if (s == "shift")
            {
                key.modifier |= KeyModifier::Shift;
            }
            else if (s == "alt")
            {
                key.modifier |= KeyModifier::Alt;
            }
            else if (s == "meta")
            {
                key.modifier |= KeyModifier::Meta;
            }
            else if (s.starts_with('f') && s.length() > 1 && std::isdigit(s[1]))
            {
                key.modifier |= KeyModifier::Function;
                key.character = s.substr(1);
            }
            else
            {
                key.character = s;
            }
        }
        return key;
    }

    Key Key::fromKeyState(const KeyState& state)
    {
        Key key;
        key.modifier = state.modifier;
        if (state.functionKey != INVALID_KEY)
        {
            key.character = std::format("f{}", state.functionKey);
            key.modifier |= KeyModifier::Function;
        }
        else if (state.keyCode != INVALID_KEY)
        {
            key.character = static_cast<char>(state.keyCode);
        }
        return key;
    }

    std::string Key::toString() const
    {
        const std::string modifiers = autoinput::toString(modifier);
        if (modifiers.empty())
        {
            return { character };
        }

        if (static_cast<bool>(modifier & KeyModifier::Function))
        {
            if (modifiers == "f")
            {
                return 'f' + character;
            }

            return modifiers + character;
        }
        return modifiers + '+' + character;
    }

    ActionState actionStateFromArguments(const std::string_view actionType)
    {
        if (actionType == "c" || actionType == "click")
        {
            return ActionState::CLICK;
        }
        if (actionType == "h" || actionType == "hold")
        {
            return ActionState::HOLD;
        }
        return ActionState::INVALID;
    }

    std::string actionStateToString(const ActionState actionState)
    {
        switch (actionState)
        {
        case ActionState::CLICK:
            return std::string{ "click" };
        case ActionState::HOLD:
            return "hold";
        case ActionState::INVALID:
        default:
            return "invalid";
        }
    }

    MouseButton mouseButtonFromArguments(const std::string_view button)
    {
        if (button == "l" || button == "left")
        {
            return MouseButton::Left;
        }
        if (button == "r" || button == "right")
        {
            return MouseButton::Right;
        }
        if (button == "m" || button == "middle")
        {
            return MouseButton::Middle;
        }
        if (button == "back")
        {
            return MouseButton::Back;
        }
        if (button == "forward")
        {
            return MouseButton::Forward;
        }
        return MouseButton::None;
    }

    std::string mouseButtonToString(const MouseButton& mouseButton)
    {
        switch (mouseButton)
        {
        case MouseButton::Left:
            return std::string{ "left" };
        case MouseButton::Middle:
            return std::string{ "middle" };
        case MouseButton::Right:
            return std::string{ "right" };
        case MouseButton::Back:
            return std::string{ "back" };
        case MouseButton::Forward:
            return std::string{ "forward" };
        case MouseButton::None:
        default:
            return std::string{};
        }
    }

    int32_t parseStringToInt(const std::string_view value)
    {
        int32_t result{};

        // From: https://en.cppreference.com/w/cpp/utility/from_chars.html
        if (auto [ptr, ec] = std::from_chars(value.data(), value.data() + value.size(), result); ec == std::errc())
        {
            const std::string_view parsedText{ value.data(), static_cast<std::size_t>(ptr - value.data()) };
            const std::string_view remainingText{ ptr, static_cast<std::size_t>((value.data() + value.size()) - ptr) };
            if (remainingText.empty())
            {
                Logger::debug(
                    "Result: {}, parsed -> {}\n",
                    result,
                    Quoted(std::string{ parsedText })
                );
            }
            else
            {
                Logger::debug(
                    "Result: {}, parsed -> {}, remaining -> {}\n",
                    result,
                    Quoted(std::string{ parsedText }),
                    Quoted(std::string{ remainingText })
                );
            }
        }
        else if (ec == std::errc::invalid_argument)
        {
            Logger::error("This is not a number.\n");
        }
        else if (ec == std::errc::result_out_of_range)
        {
            Logger::error("This number is larger than an int.\n");
        }

        return result;
    }
}

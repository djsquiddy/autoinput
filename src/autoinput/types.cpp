//
// Created by djsquiddy on 3/9/2026.
//
#include "types.h"
#include "logger.h"
#include "utils.h"

namespace autoinput
{
    std::string getConsoleColor(const ConsoleColor color)
    {
        switch (color)
        {
        case ConsoleColor::Bold:
            return "\033[1m";
        case ConsoleColor::White:
            return "\033[37m";
        case ConsoleColor::Red:
            return "\033[31m";
        case ConsoleColor::Green:
            return "\033[32m";
        case ConsoleColor::Yellow:
            return "\033[33m";
        case ConsoleColor::Blue:
            return "\033[34m";
        case ConsoleColor::Magenta:
            return "\033[35m";
        case ConsoleColor::Cyan:
            return "\033[36m";
        case ConsoleColor::Reset:
        default:
            return "\033[0m";
        }
    }

    bold::bold(const std::string_view& s)
        : m_string(s)
    {
    }

    std::ostream& operator<<(std::ostream& os, const bold& b)
    {
        os << getConsoleColor(ConsoleColor::Bold) << b.m_string << getConsoleColor(ConsoleColor::Reset);
        return os;
    }

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
            else if (s=="meta")
            {
                key.modifier |= KeyModifier::Meta;
            }
            else if (s.length() == 1)
            {
                key.character = s;
            }
            else if (s.starts_with('f'))
            {
                key.modifier |= KeyModifier::Function;
                key.character = s.substr(1);
            }
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

    MouseButton mouseButtonFromArguments(const std::string_view button)
    {
        if (button == "l" || button == "left")
        {
            return MouseButton::LEFT;
        }
        if (button == "r" || button == "right")
        {
            return MouseButton::RIGHT;
        }
        if (button == "m" || button == "middle")
        {
            return MouseButton::MIDDLE;
        }
        return MouseButton::NONE;
    }

    std::string mouseButtonToString(const MouseButton& mouseButton)
    {
        switch (mouseButton)
        {
        case MouseButton::LEFT:
            return std::string{ "left" };
        case MouseButton::MIDDLE:
            return std::string{ "middle" };
        case MouseButton::RIGHT:
            return std::string{ "right" };
        case MouseButton::BACK:
            return std::string{ "back" };
        case MouseButton::FORWARD:
            return std::string{ "forward" };
        case MouseButton::NONE:
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

            Logger::info(
                "\nResult: {}, parsed -> {}, remaining -> {}\n",
                result,
                Quoted(std::string{ parsedText }),
                Quoted(std::string{ remainingText })
            );
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
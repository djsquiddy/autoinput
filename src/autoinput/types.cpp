//
// Created by djsquiddy on 3/9/2026.
//
#include "types.h"

#include <charconv>
#include <iomanip>
#include <string>
#include <iostream>

namespace autoinput
{
    bold::bold(const std::string_view& s)
        : m_string(s)
    {
    }

    std::ostream& operator<<(std::ostream& os, const bold& b)
    {
        os << "\x1b[1m" << b.m_string << "\x1b[0m";
        return os;
    }

    ButtonState buttonStateFromArguments(std::string_view buttonType)
    {
        if (buttonType == "click" || buttonType == "c")
        {
            return ButtonState::CLICK;
        }
        if (buttonType == "hold" || buttonType == "h")
        {
            return ButtonState::HOLD;
        }
        return ButtonState::INVALID;
    }

    MouseButton mouseButtonFromArguments(std::string_view button)
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
            std::cout << "\nResult: " << result << ", ptr -> " << std::quoted(ptr) << '\n';
        }
        else if (ec == std::errc::invalid_argument)
        {
            std::cout << "This is not a number.\n";
        }
        else if (ec == std::errc::result_out_of_range)
        {
            std::cout << "This number is larger than an int.\n";
        }

        return result;
    }
}
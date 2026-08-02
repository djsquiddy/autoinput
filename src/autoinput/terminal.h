/**
 * @file terminal.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_TERMINAL_H
#define INCLUDE_AUTOINPUT_TERMINAL_H
#pragma once

#include <iostream>
#include <string_view>
#include <format>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace autoinput::terminal
{
    inline void setup()
    {
#ifdef _WIN32
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut == INVALID_HANDLE_VALUE) return;

        DWORD dwMode = 0;
        if (!GetConsoleMode(hOut, &dwMode)) return;

        dwMode |= 0x0004; // ENABLE_VIRTUAL_TERMINAL_PROCESSING
        SetConsoleMode(hOut, dwMode);
#endif
    }

    enum class Color
    {
        Reset,
        Bold,
        White,
        Red,
        Green,
        Yellow,
        Blue,
        Magenta,
        Cyan,
        Gray
    };

    inline std::string_view colorToAnsi(const Color color)
    {
        switch (color)
        {
        case Color::Bold:    return "\033[1m";
        case Color::White:   return "\033[37m";
        case Color::Red:     return "\033[31m";
        case Color::Green:   return "\033[32m";
        case Color::Yellow:  return "\033[33m";
        case Color::Blue:    return "\033[34m";
        case Color::Magenta: return "\033[35m";
        case Color::Cyan:    return "\033[36m";
        case Color::Gray:    return "\033[90m";
        case Color::Reset:   return "\033[0m";
        default:             return "";
        }
    }

    inline void printStatus(const std::string_view label, const std::string_view status, const Color color)
    {
        std::cout << label << colorToAnsi(color) << status << colorToAnsi(Color::Reset) << std::endl;
    }

    class bold
    {
    public:
        explicit bold(const std::string_view& s) : m_string(s) {}

        friend std::ostream& operator<<(std::ostream& os, const bold& b)
        {
            os << colorToAnsi(Color::Bold) << b.m_string << colorToAnsi(Color::Reset);
            return os;
        }
        [[nodiscard]] const std::string_view& getString() const { return m_string; }
    private:
        const std::string_view& m_string;
    };
}

// Specialize std::formatter for autoinput::terminal::bold
template <>
struct std::formatter<autoinput::terminal::bold>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const autoinput::terminal::bold b, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}{}{}", 
            autoinput::terminal::colorToAnsi(autoinput::terminal::Color::Bold), 
            b.getString(), 
            autoinput::terminal::colorToAnsi(autoinput::terminal::Color::Reset));
    }
};

// Specialize std::formatter for autoinput::terminal::Color
template <>
struct std::formatter<autoinput::terminal::Color>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const autoinput::terminal::Color color, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}", autoinput::terminal::colorToAnsi(color));
    }
};

#endif // INCLUDE_AUTOINPUT_TERMINAL_H

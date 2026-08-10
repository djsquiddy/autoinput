/**
 * @file terminal.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_PLATFORM_TERMINAL_H
#define INCLUDE_AUTOINPUT_PLATFORM_TERMINAL_H
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
    /**
     * @brief Performs terminal setup (e.g. enabling ANSI processing on Windows).
     */
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

    /**
     * @brief Converts a Color enum value to an ANSI escape sequence string.
     * @param color The color.
     * @return The ANSI escape sequence.
     */
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

    /**
     * @brief Prints a labeled status message to the console with color.
     * @param label The label text.
     * @param status The status text.
     * @param color The color for the status text.
     */
    inline void printStatus(const std::string_view label, const std::string_view status, const Color color)
    {
        std::cout << label << colorToAnsi(color) << status << colorToAnsi(Color::Reset) << std::endl;
    }

    class colorized
    {
    public:
        explicit colorized(const Color color, const std::string_view& s) : m_color{ color }, m_str{ s } {}
        friend std::ostream& operator<<(std::ostream& os, const colorized& b)
        {
            os << colorToAnsi(b.m_color) << b.m_str << colorToAnsi(Color::Reset);
            return os;
        }

        [[nodiscard]] const Color& getColor() const { return m_color; }
        [[nodiscard]] const std::string_view& getString() const { return m_str; }

    private:
        Color m_color;
        const std::string_view& m_str;
    };

    class bold
    {
    public:
        explicit bold(const std::string_view& s) : m_str(s) {}

        friend std::ostream& operator<<(std::ostream& os, const bold& b)
        {
            os << colorToAnsi(Color::Bold) << b.m_str << colorToAnsi(Color::Reset);
            return os;
        }
        [[nodiscard]] const std::string_view& getString() const { return m_str; }
    private:
        const std::string_view& m_str;
    };
}

// ReSharper disable CppMemberFunctionMayBeStatic
// NOLINTBEGIN(*-convert-member-functions-to-static)
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

// Specialize std::formatter for autoinput::terminal::colorized
template <>
struct std::formatter<autoinput::terminal::colorized>
{
    constexpr auto parse(std::format_parse_context& ctx)
    {
        return ctx.begin();
    }

    auto format(const autoinput::terminal::colorized b, std::format_context& ctx) const
    {
        return std::format_to(ctx.out(), "{}{}{}",
            autoinput::terminal::colorToAnsi(b.getColor()),
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

// NOLINTEND(*-convert-member-functions-to-static)
// ReSharper restore CppMemberFunctionMayBeStatic

#endif // INCLUDE_AUTOINPUT_PLATFORM_TERMINAL_H

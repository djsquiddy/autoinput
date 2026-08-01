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
        Green,
        Yellow,
        Gray
    };

    inline std::string_view colorToAnsi(Color color)
    {
        switch (color)
        {
        case Color::Green:  return "\033[32m";
        case Color::Yellow: return "\033[33m";
        case Color::Gray:   return "\033[90m";
        case Color::Reset:  return "\033[0m";
        default:            return "";
        }
    }

    inline void printStatus(std::string_view label, std::string_view status, Color color)
    {
        std::cout << label << colorToAnsi(color) << status << colorToAnsi(Color::Reset) << std::endl;
    }
}

#endif // INCLUDE_AUTOINPUT_TERMINAL_H

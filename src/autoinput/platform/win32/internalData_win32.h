/**
 * @file internalData_win32.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_PLATFORM_INTERNALDATA_WIN32_H
#define INCLUDE_AUTOINPUT_PLATFORM_INTERNALDATA_WIN32_H
#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Psapi.h>  // IWYU pragma: export

namespace autoinput
{
    struct WindowsKeyboardData
    {
        WPARAM wParam{ 0 };
        KBDLLHOOKSTRUCT* kbdStruct{ nullptr };
    };

    struct WindowsMouseData
    {
        WPARAM wParam{ 0 };
        MSLLHOOKSTRUCT* mouseStruct{ nullptr };
    };
}
#endif


#endif // INCLUDE_AUTOINPUT_PLATFORM_INTERNALDATA_WIN32_H

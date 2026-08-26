/**
 * @file internalDataWin32.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_PLATFORM_INTERNALDATAWIN32_H
#define INCLUDE_AUTOINPUT_PLATFORM_INTERNALDATAWIN32_H
#pragma once

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <Psapi.h>  // IWYU pragma: export
#include "autoinput/platform/foregroundWindowListener.h"

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

    namespace platform
    {
        AppWindowInfo getAppWindowInfo(HWND hwnd);
    }

    class WindowsForegroundWindowListener : public IForegroundWindowListener
    {
    public:
        WindowsForegroundWindowListener() = default;
        ~WindowsForegroundWindowListener() override;

        bool start(ForegroundWindowCallback callback) override;
        void stop() override;
        [[nodiscard]] std::optional<AppWindowInfo> getForegroundWindow() override;
        [[nodiscard]] bool isSupported() const override { return true; }

        void notify(const AppWindowInfo& info);

    private:
        HWINEVENTHOOK m_hook{ nullptr };
        ForegroundWindowCallback m_callback;
    };
}
#endif


#endif // INCLUDE_AUTOINPUT_PLATFORM_INTERNALDATAWIN32_H

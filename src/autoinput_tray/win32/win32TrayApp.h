/**
 * @file win32TrayApp.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef AUTOINPUT_WIN32_TRAY_APP_H
#define AUTOINPUT_WIN32_TRAY_APP_H
#pragma once

#include "../trayApp.h"

#undef NOGDI
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifndef WM_TRAY_UPDATE_UI
#define WM_TRAY_UPDATE_UI (WM_USER + 2)
#endif // WM_TRAY_UPDATE_UI

namespace autoinput::tray
{
    class Win32TrayApp final : public TrayApp
    {
    public:
        Win32TrayApp();
        ~Win32TrayApp() override;

        void run() override;
        void shutdown() override;

    protected:
        void openConfigFolder() override;
        void wakeUIThread() override;

    private:
        HWND m_hwnd = NULL;
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        void handleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
    };

}

#endif // AUTOINPUT_WIN32_TRAY_APP_H

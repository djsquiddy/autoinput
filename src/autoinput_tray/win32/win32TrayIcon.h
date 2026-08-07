/**
 * @file win32TrayIcon.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef AUTOINPUT_WIN32_TRAY_ICON_H
#define AUTOINPUT_WIN32_TRAY_ICON_H
#pragma once

#ifdef _WIN32

#include "../trayIcon.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <map>

namespace autoinput::tray
{
    class Win32TrayIcon final : public ITrayIcon
    {
    public:
        explicit Win32TrayIcon(HWND hwnd);
        ~Win32TrayIcon() override;

        void show() override;
        void hide() override;
        void setTooltip(const std::string& tooltip) override;
        void setIcon(const std::string& iconResource) override;
        void setMenu(const std::vector<MenuItem>& items) override;
        void showNotification(const std::string& title, const std::string& message) override;
        void update() override;

        void handleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    private:
        HWND m_hwnd;
        NOTIFYICONDATAA m_nid;
        bool m_iconOwned = false;
        std::vector<MenuItem> m_menuItems;
        std::map<UINT, std::function<void()>> m_commandHandlers;
        UINT m_nextCommandId = 1001;

        void createMenu(HMENU hMenu, const std::vector<MenuItem>& items);
    };
}

#endif // _WIN32

#endif // AUTOINPUT_WIN32_TRAY_ICON_H

/**
 * @file notifications_win32.cpp
 * @author djsquiddy
 * @date August 2026
 */

#include "autoinput/notifications.h"
#include "autoinput/platform.h"
#include "autoinput/logger.h"
#include <windows.h>
#include <shellapi.h>
#include <string>

namespace autoinput
{
    namespace
    {
        // Unique ID for our notification
        constexpr UINT AutoInputNotificationIconId = 1;
        constexpr UINT WM_NOTIFICATION_CALLBACK = WM_APP + 1;
        constexpr auto NOTIFICATION_CLASS_NAME = "AutoInputNotificationWindow";

        LRESULT CALLBACK NotificationWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
        {
            return DefWindowProcA(hwnd, uMsg, wParam, lParam);
        }

        class WindowsDesktopNotificationSink : public INotificationSink
        {
        public:
            WindowsDesktopNotificationSink()
                : m_hwnd{ nullptr }, m_iconAdded{ false }
            {
                m_nid = {};
                m_nid.cbSize = sizeof(m_nid);
                
                if (initializeWindow())
                {
                    m_nid.hWnd = m_hwnd;
                    m_nid.uID = AutoInputNotificationIconId;
                    m_nid.uFlags = NIF_INFO | NIF_ICON | NIF_MESSAGE;
                    m_nid.uCallbackMessage = WM_NOTIFICATION_CALLBACK;
                    // ReSharper disable once CppZeroConstantCanBeReplacedWithNullptr
                    m_nid.hIcon = LoadIconA(NULL, (LPCSTR)IDI_APPLICATION);
                }
            }

            ~WindowsDesktopNotificationSink() override
            {
                if (m_iconAdded)
                {
                    Shell_NotifyIconA(NIM_DELETE, &m_nid);
                }

                if (m_hwnd)
                {
                    DestroyWindow(m_hwnd);
                }

                // ReSharper disable once CppZeroConstantCanBeReplacedWithNullptr
                UnregisterClassA(NOTIFICATION_CLASS_NAME, GetModuleHandleA(NULL));
            }

            void notify(const std::string& title, const std::string& body) override
            {
                if (!m_hwnd)
                {
                    return;
                }

                strncpy_s(m_nid.szInfoTitle, title.c_str(), sizeof(m_nid.szInfoTitle) - 1);
                strncpy_s(m_nid.szInfo, body.c_str(), sizeof(m_nid.szInfo) - 1);
                
                m_nid.dwInfoFlags = NIIF_INFO | NIIF_NOSOUND;
                m_nid.uTimeout = 2000; // 2 seconds (deprecated in newer Windows, but good to have)

                if (m_iconAdded)
                {
                    if (!Shell_NotifyIconA(NIM_MODIFY, &m_nid))
                    {
                        Logger::debug("Failed to modify Windows desktop notification.\n");
                    }
                }
                else
                {
                    if (Shell_NotifyIconA(NIM_ADD, &m_nid))
                    {
                        m_iconAdded = true;
                    }
                    else
                    {
                        Logger::debug("Failed to add Windows desktop notification.\n");
                    }
                }
            }

        private:
            bool initializeWindow()
            {
                // ReSharper disable once CppZeroConstantCanBeReplacedWithNullptr
                HINSTANCE hInst = GetModuleHandleA(NULL);

                WNDCLASSA wc = {};
                wc.lpfnWndProc = NotificationWindowProc;
                wc.hInstance = hInst;
                wc.lpszClassName = NOTIFICATION_CLASS_NAME;

                if (!RegisterClassA(&wc) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
                {
                    Logger::debug("Failed to register notification window class.\n");
                    return false;
                }

                // ReSharper disable CppZeroConstantCanBeReplacedWithNullptr
                m_hwnd = CreateWindowExA(
                    0,
                    NOTIFICATION_CLASS_NAME,
                    "AutoInput Notification",
                    0,
                    0, 0, 0, 0,
                    HWND_MESSAGE,
                    NULL,
                    hInst,
                    NULL
                );
                // ReSharper restore CppZeroConstantCanBeReplacedWithNullptr

                if (!m_hwnd)
                {
                    Logger::debug("Failed to create notification message-only window.\n");
                    return false;
                }

                return true;
            }

            NOTIFYICONDATAA m_nid;
            HWND m_hwnd;
            bool m_iconAdded;
        };
    }

    namespace platform
    {
        std::unique_ptr<INotificationSink> createDesktopNotificationSink()
        {
            return std::make_unique<WindowsDesktopNotificationSink>();
        }
    }
}

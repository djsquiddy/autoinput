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
        class WindowsDesktopNotificationSink : public INotificationSink
        {
        public:
            WindowsDesktopNotificationSink()
            {
                // Initialize NOTIFYICONDATAA
                memset(&m_nid, 0, sizeof(m_nid));
                m_nid.cbSize = sizeof(m_nid);
                m_nid.uID = 1; // Unique ID for our notification
                m_nid.uFlags = NIF_INFO | NIF_GUID;
                
                // Use a stable GUID for this notification to allow replacement/updating
                // {7C8E2D5B-4C9A-4E1B-9A7D-6D4C2F8E1A3B}
                static const GUID AutoInputGuid = 
                    { 0x7c8e2d5b, 0x4c9a, 0x4e1b, { 0x9a, 0x7d, 0x6d, 0x4c, 0x2f, 0x8e, 0x1a, 0x3b } };
                m_nid.guidItem = AutoInputGuid;
            }

            ~WindowsDesktopNotificationSink() override
            {
                // Remove the notification icon if we added one (though we only show balloons)
                Shell_NotifyIconA(NIM_DELETE, &m_nid);
            }

            void notify(const std::string& title, const std::string& body) override
            {
                strncpy_s(m_nid.szInfoTitle, title.c_str(), sizeof(m_nid.szInfoTitle) - 1);
                strncpy_s(m_nid.szInfo, body.c_str(), sizeof(m_nid.szInfo) - 1);
                
                m_nid.dwInfoFlags = NIIF_INFO | NIIF_NOSOUND;
                m_nid.uTimeout = 2000; // 2 seconds (deprecated in newer Windows, but good to have)

                // NIM_MODIFY updates if it exists, NIM_ADD creates. 
                // We try NIM_MODIFY first, if it fails, we try NIM_ADD.
                if (!Shell_NotifyIconA(NIM_MODIFY, &m_nid))
                {
                    if (!Shell_NotifyIconA(NIM_ADD, &m_nid))
                    {
                        Logger::debug("Failed to show Windows desktop notification.\n");
                    }
                }
            }

        private:
            NOTIFYICONDATAA m_nid;
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

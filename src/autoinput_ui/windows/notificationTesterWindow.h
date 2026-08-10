/**
 * @file notificationTesterWindow.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_WINDOWS_NOTIFICATION_TESTER_WINDOW_H
#define INCLUDE_AUTOINPUT_UI_WINDOWS_NOTIFICATION_TESTER_WINDOW_H
#pragma once

#include "../core/uiWindow.h"
#include "autoinput/services/automationRuntimeClient.h"
#include "autoinput/config/settings.h"
#include <string>

namespace autoinput::ui
{
    class NotificationTesterWindow : public UiWindow
    {
    public:
        explicit NotificationTesterWindow(services::IAutomationRuntimeClient& runtimeClient);

    protected:
        void renderContent() override;
        void onOpen() override;

    private:
        void sendNotification(NotificationSeverity severity);

        services::IAutomationRuntimeClient& m_runtimeClient;
        Settings m_settings;
        
        std::string m_testTitle{ "Test Notification" };
        std::string m_testMessage{ "This is a test notification from AutoInput." };
        std::string m_lastResult;
        bool m_lastSuccess{ true };
        
        StatusNotificationMode m_tempMode{ StatusNotificationMode::Desktop };
        bool m_useTempMode{ false };
    };
}

#endif // INCLUDE_AUTOINPUT_UI_WINDOWS_NOTIFICATION_TESTER_WINDOW_H

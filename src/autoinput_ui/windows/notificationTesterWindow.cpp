/**
 * @file notificationTesterWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "notificationTesterWindow.h"
#include "../widgets/basicWidgets.h"
#include "autoinput/logger.h"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <format>

namespace autoinput::ui
{
    NotificationTesterWindow::NotificationTesterWindow(services::IAutomationRuntimeClient& runtimeClient)
        : UiWindow("Notification Tester")
        , m_runtimeClient(runtimeClient)
    {
    }

    void NotificationTesterWindow::onOpen()
    {
        m_settings.load();
        m_tempMode = m_settings.getDefaults().statusNotificationMode.empty() ? StatusNotificationMode::Off : statusNotificationModeFromString(m_settings.getDefaults().statusNotificationMode);
        m_useTempMode = false;
        m_lastResult.clear();
    }

    void NotificationTesterWindow::renderContent()
    {
        ImGui::Text("Current Settings");
        ImGui::Text("Notification Mode: %s", m_settings.getDefaults().statusNotificationMode.c_str());
        
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::Checkbox("Override Mode for Testing", &m_useTempMode);
        if (m_useTempMode)
        {
            const char* modes[] = { "Off", "Console", "Desktop", "Both" };
            int currentMode = 0;
            if (m_tempMode == StatusNotificationMode::Console) currentMode = 1;
            else if (m_tempMode == StatusNotificationMode::Desktop) currentMode = 2;
            else if (m_tempMode == StatusNotificationMode::Both) currentMode = 3;

            if (ImGui::Combo("Test Mode", &currentMode, modes, IM_ARRAYSIZE(modes)))
            {
                if (currentMode == 0) m_tempMode = StatusNotificationMode::Off;
                else if (currentMode == 1) m_tempMode = StatusNotificationMode::Console;
                else if (currentMode == 2) m_tempMode = StatusNotificationMode::Desktop;
                else if (currentMode == 3) m_tempMode = StatusNotificationMode::Both;
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        ImGui::InputText("Title", &m_testTitle);
        ImGui::InputTextMultiline("Message", &m_testMessage, ImVec2(0, 60));

        ImGui::Spacing();

        if (ImGui::Button("Send Info")) sendNotification(NotificationSeverity::Info);
        ImGui::SameLine();
        if (ImGui::Button("Send Success")) sendNotification(NotificationSeverity::Success);
        ImGui::SameLine();
        if (ImGui::Button("Send Warning")) sendNotification(NotificationSeverity::Warning);
        ImGui::SameLine();
        if (ImGui::Button("Send Error")) sendNotification(NotificationSeverity::Error);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (!m_lastResult.empty())
        {
            if (m_lastSuccess)
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Last Result: %s", m_lastResult.c_str());
            else
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Last Result: %s", m_lastResult.c_str());
        }

        ImGui::Spacing();
        
        auto caps = m_runtimeClient.getBackendCapabilities();
        ImGui::Text("Backend: %s", m_runtimeClient.getBackendName().c_str());
        ImGui::Text("Notifications Supported: %s", "Desktop (Win32/Linux)"); // We know we have sinks for these
    }

    void NotificationTesterWindow::sendNotification(NotificationSeverity severity)
    {
        std::optional<StatusNotificationMode> mode;
        if (m_useTempMode)
        {
            mode = m_tempMode;
        }

        auto result = m_runtimeClient.sendTestNotification(m_testTitle, m_testMessage, severity, mode);
        m_lastSuccess = result.success;
        m_lastResult = result.message;
        
        if (m_lastResult.empty())
        {
            m_lastResult = m_lastSuccess ? "Success" : "Failed";
        }
    }
}

/**
 * @file notificationTesterWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "notificationTesterWindow.h"
#include "../widgets/basicWidgets.h"
#include "../core/localization.h"
#include "autoinput/support/logger.h"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <format>

namespace autoinput::ui
{
    NotificationTesterWindow::NotificationTesterWindow(services::IAutomationRuntimeClient& runtimeClient)
        : UiWindow("Notification Tester", "windows.notificationTester")
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
        auto& loc = Localization::get();
        ImGui::Text("%s", loc.text("labels.currentSettings").data());
        ImGui::Text("%s: %s", loc.text("labels.notificationMode").data(), m_settings.getDefaults().statusNotificationMode.c_str());
        
        ImGui::Separator();
        ImGui::Spacing();
 
        ImGui::Checkbox(loc.text("labels.overrideMode").data(), &m_useTempMode);
        if (m_useTempMode)
        {
            const char* modes[] = { loc.text("status.off").data(), loc.text("status.console").data(), loc.text("status.desktop").data(), loc.text("status.both").data() };
            int currentMode = 0;
            if (m_tempMode == StatusNotificationMode::Console) currentMode = 1;
            else if (m_tempMode == StatusNotificationMode::Desktop) currentMode = 2;
            else if (m_tempMode == StatusNotificationMode::Both) currentMode = 3;
 
            if (ImGui::Combo(loc.text("labels.testMode").data(), &currentMode, modes, IM_ARRAYSIZE(modes)))
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
 
        ImGui::InputText(loc.text("labels.title").data(), &m_testTitle);
        ImGui::InputTextMultiline(loc.text("labels.message").data(), &m_testMessage, ImVec2(0, 60));
 
        ImGui::Spacing();
 
        if (ImGui::Button(loc.text("buttons.sendInfo").data())) sendNotification(NotificationSeverity::Info);
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.sendSuccess").data())) sendNotification(NotificationSeverity::Success);
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.sendWarning").data())) sendNotification(NotificationSeverity::Warning);
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.sendError").data())) sendNotification(NotificationSeverity::Error);
 
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
 
        if (!m_lastResult.empty())
        {
            if (m_lastSuccess)
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "%s: %s", loc.text("labels.lastResult").data(), m_lastResult.c_str());
            else
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s: %s", loc.text("labels.lastResult").data(), m_lastResult.c_str());
        }
 
        ImGui::Spacing();
        
        ImGui::Text("%s: %s", loc.text("labels.supportedByBackend").data(), m_runtimeClient.getBackendName().c_str());
        ImGui::Text("%s: %s", loc.text("labels.supportedNotifications").data(), loc.text("labels.supportedPlatforms").data());
    }

    void NotificationTesterWindow::sendNotification(NotificationSeverity severity)
    {
        auto& loc = Localization::get();
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
            m_lastResult = m_lastSuccess ? loc.text("status.success") : loc.text("status.failed");
        }
    }
}

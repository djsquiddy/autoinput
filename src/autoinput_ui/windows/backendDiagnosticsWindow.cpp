/**
 * @file backendDiagnosticsWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "backendDiagnosticsWindow.h"
#include "../widgets/basicWidgets.h"
#include "../core/localization.h"
#include <imgui.h>
#include <thread>
#include <format>
#include <mutex>

namespace autoinput::ui
{
    namespace
    {
        std::mutex g_diagMutex;
    }

    BackendDiagnosticsWindow::BackendDiagnosticsWindow(services::IAutomationRuntimeClient& runtimeClient, const IEnvironment& environment)
        : UiWindow("Backend Diagnostics", "windows.backendDiagnostics"), m_runtimeClient(runtimeClient), m_environment(environment)
    {
    }

    void BackendDiagnosticsWindow::onOpen()
    {
        refreshDiagnostics();
    }

    void BackendDiagnosticsWindow::refreshDiagnostics()
    {
        std::lock_guard lock(g_diagMutex);
        m_platformName = m_environment.platformName();
        m_backendName = m_runtimeClient.getBackendName();
        m_capabilities = m_runtimeClient.getBackendCapabilities();
        m_transportConnected = m_runtimeClient.isBackendAvailable();
        m_lastError = m_runtimeClient.getLastMessage();
    }

    void BackendDiagnosticsWindow::pingRuntime()
    {
        m_pingPending = true;
        m_pingResult = "Pinging...";
        
        std::thread([this]() {
            auto res = m_runtimeClient.ping();
            std::lock_guard lock(g_diagMutex);
            m_pingResult = res.success ? std::format("Success: {}", res.message) : std::format("Failed: {}", res.message);
            m_pingPending = false;
        }).detach();
    }

    void BackendDiagnosticsWindow::sendTestNotification()
    {
        m_notificationPending = true;
        m_notificationResult = "Sending...";
        
        std::thread([this]() {
            auto res = m_runtimeClient.sendTestNotification("AutoInput Diagnostics", "This is a test notification.");
            std::lock_guard lock(g_diagMutex);
            m_notificationResult = res.success ? "Success" : std::format("Failed: {}", res.message);
            m_notificationPending = false;
        }).detach();
    }

    void BackendDiagnosticsWindow::renderContent()
    {
        auto& loc = Localization::get();
        if (ImGui::Button(loc.text("buttons.refresh").data()))
        {
            refreshDiagnostics();
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.pingRuntime").data()) && !m_pingPending)
        {
            pingRuntime();
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.testNotification").data()) && !m_notificationPending)
        {
            sendTestNotification();
        }

        std::lock_guard lock(g_diagMutex);

        if (!m_pingResult.empty())
        {
            ImGui::Text("Ping Result: %s", m_pingResult.c_str());
        }
        if (!m_notificationResult.empty())
        {
            ImGui::Text("Notification Result: %s", m_notificationResult.c_str());
        }

        ImGui::Separator();

        if (ImGui::BeginTable("DiagnosticsTable", 2, ImGuiTableFlags_BordersInnerH))
        {
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s:", loc.text("labels.platform").data());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(m_platformName.c_str());
 
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s:", loc.text("labels.backend").data());
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(m_backendName.c_str());
 
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s:", loc.text("labels.transportConnected").data());
            ImGui::TableNextColumn();
            if (m_transportConnected)
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "%s", loc.text("buttons.yes").data());
            else
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", loc.text("buttons.no").data());
 
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%s:", loc.text("labels.lastMessageError").data());
            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s", m_lastError.empty() ? loc.text("labels.none").data() : m_lastError.c_str());
 
            ImGui::EndTable();
        }
 
        ImGui::Spacing();
        ImGui::Text("%s:", loc.text("labels.backendCapabilities").data());
        ImGui::Separator();
 
        auto renderCapability = [&loc](const char* name, bool supported) {
            ImGui::Text("%s:", name);
            ImGui::SameLine(ImGui::GetWindowWidth() * 0.6f);
            if (supported)
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "%s", loc.text("labels.supported").data());
            else
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", loc.text("labels.notSupported").data());
        };

        renderCapability(loc.text("labels.keyboardInputSupport").data(), m_capabilities.syntheticKeyboardInput);
        renderCapability(loc.text("labels.mouseInputSupport").data(), m_capabilities.syntheticMouseInput);
        renderCapability(loc.text("labels.keyboardHooks").data(), m_capabilities.keyboardHooks);
        renderCapability(loc.text("labels.mouseHooks").data(), m_capabilities.mouseHooks);
        renderCapability(loc.text("labels.focusDetection").data(), m_capabilities.focusDetection);
        renderCapability(loc.text("labels.absoluteMouseMove").data(), m_capabilities.absoluteMouseMovement);
        renderCapability(loc.text("labels.getCursorPos").data(), m_capabilities.getCursorPosition);
        renderCapability(loc.text("labels.listApplications").data(), m_capabilities.listApplications);
        
        renderCapability(loc.text("labels.notificationsSupport").data(), true);
        renderCapability(loc.text("labels.runtimeCommandSupport").data(), true);
    }
}

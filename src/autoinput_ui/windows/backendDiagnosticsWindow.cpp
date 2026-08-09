/**
 * @file backendDiagnosticsWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "backendDiagnosticsWindow.h"
#include "../widgets/basicWidgets.h"
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
        : UiWindow("Backend Diagnostics"), m_runtimeClient(runtimeClient), m_environment(environment)
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
        if (ImGui::Button("Refresh"))
        {
            refreshDiagnostics();
        }
        ImGui::SameLine();
        if (ImGui::Button("Ping Runtime") && !m_pingPending)
        {
            pingRuntime();
        }
        ImGui::SameLine();
        if (ImGui::Button("Test Notification") && !m_notificationPending)
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
            ImGui::Text("Platform:");
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(m_platformName.c_str());

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Backend:");
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(m_backendName.c_str());

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Transport Connected:");
            ImGui::TableNextColumn();
            if (m_transportConnected)
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Yes");
            else
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "No");

            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("Last Message/Error:");
            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s", m_lastError.empty() ? "None" : m_lastError.c_str());

            ImGui::EndTable();
        }

        ImGui::Spacing();
        ImGui::Text("Backend Capabilities:");
        ImGui::Separator();

        auto renderCapability = [](const char* name, bool supported) {
            ImGui::Text("%s:", name);
            ImGui::SameLine(ImGui::GetWindowWidth() * 0.6f);
            if (supported)
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Supported");
            else
                ImGui::TextColored(ImVec4(1, 1, 0, 1), "Not Supported");
        };

        renderCapability("Keyboard Input Support", m_capabilities.syntheticKeyboardInput);
        renderCapability("Mouse Input Support", m_capabilities.syntheticMouseInput);
        renderCapability("Keyboard Hooks (Recording)", m_capabilities.keyboardHooks);
        renderCapability("Mouse Hooks (Recording)", m_capabilities.mouseHooks);
        renderCapability("Focus Detection", m_capabilities.focusDetection);
        renderCapability("Absolute Mouse Movement", m_capabilities.absoluteMouseMovement);
        renderCapability("Get Cursor Position", m_capabilities.getCursorPosition);
        renderCapability("List Applications", m_capabilities.listApplications);
        
        renderCapability("Notifications Support", true);
        renderCapability("Runtime Command Execution Support", true);
    }
}

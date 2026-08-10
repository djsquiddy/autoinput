/**
 * @file runtimeDashboardWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "runtimeDashboardWindow.h"
#include "../widgets/basicWidgets.h"
#include <imgui.h>
#include <string>

namespace autoinput::ui
{
    RuntimeDashboardWindow::RuntimeDashboardWindow(services::IAutomationRuntimeClient& runtimeClient)
        : UiWindow("Runtime Dashboard")
        , m_runtimeClient(runtimeClient)
    {
    }

    void RuntimeDashboardWindow::renderContent()
    {
        using namespace autoinput::services;
        const RuntimeStatus status = m_runtimeClient.getStatus();
        const std::string activeConfig = m_runtimeClient.getActiveConfig();
        const std::string activeCommand = m_runtimeClient.getActiveCommand();
        const std::string lastMessage = m_runtimeClient.getLastMessage();
        const bool backendAvailable = m_runtimeClient.isBackendAvailable();

        // Status Section
        ImGui::Text("Runtime Status:");
        ImGui::SameLine();
        
        std::string displayStatus;
        if (status == RuntimeStatus::Stopped) displayStatus = "Idle";
        else displayStatus = statusToString(status);
        
        widgets::RuntimeStatusIndicator(displayStatus);

        ImGui::Separator();

        // Active State Section
        ImGui::Columns(2, "RuntimeInfoColumns", false);
        ImGui::SetColumnWidth(0, 150.0f);
        
        ImGui::Text("Active Config:");
        ImGui::NextColumn();
        if (activeConfig.empty())
            ImGui::TextDisabled("None");
        else
            ImGui::Text("%s", activeConfig.c_str());
        ImGui::NextColumn();

        ImGui::Text("Active Command:");
        ImGui::NextColumn();
        if (activeCommand.empty())
            ImGui::TextDisabled("None");
        else
            ImGui::Text("%s", activeCommand.c_str());
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();

        // Last Message Section
        ImGui::Text("Last Runtime Message:");
        if (lastMessage.empty())
            ImGui::TextDisabled("No messages yet.");
        else
            ImGui::TextWrapped("%s", lastMessage.c_str());

        ImGui::Separator();

        // Controls Section
        bool canStart = (status == RuntimeStatus::Stopped || status == RuntimeStatus::Error);
        bool canStop = (status == RuntimeStatus::Running || status == RuntimeStatus::Starting || status == RuntimeStatus::Paused);

        ImGui::Text("Controls:");
        
        ImGui::PushItemWidth(200.0f);
        ImGui::InputText("##ConfigInput", m_configInput, sizeof(m_configInput));
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::TextDisabled("(Config Name)");
        
        if (!canStart) ImGui::BeginDisabled();
        if (ImGui::Button("Start"))
        {
            const auto result = m_runtimeClient.start(m_configInput);
            m_lastOperationMessage = result.message;
        }
        if (!canStart) ImGui::EndDisabled();

        ImGui::SameLine();
        if (!canStop) ImGui::BeginDisabled();
        if (ImGui::Button("Stop"))
        {
            auto result = m_runtimeClient.stop();
            m_lastOperationMessage = result.message;
        }
        if (!canStop) ImGui::EndDisabled();

        ImGui::SameLine();
        if (!canStop) ImGui::BeginDisabled(); 
        if (ImGui::Button("Restart"))
        {
            AUTOINPUT_UNUSED(m_runtimeClient.stop());
            const auto result = m_runtimeClient.start(activeConfig.empty() ? m_configInput : activeConfig);
            m_lastOperationMessage = result.message;
        }
        if (!canStop) ImGui::EndDisabled();

        ImGui::SameLine();
        if (ImGui::Button("Refresh"))
        {
            m_lastOperationMessage.clear();
        }

        if (!m_lastOperationMessage.empty())
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "Status: %s", m_lastOperationMessage.c_str());
        }

        ImGui::Separator();

        // Health Section
        ImGui::Text("Runtime Health:");
        ImGui::Indent();
        ImGui::Text("Backend Availability: ");
        ImGui::SameLine();
        widgets::RuntimeStatusIndicator(backendAvailable ? "Healthy" : "Error");
        ImGui::Unindent();
    }
}

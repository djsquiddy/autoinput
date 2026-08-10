/**
 * @file runtimeDashboardWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "runtimeDashboardWindow.h"
#include "../widgets/basicWidgets.h"
#include "../core/localization.h"
#include <imgui.h>
#include <string>

namespace autoinput::ui
{
    RuntimeDashboardWindow::RuntimeDashboardWindow(services::IAutomationRuntimeClient& runtimeClient)
        : UiWindow("Runtime Dashboard", "windows.runtimeDashboard")
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

        auto& loc = Localization::get();
        // Status Section
        ImGui::Text("%s:", loc.text("labels.status").data());
        ImGui::SameLine();
        
        std::string displayStatus;
        if (status == RuntimeStatus::Stopped) displayStatus = "Idle";
        else displayStatus = statusToString(status);
        
        widgets::RuntimeStatusIndicator(displayStatus);

        ImGui::Separator();

        // Active State Section
        ImGui::Columns(2, "RuntimeInfoColumns", false);
        ImGui::SetColumnWidth(0, 150.0f);
        
        ImGui::Text("%s:", loc.text("labels.activeConfig").data());
        ImGui::NextColumn();
        if (activeConfig.empty())
            ImGui::TextDisabled("%s", loc.text("labels.none").data());
        else
            ImGui::Text("%s", activeConfig.c_str());
        ImGui::NextColumn();
 
        ImGui::Text("%s:", loc.text("labels.activeCommand").data());
        ImGui::NextColumn();
        if (activeCommand.empty())
            ImGui::TextDisabled("%s", loc.text("labels.none").data());
        else
            ImGui::Text("%s", activeCommand.c_str());
        ImGui::NextColumn();

        ImGui::Columns(1);
        ImGui::Separator();

        // Last Message Section
        ImGui::Text("%s:", loc.text("labels.lastMessage").data());
        if (lastMessage.empty())
            ImGui::TextDisabled("%s", loc.text("labels.noMessages").data());
        else
            ImGui::TextWrapped("%s", lastMessage.c_str());

        ImGui::Separator();

        // Controls Section
        bool canStart = (status == RuntimeStatus::Stopped || status == RuntimeStatus::Error);
        bool canStop = (status == RuntimeStatus::Running || status == RuntimeStatus::Starting || status == RuntimeStatus::Paused);

        ImGui::Text("%s:", loc.text("labels.actions").data());
        
        ImGui::PushItemWidth(200.0f);
        ImGui::InputText("##ConfigInput", m_configInput, sizeof(m_configInput));
        ImGui::PopItemWidth();
        ImGui::SameLine();
        ImGui::TextDisabled("(%s)", loc.text("labels.name").data());
        
        if (!canStart) ImGui::BeginDisabled();
        if (ImGui::Button(loc.text("buttons.start").data()))
        {
            const auto result = m_runtimeClient.start(m_configInput);
            m_lastOperationMessage = result.message;
        }
        if (!canStart) ImGui::EndDisabled();
 
        ImGui::SameLine();
        if (!canStop) ImGui::BeginDisabled();
        if (ImGui::Button(loc.text("buttons.stop").data()))
        {
            auto result = m_runtimeClient.stop();
            m_lastOperationMessage = result.message;
        }
        if (!canStop) ImGui::EndDisabled();
 
        ImGui::SameLine();
        if (!canStop) ImGui::BeginDisabled(); 
        if (ImGui::Button(loc.text("buttons.restart").data()))
        {
            AUTOINPUT_UNUSED(m_runtimeClient.stop());
            const auto result = m_runtimeClient.start(activeConfig.empty() ? m_configInput : activeConfig);
            m_lastOperationMessage = result.message;
        }
        if (!canStop) ImGui::EndDisabled();
 
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.refresh").data()))
        {
            m_lastOperationMessage.clear();
        }
 
        if (!m_lastOperationMessage.empty())
        {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 1.0f, 1.0f), "%s: %s", loc.text("labels.status").data(), m_lastOperationMessage.c_str());
        }

        ImGui::Separator();

        // Health Section
        ImGui::Text("%s:", loc.text("labels.health").data());
        ImGui::Indent();
        ImGui::Text("%s: ", loc.text("labels.backendAvailability").data());
        ImGui::SameLine();
        widgets::RuntimeStatusIndicator(backendAvailable ? "Healthy" : "Error");
        ImGui::Unindent();
    }
}

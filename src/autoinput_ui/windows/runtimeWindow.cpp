/**
 * @file runtimeWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "runtimeWindow.h"
#include "../core/localization.h"
#include <imgui.h>
#include <format>

namespace autoinput::ui
{
    namespace
    {
        const char* runtimeStatusDisplayName(services::RuntimeStatus status)
        {
            auto& loc = Localization::get();
            switch (status)
            {
            case services::RuntimeStatus::Stopped:
                return loc.text("status.stopped").data();
            case services::RuntimeStatus::Starting:
                return loc.text("status.starting").data();
            case services::RuntimeStatus::Running:
                return loc.text("status.running").data();
            case services::RuntimeStatus::Paused:
                return loc.text("status.paused").data();
            case services::RuntimeStatus::Error:
                return loc.text("status.error").data();
            default: return loc.text("status.unknown").data();
            }
        }
    }

    RuntimeWindow::RuntimeWindow(services::IAutomationRuntimeClient& runtimeClient)
        : UiWindow("Advanced Runtime Control", "windows.advancedRuntime")
        , m_runtimeClient(runtimeClient)
    {
    }

    void RuntimeWindow::renderContent()
    {
        using namespace autoinput::services;
        auto& loc = Localization::get();
 
        ImGui::InputText(loc.text("labels.configName").data(), m_configName, sizeof(m_configName));
 
        RuntimeStatus status = m_runtimeClient.getStatus();
        const char* statusText = runtimeStatusDisplayName(status);
 
        ImGui::Text("%s: %s", loc.text("labels.status").data(), statusText);
 
        bool isRunning = (status == RuntimeStatus::Running || status == RuntimeStatus::Starting);
        bool isStopped = (status == RuntimeStatus::Stopped || status == RuntimeStatus::Error);
 
        if (isRunning) ImGui::BeginDisabled();
        if (ImGui::Button(loc.text("buttons.start").data()))
        {
            auto result = m_runtimeClient.start(m_configName);
            m_lastMessage = result.message;
        }
        if (isRunning) ImGui::EndDisabled();
        
        ImGui::SameLine();
        if (isStopped) ImGui::BeginDisabled();
        if (ImGui::Button(loc.text("buttons.stop").data()))
        {
            auto result = m_runtimeClient.stop();
            m_lastMessage = result.message;
        }
        if (isStopped) ImGui::EndDisabled();
        
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.pause").data()))
        {
            auto result = m_runtimeClient.pause();
            m_lastMessage = result.message;
        }
        
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.resume").data()))
        {
            auto result = m_runtimeClient.resume();
            m_lastMessage = result.message;
        }
 
        if (!m_lastMessage.empty())
        {
            ImGui::Separator();
            ImGui::TextWrapped("%s", loc.format("labels.lastMessageLabel", m_lastMessage).c_str());
        }
    }
}

/**
 * @file runtimeWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "runtimeWindow.h"
#include <imgui.h>

namespace autoinput::ui
{
    namespace
    {
        const char* runtimeStatusDisplayName(services::RuntimeStatus status)
        {
            switch (status)
            {
            case services::RuntimeStatus::Stopped:
                return "Stopped";
            case services::RuntimeStatus::Starting:
                return "Starting";
            case services::RuntimeStatus::Running:
                return "Running";
            case services::RuntimeStatus::Paused:
                return "Paused";
            case services::RuntimeStatus::Error:
                return "Error";
            default: return "Unknown";
            }
        }
    }

    RuntimeWindow::RuntimeWindow(services::IAutomationRuntimeClient& runtimeClient)
        : UiWindow("Advanced Runtime Control")
        , m_runtimeClient(runtimeClient)
    {
    }

    void RuntimeWindow::renderContent()
    {
        using namespace autoinput::services;

        ImGui::InputText("Config Name", m_configName, sizeof(m_configName));

        RuntimeStatus status = m_runtimeClient.getStatus();
        const char* statusText = runtimeStatusDisplayName(status);

        ImGui::Text("Status: %s", statusText);

        bool isRunning = (status == RuntimeStatus::Running || status == RuntimeStatus::Starting);
        bool isStopped = (status == RuntimeStatus::Stopped || status == RuntimeStatus::Error);

        if (isRunning) ImGui::BeginDisabled();
        if (ImGui::Button("Start"))
        {
            auto result = m_runtimeClient.start(m_configName);
            m_lastMessage = result.message;
        }
        if (isRunning) ImGui::EndDisabled();
        
        ImGui::SameLine();
        if (isStopped) ImGui::BeginDisabled();
        if (ImGui::Button("Stop"))
        {
            auto result = m_runtimeClient.stop();
            m_lastMessage = result.message;
        }
        if (isStopped) ImGui::EndDisabled();
        
        ImGui::SameLine();
        if (ImGui::Button("Pause"))
        {
            auto result = m_runtimeClient.pause();
            m_lastMessage = result.message;
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Resume"))
        {
            auto result = m_runtimeClient.resume();
            m_lastMessage = result.message;
        }

        if (!m_lastMessage.empty())
        {
            ImGui::Separator();
            ImGui::TextWrapped("Last message: %s", m_lastMessage.c_str());
        }
    }
}

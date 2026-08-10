/**
 * @file commandRunnerWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "commandRunnerWindow.h"
#include "../widgets/basicWidgets.h"
#include <imgui.h>
#include <algorithm>

namespace autoinput::ui
{
    CommandRunnerWindow::CommandRunnerWindow(services::IAutomationRuntimeClient& runtimeClient, const IEnvironment& environment)
        : UiWindow("Command Runner", "windows.commandRunner"), m_runtimeClient(runtimeClient), m_configService(environment)
    {
        refreshConfigs();
    }

    void CommandRunnerWindow::refreshConfigs()
    {
        m_availableConfigs.clear();
        auto configs = m_configService.listAvailableConfigs();
        for (const auto& config : configs)
        {
            m_availableConfigs.push_back(config.fileStem());
        }
        
        m_selectedConfigIndex = -1;
        m_loadedConfig = ConfigData{};
        m_availableCommands.clear();
        m_selectedCommandIndex = -1;
    }

    void CommandRunnerWindow::loadSelectedConfig()
    {
        if (m_selectedConfigIndex < 0 || m_selectedConfigIndex >= static_cast<int>(m_availableConfigs.size()))
        {
            return;
        }

        try
        {
            auto configPath = autoinput::getConfigFilePath(m_availableConfigs[m_selectedConfigIndex]);
            auto data = autoinput::loadConfigData(configPath);
            if (data)
            {
                m_loadedConfig = *data;
                m_availableCommands.clear();
                
                for (const auto& cmd : m_loadedConfig.commands)
                {
                    m_availableCommands.push_back(cmd.name);
                }
                for (const auto& seq : m_loadedConfig.sequences)
                {
                    m_availableCommands.push_back(seq.name);
                }
                
                m_selectedCommandIndex = -1;
                m_statusMessage = "Config loaded successfully.";
                m_isError = false;
            }
            else
            {
                m_statusMessage = "Failed to load config data.";
                m_isError = true;
            }
        }
        catch (const std::exception& e)
        {
            m_statusMessage = std::format("Failed to load config: {}", e.what());
            m_isError = true;
        }
    }

    void CommandRunnerWindow::renderContent()
    {
        // Config Selection
        ImGui::Text("Configuration:");
        
        if (ImGui::Button("Reload Configs"))
        {
            refreshConfigs();
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Load Config"))
        {
            loadSelectedConfig();
        }

        const char* currentConfig = (m_selectedConfigIndex >= 0) ? m_availableConfigs[m_selectedConfigIndex].c_str() : "Select a config...";
        if (ImGui::BeginCombo("##ConfigCombo", currentConfig))
        {
            for (int i = 0; i < static_cast<int>(m_availableConfigs.size()); ++i)
            {
                bool isSelected = (m_selectedConfigIndex == i);
                if (ImGui::Selectable(m_availableConfigs[i].c_str(), isSelected))
                {
                    m_selectedConfigIndex = i;
                    loadSelectedConfig();
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Separator();

        // Command Selection
        ImGui::Text("Command:");
        
        const char* currentCommand = (m_selectedCommandIndex >= 0) ? m_availableCommands[m_selectedCommandIndex].c_str() : "Select a command...";
        bool hasCommands = !m_availableCommands.empty();
        
        if (!hasCommands) ImGui::BeginDisabled();
        if (ImGui::BeginCombo("##CommandCombo", currentCommand))
        {
            for (int i = 0; i < static_cast<int>(m_availableCommands.size()); ++i)
            {
                bool isSelected = (m_selectedCommandIndex == i);
                if (ImGui::Selectable(m_availableCommands[i].c_str(), isSelected))
                {
                    m_selectedCommandIndex = i;
                }
                if (isSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        if (!hasCommands) ImGui::EndDisabled();

        ImGui::Separator();

        // Details Panel
        ImGui::Text("Selected Command Details:");
        ImGui::BeginChild("DetailsPanel", ImVec2(0, 150), true);
        if (m_selectedCommandIndex >= 0)
        {
            std::string cmdName = m_availableCommands[m_selectedCommandIndex];
            ImGui::Text("Name: %s", cmdName.c_str());
            
            // Find in commands or sequences
            auto itCmd = std::find_if(m_loadedConfig.commands.begin(), m_loadedConfig.commands.end(), 
                [&](const auto& c) { return c.name == cmdName; });
            
            if (itCmd != m_loadedConfig.commands.end())
            {
                ImGui::Text("Type: Single Command");
                ImGui::Text("Press Wait: %s", itCmd->pressWait.c_str());
                ImGui::Text("Release Wait: %s", itCmd->releaseWait.c_str());
                std::string triggers = "";
                for (const auto& k : itCmd->startKeys) triggers += k + " ";
                ImGui::Text("Trigger: %s", triggers.c_str());
            }
            else
            {
                auto itSeq = std::find_if(m_loadedConfig.sequences.begin(), m_loadedConfig.sequences.end(), 
                    [&](const auto& s) { return s.name == cmdName; });
                if (itSeq != m_loadedConfig.sequences.end())
                {
                    ImGui::Text("Type: Sequence");
                    ImGui::Text("Events: %zu", itSeq->events.size());
                    ImGui::Text("Trigger: %s", itSeq->start.c_str());
                }
            }
        }
        else
        {
            ImGui::TextDisabled("No command selected.");
        }
        ImGui::EndChild();

        // Preview Section
        if (ImGui::CollapsingHeader("Preview"))
        {
            if (m_selectedCommandIndex >= 0 && m_selectedConfigIndex >= 0)
            {
                ImGui::Text("Will execute command '%s'", m_availableCommands[m_selectedCommandIndex].c_str());
                ImGui::Text("from configuration '%s'.", m_availableConfigs[m_selectedConfigIndex].c_str());
            }
            else
            {
                ImGui::TextDisabled("Select config and command to see preview.");
            }
        }

        ImGui::Separator();

        // Actions
        bool canRun = (m_selectedConfigIndex >= 0 && m_selectedCommandIndex >= 0);
        
        if (!canRun) ImGui::BeginDisabled();
        if (ImGui::Button("Run Command", ImVec2(120, 0)))
        {
            std::string configName = m_availableConfigs[m_selectedConfigIndex];
            std::string commandName = m_availableCommands[m_selectedCommandIndex];
            
            auto result = m_runtimeClient.runCommand(configName, commandName);
            m_statusMessage = result.message;
            m_isError = !result.success;
        }
        if (!canRun) ImGui::EndDisabled();

        ImGui::SameLine();
        
        if (ImGui::Button("Stop / Emergency Stop", ImVec2(180, 0)))
        {
            auto result = m_runtimeClient.stop();
            m_statusMessage = result.message;
            m_isError = !result.success;
        }

        // Status
        if (!m_statusMessage.empty())
        {
            ImGui::Spacing();
            if (m_isError)
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Status: %s", m_statusMessage.c_str());
            else
                ImGui::Text("Status: %s", m_statusMessage.c_str());
        }
    }
}

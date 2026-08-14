/**
 * @file commandRunnerWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "commandRunnerWindow.h"
#include "../widgets/basicWidgets.h"
#include "../core/localization.h"
#include <imgui.h>
#include <algorithm>

namespace autoinput::ui
{
    CommandRunnerWindow::CommandRunnerWindow(services::IAutomationRuntimeClient& runtimeClient, const IEnvironment& environment)
        : UiWindow("Command Runner", "windows.commandRunner"), m_runtimeClient(runtimeClient), m_configService(environment)
    {
        refreshConfigs();
    }

    void CommandRunnerWindow::onOpen()
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
                m_statusMessage = Localization::get().text("status.configLoaded");
                m_isError = false;
            }
            else
            {
                m_statusMessage = Localization::get().text("status.failedToLoadConfig");
                m_isError = true;
            }
        }
        catch (const std::exception& e)
        {
            m_statusMessage = Localization::get().format("status.failedToLoadConfigWithReason", e.what());
            m_isError = true;
        }
    }

    void CommandRunnerWindow::renderContent()
    {
        auto& loc = Localization::get();
        // Config Selection
        ImGui::Text("%s:", loc.text("labels.configuration").data());
        
        if (ImGui::Button(loc.text("buttons.reloadConfigs").data()))
        {
            refreshConfigs();
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button(loc.text("buttons.loadConfig").data()))
        {
            loadSelectedConfig();
        }
 
        const char* currentConfig = (m_selectedConfigIndex >= 0) ? m_availableConfigs[m_selectedConfigIndex].c_str() : loc.text("labels.selectConfig").data();
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
        ImGui::Text("%s:", loc.text("labels.command").data());
        
        const char* currentCommand = (m_selectedCommandIndex >= 0) ? m_availableCommands[m_selectedCommandIndex].c_str() : loc.text("labels.selectCommand").data();
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
        ImGui::Text("%s %s:", loc.text("labels.selected").data(), loc.text("labels.details").data());
        ImGui::BeginChild("DetailsPanel", ImVec2(0, 150), true);
        if (m_selectedCommandIndex >= 0)
        {
            std::string cmdName = m_availableCommands[m_selectedCommandIndex];
            ImGui::Text("%s: %s", loc.text("labels.name").data(), cmdName.c_str());
            
            // Find in commands or sequences
            auto itCmd = std::find_if(m_loadedConfig.commands.begin(), m_loadedConfig.commands.end(), 
                [&](const auto& c) { return c.name == cmdName; });
            
            if (itCmd != m_loadedConfig.commands.end())
            {
                ImGui::Text("%s: %s", loc.text("labels.type").data(), loc.text("labels.singleCommand").data());
                ImGui::Text("%s: %s", loc.text("labels.pressWait").data(), itCmd->pressWait.c_str());
                ImGui::Text("%s: %s", loc.text("labels.releaseWait").data(), itCmd->releaseWait.c_str());
                std::string triggers = "";
                for (const auto& k : itCmd->startKeys) triggers += k + " ";
                ImGui::Text("%s: %s", loc.text("labels.trigger").data(), triggers.c_str());
            }
            else
            {
                auto itSeq = std::find_if(m_loadedConfig.sequences.begin(), m_loadedConfig.sequences.end(), 
                    [&](const auto& s) { return s.name == cmdName; });
                if (itSeq != m_loadedConfig.sequences.end())
                {
                    ImGui::Text("%s: %s", loc.text("labels.type").data(), loc.text("labels.sequence").data());
                    ImGui::Text("%s: %zu", loc.text("labels.events").data(), itSeq->events.size());
                    ImGui::Text("%s: %s", loc.text("labels.trigger").data(), itSeq->start.c_str());
                }
            }
        }
        else
        {
            ImGui::TextDisabled("%s", loc.text("labels.noCommandSelected").data());
        }
        ImGui::EndChild();
 
        // Preview Section
        if (ImGui::CollapsingHeader(loc.text("labels.preview").data()))
        {
            if (m_selectedCommandIndex >= 0 && m_selectedConfigIndex >= 0)
            {
                ImGui::Text("%s", loc.format("labels.willExecuteCommand", m_availableCommands[m_selectedCommandIndex]).c_str());
                ImGui::Text("%s", loc.format("labels.fromConfiguration", m_availableConfigs[m_selectedConfigIndex]).c_str());
            }
            else
            {
                ImGui::TextDisabled("%s", loc.text("labels.selectConfigAndCommandForPreview").data());
            }
        }

        ImGui::Separator();

        // Actions
        bool canRun = (m_selectedConfigIndex >= 0 && m_selectedCommandIndex >= 0);
        
        if (!canRun) ImGui::BeginDisabled();
        if (ImGui::Button(loc.text("buttons.runCommand").data(), ImVec2(120, 0)))
        {
            std::string configName = m_availableConfigs[m_selectedConfigIndex];
            std::string commandName = m_availableCommands[m_selectedCommandIndex];
            
            auto result = m_runtimeClient.runCommand(configName, commandName);
            m_statusMessage = result.message;
            m_isError = !result.success;
        }
        if (!canRun) ImGui::EndDisabled();
 
        ImGui::SameLine();
        
        if (ImGui::Button(loc.text("buttons.emergencyStop").data(), ImVec2(180, 0)))
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
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s: %s", loc.text("labels.status").data(), m_statusMessage.c_str());
            else
                ImGui::Text("%s: %s", loc.text("labels.status").data(), m_statusMessage.c_str());
        }
    }
}

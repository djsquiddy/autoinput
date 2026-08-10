/**
 * @file commandPaletteWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "commandPaletteWindow.h"
#include "../core/windowManager.h"
#include "autoinput/logger.h"
#include <imgui.h>
#include <algorithm>
#include <string>
#include <format>

namespace autoinput::ui
{
    CommandPaletteWindow::CommandPaletteWindow(WindowManager& windowManager)
        : UiWindow("Command Palette")
        , m_windowManager(windowManager)
    {
        registerCommands();
    }

    void CommandPaletteWindow::onOpen()
    {
        m_filter[0] = '\0';
        m_selectedIndex = 0;
        m_focusSearch = true;
    }

    int CommandPaletteWindow::getFlags() const
    {
        return ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
               ImGuiWindowFlags_AlwaysAutoResize;
    }

    void CommandPaletteWindow::registerCommands()
    {
        m_commands.clear();
        
        // Window commands
        m_commands.push_back({ "Open Config Editor", "Window", [this] { m_windowManager.open("config-editor"); } });
        m_commands.push_back({ "Open Settings", "Window", [this] { m_windowManager.open("settings"); } });
        m_commands.push_back({ "Open Logs", "Window", [this] { m_windowManager.open("logs"); } });
        m_commands.push_back({ "Open About", "Window", [this] { m_windowManager.open("about"); } });
        m_commands.push_back({ "Open Config Manager", "Window", [this] { m_windowManager.open("config-manager"); } });
        m_commands.push_back({ "Open Hotkey Manager", "Window", [this] { m_windowManager.open("hotkey-manager"); } });
        m_commands.push_back({ "Open Application Picker", "Window", [this] { m_windowManager.open("application-picker"); } });
        m_commands.push_back({ "Open Validation Report", "Window", [this] { m_windowManager.open("validation-report"); } });
        m_commands.push_back({ "Open Setup Wizard", "Window", [this] { m_windowManager.open("setup-wizard"); } });

        // Runtime commands
        m_commands.push_back({ "Open Runtime Dashboard", "Runtime", [this] { m_windowManager.open("runtime-dashboard"); } });
        m_commands.push_back({ "Open Command Runner", "Runtime", [this] { m_windowManager.open("command-runner"); } });
        m_commands.push_back({ "Open Sequence Recorder", "Runtime", [this] { m_windowManager.open("sequence-recorder"); } });
        m_commands.push_back({ "Open Backend Diagnostics", "Runtime", [this] { m_windowManager.open("backend-diagnostics"); } });
        m_commands.push_back({ "Open Notification Tester", "Runtime", [this] { m_windowManager.open("notification-tester"); } });

        // Config commands
        m_commands.push_back({ "Validate All Configs", "Config", [this] { 
            m_windowManager.open("validation-report");
        } });
    }

    void CommandPaletteWindow::renderContent()
    {
        // Center window (roughly)
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetWindowPos(ImVec2(viewport->WorkPos.x + viewport->WorkSize.x * 0.5f - 300.0f, viewport->WorkPos.y + viewport->WorkSize.y * 0.2f), ImGuiCond_Appearing);
        ImGui::SetWindowSize(ImVec2(600, 0), ImGuiCond_Always);

        if (m_focusSearch)
        {
            ImGui::SetKeyboardFocusHere();
            m_focusSearch = false;
        }

        ImGui::PushItemWidth(-1);
        if (ImGui::InputText("##Filter", m_filter, IM_ARRAYSIZE(m_filter)))
        {
            m_selectedIndex = 0;
        }
        ImGui::PopItemWidth();

        std::string filterLower = m_filter;
        std::transform(filterLower.begin(), filterLower.end(), filterLower.begin(), ::tolower);

        std::vector<const PaletteCommand*> filtered;
        for (const auto& cmd : m_commands)
        {
            std::string labelLower = cmd.label;
            std::transform(labelLower.begin(), labelLower.end(), labelLower.begin(), ::tolower);
            
            std::string catLower = cmd.category;
            std::transform(catLower.begin(), catLower.end(), catLower.begin(), ::tolower);

            if (filterLower.empty() || labelLower.find(filterLower) != std::string::npos || catLower.find(filterLower) != std::string::npos)
            {
                filtered.push_back(&cmd);
            }
        }

        if (filtered.empty())
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No matching commands found.");
        }
        else
        {
            if (m_selectedIndex < 0) m_selectedIndex = 0;
            if (m_selectedIndex >= (int)filtered.size()) m_selectedIndex = (int)filtered.size() - 1;

            if (ImGui::IsKeyPressed(ImGuiKey_UpArrow)) m_selectedIndex--;
            if (ImGui::IsKeyPressed(ImGuiKey_DownArrow)) m_selectedIndex++;
            if (m_selectedIndex < 0) m_selectedIndex = (int)filtered.size() - 1;
            if (m_selectedIndex >= (int)filtered.size()) m_selectedIndex = 0;

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::BeginChild("ResultList", ImVec2(0, 300), false);

            std::string currentCategory = "";
            for (int i = 0; i < (int)filtered.size(); ++i)
            {
                const auto* cmd = filtered[i];
                
                if (cmd->category != currentCategory)
                {
                    if (i > 0) ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.4f, 0.7f, 1.0f, 1.0f), "[ %s ]", cmd->category.c_str());
                    currentCategory = cmd->category;
                }

                bool isSelected = (i == m_selectedIndex);
                if (ImGui::Selectable(cmd->label.c_str(), isSelected))
                {
                    executeCommand(*cmd);
                }

                if (isSelected && (ImGui::IsKeyPressed(ImGuiKey_Enter) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter)))
                {
                    executeCommand(*cmd);
                }
            }
            ImGui::EndChild();
        }

        if (ImGui::IsKeyPressed(ImGuiKey_Escape))
        {
            requestClose();
        }

        // Close if focus is lost (clicking outside)
        if (!ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows) && ImGui::IsMouseClicked(0))
        {
             requestClose();
        }
    }

    void CommandPaletteWindow::executeCommand(const PaletteCommand& cmd)
    {
        Logger::info(std::format("Executing command: {}", cmd.label));
        cmd.action();
        requestClose();
    }
}

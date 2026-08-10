/**
 * @file commandPaletteWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "commandPaletteWindow.h"
#include "../core/windowManager.h"
#include "../core/windowIds.h"
#include "../core/uiActions.h"
#include "../core/localization.h"
#include "autoinput/support/logger.h"
#include <imgui.h>
#include <algorithm>
#include <string>
#include <format>

namespace autoinput::ui
{
    CommandPaletteWindow::CommandPaletteWindow(WindowManager& windowManager)
        : UiWindow("Command Palette", "windows.commandPalette")
        , m_windowManager(windowManager)
    {
        refreshActions();
    }

    void CommandPaletteWindow::onOpen()
    {
        m_filter[0] = '\0';
        m_selectedIndex = 0;
        m_focusSearch = true;
        refreshActions();
    }

    int CommandPaletteWindow::getFlags() const
    {
        return ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse |
               ImGuiWindowFlags_AlwaysAutoResize;
    }

    void CommandPaletteWindow::refreshActions()
    {
        auto& loc = Localization::get();
        m_commands.clear();
        auto actions = UiActionRegistry::getActions();
        for (const auto& action : actions)
        {
            m_commands.push_back({ std::string(loc.text(action.labelKey)), std::string(loc.text(action.categoryKey)), [this, id = action.id, labelKey = action.labelKey] { 
                auto& l = Localization::get();
                Logger::info(std::format("Executing action: {}", l.text(labelKey)));
                UiActionRegistry::execute(id, m_windowManager); 
            } });
        }
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
        cmd.action();
        requestClose();
    }
}

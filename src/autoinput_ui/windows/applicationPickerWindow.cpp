/**
 * @file applicationPickerWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "applicationPickerWindow.h"
#include "../widgets/basicWidgets.h"
#include "../core/localization.h"
#include "autoinput/support/logger.h"
#include "autoinput/services/configService.h"
#include "../core/windowManager.h"
#include "configEditorWindow.h"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <format>
#include <algorithm>

namespace autoinput::ui
{
    ApplicationPickerWindow::ApplicationPickerWindow(WindowManager& windowManager, services::IAutomationRuntimeClient& runtimeClient, const IEnvironment& environment)
        : UiWindow("Application Picker", "windows.applicationPicker")
        , m_windowManager(windowManager)
        , m_runtimeClient(runtimeClient)
        , m_environment(environment)
    {
        refreshWindows();
    }

    void ApplicationPickerWindow::refreshWindows()
    {
        auto& loc = Localization::get();
        m_windows = m_runtimeClient.enumerateWindows();
        m_foregroundWindow = m_runtimeClient.getForegroundWindow();
        m_selectedWindowIndex = -1;
        m_statusMessage = loc.format("status.foundWindows", m_windows.size());
    }

    void ApplicationPickerWindow::update()
    {
        // Occasional background refresh could be added here if needed.
    }

    void ApplicationPickerWindow::renderContent()
    {
        auto& loc = Localization::get();
        // Toolbar
        if (ImGui::Button(loc.text("buttons.refresh").data()))
        {
            refreshWindows();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200.0f);
        if (ImGui::InputText(loc.text("labels.search").data(), &m_searchText))
        {
            m_selectedWindowIndex = -1;
        }
        
        ImGui::Separator();
        
        // Foreground window info
        if (m_foregroundWindow)
        {
            ImGui::Text("%s: ", loc.text("labels.foregroundWindow").data());
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "%s", m_foregroundWindow->processName.c_str());
            ImGui::SameLine();
            ImGui::Text("(%s)", m_foregroundWindow->windowTitle.c_str());
            
            ImGui::SameLine();
            if (ImGui::SmallButton(loc.text("buttons.selectForeground").data()))
            {
                for (size_t i = 0; i < m_windows.size(); ++i)
                {
                    if (m_windows[i].backendId == m_foregroundWindow->backendId)
                    {
                        m_selectedWindowIndex = static_cast<int>(i);
                        break;
                    }
                }
            }
        }
        else
        {
            ImGui::Text("%s: %s / %s", loc.text("labels.foregroundWindow").data(), loc.text("status.unknown").data(), loc.text("labels.notSupported").data());
        }
        
        ImGui::Separator();

        // Capabilities check
        auto caps = m_runtimeClient.getBackendCapabilities();
        if (!caps.listApplications)
        {
            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "%s", loc.text("status.backendListingWarning").data());
        }

        // Table
        const float tableHeight = ImGui::GetContentRegionAvail().y - 150.0f;
        if (ImGui::BeginTable("WindowTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0, std::max(100.0f, tableHeight))))
        {
            ImGui::TableSetupColumn(loc.text("labels.process").data(), ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn(loc.text("labels.title").data(), ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn(loc.text("labels.pid").data(), ImGuiTableColumnFlags_WidthFixed, 70.0f);
            ImGui::TableSetupColumn(loc.text("labels.backendId").data(), ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn(loc.text("labels.path").data(), ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();
            
            for (int i = 0; i < static_cast<int>(m_windows.size()); ++i)
            {
                const auto& win = m_windows[i];
                
                if (!m_searchText.empty())
                {
                    auto toLower = [](std::string s) {
                        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
                        return s;
                    };
                    std::string searchLower = toLower(m_searchText);
                    if (toLower(win.processName).find(searchLower) == std::string::npos &&
                        toLower(win.windowTitle).find(searchLower) == std::string::npos)
                    {
                        continue;
                    }
                }

                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                
                bool isSelected = (m_selectedWindowIndex == i);
                std::string label = std::format("{}##{}", win.processName, i);
                if (ImGui::Selectable(label.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
                {
                    m_selectedWindowIndex = i;
                }
                
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(win.windowTitle.c_str());
                
                ImGui::TableNextColumn();
                ImGui::Text("%llu", win.pid);
                
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(win.backendId.c_str());
                
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(win.executablePath.c_str());
            }
            ImGui::EndTable();
        }
        
        ImGui::Separator();
        
        // Actions
        if (m_selectedWindowIndex >= 0 && m_selectedWindowIndex < static_cast<int>(m_windows.size()))
        {
            const auto& selected = m_windows[m_selectedWindowIndex];
            
            if (ImGui::Button(loc.text("buttons.useAsTarget").data())) useAsTarget();
            ImGui::SameLine();
            if (ImGui::Button(loc.text("buttons.addToGlobalBlacklist").data())) addToGlobalBlacklist();
            ImGui::SameLine();
            if (ImGui::Button(loc.text("buttons.addToConfigBlacklist").data())) addToCurrentConfigBlacklist();
            ImGui::SameLine();
            if (ImGui::Button(loc.text("buttons.copyId").data())) copyIdentifier();
            ImGui::SameLine();
            if (ImGui::Button(loc.text("buttons.copyTitle").data())) copyWindowTitle();
            
            ImGui::Spacing();
            ImGui::Text("%s: %s", loc.text("labels.selectedApplication").data(), selected.processName.c_str());
            ImGui::Text("%s: %s", loc.text("labels.selectedWindow").data(), selected.windowTitle.c_str());
            
            if (ImGui::Button(loc.text("buttons.testMatchRules").data())) testMatch();
            if (!m_matchResult.empty())
            {
                ImGui::SameLine();
                ImGui::TextColored(m_matchSuccess ? ImVec4(0.4f, 1.0f, 0.4f, 1.0f) : ImVec4(1.0f, 1.0f, 0.4f, 1.0f), "%s: %s", loc.text("labels.result").data(), m_matchResult.c_str());
            }
        }
        else
        {
            ImGui::Text("%s", loc.text("status.selectWindowToPerformActions").data());
        }
        
        if (!m_statusMessage.empty())
        {
            ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 30.0f);
            ImGui::Separator();
            ImGui::TextUnformatted(m_statusMessage.c_str());
        }
    }

    void ApplicationPickerWindow::useAsTarget()
    {
        auto& loc = Localization::get();
        if (m_selectedWindowIndex < 0) return;
        const auto& win = m_windows[m_selectedWindowIndex];
        ImGui::SetClipboardText(win.processName.c_str());
        m_statusMessage = loc.format("status.copiedToClipboardTarget", win.processName);
    }
 
    void ApplicationPickerWindow::addToGlobalBlacklist()
    {
        auto& loc = Localization::get();
        if (m_selectedWindowIndex < 0) return;
        const auto& win = m_windows[m_selectedWindowIndex];
        ImGui::SetClipboardText(win.processName.c_str());
        m_statusMessage = loc.format("status.copiedToClipboardGlobalBlacklist", win.processName);
    }
 
    void ApplicationPickerWindow::addToCurrentConfigBlacklist()
    {
        auto& loc = Localization::get();
        if (m_selectedWindowIndex < 0) return;
        const auto& win = m_windows[m_selectedWindowIndex];
        ImGui::SetClipboardText(win.processName.c_str());
        m_statusMessage = loc.format("status.copiedToClipboardConfigBlacklist", win.processName);
    }
 
    void ApplicationPickerWindow::copyIdentifier()
    {
        auto& loc = Localization::get();
        if (m_selectedWindowIndex < 0) return;
        ImGui::SetClipboardText(m_windows[m_selectedWindowIndex].backendId.c_str());
        m_statusMessage = loc.text("status.backendIdCopied");
    }
 
    void ApplicationPickerWindow::copyWindowTitle()
    {
        auto& loc = Localization::get();
        if (m_selectedWindowIndex < 0) return;
        ImGui::SetClipboardText(m_windows[m_selectedWindowIndex].windowTitle.c_str());
        m_statusMessage = loc.text("status.windowTitleCopied");
    }
 
    void ApplicationPickerWindow::testMatch()
    {
        auto& loc = Localization::get();
        if (m_selectedWindowIndex < 0) return;
        const auto& win = m_windows[m_selectedWindowIndex];
        
        // Very basic placeholder match logic
        m_matchSuccess = true;
        m_matchResult = loc.format("status.matchResultSuccess", win.processName);
    }
}

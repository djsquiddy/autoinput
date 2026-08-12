/**
 * @file mainWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "mainWindow.h"
#include "../core/windowManager.h"
#include "../core/windowIds.h"
#include "../core/uiActions.h"
#include "../core/localization.h"
#include <imgui.h>

namespace autoinput::ui
{
    MainWindow::MainWindow(WindowManager& windows)
        : UiWindow("AutoInput Main", "windows.main"), m_windows(windows)
    {
        setFullscreen(true);
    }

    int MainWindow::getFlags() const
    {
        return ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
               ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar |
               ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus;
    }

    void MainWindow::renderContent()
    {
        auto& loc = Localization::get();

        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu(loc.text("menus.file").data()))
            {
                if (ImGui::MenuItem(loc.text("buttons.exit").data(), "Alt+F4"))
                {
                    m_shouldExit = true;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu(loc.text("menus.edit").data()))
            {
                if (ImGui::MenuItem(loc.text("actions.openCommandPalette").data(), "Ctrl+P"))
                {
                    m_windows.open(WindowIds::CommandPalette);
                }
                ImGui::Separator();
                
                auto actions = UiActionRegistry::getActions();
                for (const auto& action : actions)
                {
                    if (ImGui::MenuItem(loc.text(action.labelKey).data()))
                    {
                        UiActionRegistry::execute(action.id, m_windows);
                    }
                }
                
                ImGui::Separator();
                if (ImGui::MenuItem("ImGui Demo", nullptr, m_showDemoWindow))
                {
                    m_showDemoWindow = !m_showDemoWindow;
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::Text("%s", loc.text("app.name").data());
        ImGui::Separator();

        // Quick Actions
        if (ImGui::Button(loc.text("windows.commandRunner").data(), ImVec2(150, 40)))
        {
            m_windows.open(WindowIds::CommandRunner);
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.text("windows.runtimeDashboard").data(), ImVec2(150, 40)))
        {
            m_windows.open(WindowIds::RuntimeDashboard);
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.text("windows.sequenceRecorder").data(), ImVec2(150, 40)))
        {
            m_windows.open(WindowIds::SequenceRecorder);
        }

        ImGui::Spacing();
        ImGui::Text("%s", loc.text("actionCategories.tools").data());
        if (ImGui::Button(loc.text("windows.configManager").data())) m_windows.open(WindowIds::ConfigManager);
        ImGui::SameLine();
        if (ImGui::Button(loc.text("windows.settings").data())) m_windows.open(WindowIds::Settings);
        ImGui::SameLine();
        if (ImGui::Button(loc.text("windows.logs").data())) m_windows.open(WindowIds::Logs);
        ImGui::SameLine();
        if (ImGui::Button(loc.text("windows.backendDiagnostics").data())) m_windows.open(WindowIds::BackendDiagnostics);

        ImGui::Separator();
        ImGui::Text("%s:", loc.text("status.ready").data());
        ImGui::BeginChild("StatusRegion", ImVec2(0, 0), true);
        ImGui::TextUnformatted("Application initialized.");
        ImGui::Text("Press Ctrl+P to open the %s.", loc.text("actions.openCommandPalette").data());
        ImGui::EndChild();

        if (m_showDemoWindow)
        {
            ImGui::ShowDemoWindow(&m_showDemoWindow);
        }
    }
}

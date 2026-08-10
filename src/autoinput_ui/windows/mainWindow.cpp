/**
 * @file mainWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "mainWindow.h"
#include "../core/windowManager.h"
#include "../core/windowIds.h"
#include "../core/uiActions.h"
#include <imgui.h>

namespace autoinput::ui
{
    MainWindow::MainWindow(WindowManager& windows)
        : UiWindow("AutoInput Main"), m_windows(windows)
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
        if (ImGui::BeginMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("Exit", "Alt+F4"))
                {
                    m_shouldExit = true;
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Command Palette", "Ctrl+P"))
                {
                    m_windows.open(WindowIds::CommandPalette);
                }
                ImGui::Separator();
                
                auto actions = UiActionRegistry::getActions();
                for (const auto& action : actions)
                {
                    if (ImGui::MenuItem(action.label.c_str()))
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

        ImGui::Text("AutoInput UI");
        ImGui::Separator();

        // Quick Actions
        if (ImGui::Button("Run Command", ImVec2(150, 40)))
        {
            m_windows.open(WindowIds::CommandRunner);
        }
        ImGui::SameLine();
        if (ImGui::Button("Runtime Status", ImVec2(150, 40)))
        {
            m_windows.open(WindowIds::RuntimeDashboard);
        }
        ImGui::SameLine();
        if (ImGui::Button("Record Sequence", ImVec2(150, 40)))
        {
            m_windows.open(WindowIds::SequenceRecorder);
        }

        ImGui::Spacing();
        ImGui::Text("Shortcuts");
        if (ImGui::Button("Config Manager")) m_windows.open(WindowIds::ConfigManager);
        ImGui::SameLine();
        if (ImGui::Button("Settings")) m_windows.open(WindowIds::Settings);
        ImGui::SameLine();
        if (ImGui::Button("Logs")) m_windows.open(WindowIds::Logs);
        ImGui::SameLine();
        if (ImGui::Button("Diagnostics")) m_windows.open(WindowIds::BackendDiagnostics);

        ImGui::Separator();
        ImGui::Text("Information:");
        ImGui::BeginChild("StatusRegion", ImVec2(0, 0), true);
        ImGui::TextUnformatted("Application initialized.");
        ImGui::TextUnformatted("Press Ctrl+P to open the Command Palette.");
        ImGui::EndChild();

        if (m_showDemoWindow)
        {
            ImGui::ShowDemoWindow(&m_showDemoWindow);
        }
    }
}

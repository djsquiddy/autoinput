/**
 * @file mainWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "mainWindow.h"
#include "../core/windowManager.h"
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
                if (ImGui::MenuItem("Settings"))
                {
                    m_windows.open("settings");
                }
                if (ImGui::MenuItem("Config Editor"))
                {
                    m_windows.open("config-editor");
                }
                if (ImGui::MenuItem("Config Manager"))
                {
                    m_windows.open("config-manager");
                }
                if (ImGui::MenuItem("Automation Runtime"))
                {
                    m_windows.open("runtime");
                }
                if (ImGui::MenuItem("Runtime Dashboard"))
                {
                    m_windows.open("runtime-dashboard");
                }
                if (ImGui::MenuItem("Command Runner"))
                {
                    m_windows.open("command-runner");
                }
                if (ImGui::MenuItem("Logs"))
                {
                    m_windows.open("logs");
                }
                if (ImGui::MenuItem("Backend Diagnostics"))
                {
                    m_windows.open("backend-diagnostics");
                }
                if (ImGui::MenuItem("Sequence Recorder"))
                {
                    m_windows.open("sequence-recorder");
                }
                if (ImGui::MenuItem("Sequence Editor"))
                {
                    m_windows.open("sequence-editor");
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
        ImGui::Text("Status: %s", m_statusText.c_str());
        ImGui::Separator();

        if (ImGui::Button("Run"))
        {
            m_statusText = "Running...";
        }
        ImGui::SameLine();
        if (ImGui::Button("Stop"))
        {
            m_statusText = "Stopped";
        }
        ImGui::SameLine();
        if (ImGui::Button("Record"))
        {
            m_statusText = "Recording...";
        }
        ImGui::SameLine();
        if (ImGui::Button("Automation Runtime"))
        {
            m_windows.open("runtime");
        }
        ImGui::SameLine();
        if (ImGui::Button("Runtime Dashboard"))
        {
            m_windows.open("runtime-dashboard");
        }
        ImGui::SameLine();
        if (ImGui::Button("Settings"))
        {
            m_windows.open("settings");
        }
        ImGui::SameLine();
        if (ImGui::Button("Config Editor"))
        {
            m_windows.open("config-editor");
        }
        ImGui::SameLine();
        if (ImGui::Button("Config Manager"))
        {
            m_windows.open("config-manager");
        }
        ImGui::SameLine();
        if (ImGui::Button("Diagnostics"))
        {
            m_windows.open("backend-diagnostics");
        }
        ImGui::SameLine();
        if (ImGui::Button("Sequence Recorder"))
        {
            m_windows.open("sequence-recorder");
        }
        ImGui::SameLine();
        if (ImGui::Button("Sequence Editor"))
        {
            m_windows.open("sequence-editor");
        }

        ImGui::Separator();
        ImGui::Text("Log / Status Panel:");
        ImGui::BeginChild("LogRegion", ImVec2(0, 0), true);
        ImGui::TextUnformatted("Application initialized.");
        ImGui::TextUnformatted("Ready to receive commands.");
        ImGui::EndChild();

        if (m_showDemoWindow)
        {
            ImGui::ShowDemoWindow(&m_showDemoWindow);
        }
    }
}

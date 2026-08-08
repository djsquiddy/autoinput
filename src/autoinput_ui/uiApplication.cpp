#include "uiApplication.h"
#include "settingsEditorWindow.h"
#include "configEditorWindow.h"
#include <raylib.h>
#include <imgui.h>
#include <rlImGui.h>

namespace autoinput::ui
{
    UiApplication::UiApplication()
    {
        m_settingsEditor = std::make_unique<SettingsEditorWindow>();
        m_configEditor = std::make_unique<ConfigEditorWindow>();
    }

    UiApplication::~UiApplication() = default;

    void UiApplication::run()
    {
        initialize();

        while (!m_shouldClose && !WindowShouldClose())
        {
            handleInput();
            update();
            render();
        }

        shutdown();
    }

    void UiApplication::initialize()
    {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE);
        InitWindow(1024, 768, "autoinput UI");
        SetTargetFPS(60);
        rlImGuiSetup(true);
    }

    void UiApplication::shutdown()
    {
        rlImGuiShutdown();
        CloseWindow();
    }

    void UiApplication::handleInput()
    {
        // Basic input handling
    }

    void UiApplication::update()
    {
        // Update logic
    }

    void UiApplication::render()
    {
        BeginDrawing();
        ClearBackground(DARKGRAY);

        rlImGuiBegin();

        // UI Layout
        ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())), ImGuiCond_Always);

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

        if (ImGui::Begin("AutoInput UI", nullptr, window_flags))
        {
            if (ImGui::BeginMenuBar())
            {
                if (ImGui::BeginMenu("File"))
                {
                    if (ImGui::MenuItem("Exit"))
                    {
                        m_shouldClose = true;
                    }
                    ImGui::EndMenu();
                }
                if (ImGui::BeginMenu("Edit"))
                {
                    if (ImGui::MenuItem("Settings"))
                    {
                        m_settingsEditor->open();
                    }
                    if (ImGui::MenuItem("Config Editor"))
                    {
                        m_configEditor->open();
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenuBar();
            }

            ImGui::Text("App Title: autoinput");
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
            if (ImGui::Button("Settings"))
            {
                m_settingsEditor->open();
            }
            ImGui::SameLine();
            if (ImGui::Button("Config Editor"))
            {
                m_configEditor->open();
            }

            ImGui::Separator();
            ImGui::Text("Log / Status Panel:");
            ImGui::BeginChild("LogRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);
            ImGui::TextUnformatted("Application initialized.");
            ImGui::TextUnformatted("Ready to receive commands.");
            ImGui::EndChild();
        }
        ImGui::End();

        m_settingsEditor->render();
        m_configEditor->render();

        rlImGuiEnd();

        EndDrawing();
    }
}

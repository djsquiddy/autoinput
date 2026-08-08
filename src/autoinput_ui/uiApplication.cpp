#include "uiApplication.h"
#include "imgui_impl_raylib.h"
#include "settingsEditorWindow.h"
#include "configEditorWindow.h"
#include "autoinput/config.h"
#include "autoinput/logger.h"
#include <raylib.h>
#include <imgui.h>

namespace autoinput::ui
{
    namespace
    {
        namespace fs = std::filesystem;
    }

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
        Logger::info("Initializing UI Application...");
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
        InitWindow(1024, 768, "autoinput");
        SetTargetFPS(60);

        ImGui::CreateContext();
        ImGui_ImplRaylib_Init();
        ImGui::StyleColorsDark();

        if (ImGui::GetCurrentContext())
        {
            Logger::info("ImGui context created successfully.");
            
            // Explicitly build font atlas
            unsigned char* pixels;
            int width, height;
            ImGui::GetIO().Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
            
            if (ImGui::GetIO().Fonts->IsBuilt())
            {
                Logger::info("ImGui font atlas built successfully. Size: {}x{}", width, height);
            }
            else
            {
                Logger::error("ImGui font atlas NOT built even after GetTexData!");
            }
        }
        else
        {
            Logger::error("Failed to create ImGui context!");
        }

        // Set imgui.ini path to user config directory
        const auto userConfigPath = getUserConfigsPath();
        if (!userConfigPath.empty())
        {
            if (!fs::exists(userConfigPath))
            {
                fs::create_directories(userConfigPath);
            }
            static std::string iniPath = (userConfigPath / "imgui.ini").string();
            ImGui::GetIO().IniFilename = iniPath.c_str();
        }
    }

    void UiApplication::shutdown()
    {
        ImGui_ImplRaylib_Shutdown();
        ImGui::DestroyContext();
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

        ImGui_ImplRaylib_NewFrame();
        ImGui::NewFrame();

        // UI Layout
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)GetScreenWidth(), (float)GetScreenHeight()));
        ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_MenuBar |
                                      ImGuiWindowFlags_NoCollapse;

        if (ImGui::Begin("AutoInput UI", nullptr, windowFlags))
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
                    ImGui::Separator();
                    if (ImGui::MenuItem("ImGui Demo"))
                    {
                        m_showDemoWindow = true;
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
            ImGui::BeginChild("LogRegion", ImVec2(0, 0), true);
            ImGui::TextUnformatted("Application initialized.");
            ImGui::TextUnformatted("Ready to receive commands.");
            ImGui::EndChild();
        }
        ImGui::End();

        m_settingsEditor->render();
        m_configEditor->render();

        if (m_showDemoWindow)
        {
            ImGui::ShowDemoWindow(&m_showDemoWindow);
        }

        ImGui::Render();
        ImGui_ImplRaylib_RenderDrawData(ImGui::GetDrawData());

        EndDrawing();
    }
}

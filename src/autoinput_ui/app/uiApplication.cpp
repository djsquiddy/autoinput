/**
 * @file uiApplication.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "uiApplication.h"
#include "../imgui_impl_raylib.h"
#include "../core/windowManager.h"
#include "../windows/mainWindow.h"
#include "../windows/settingsEditorWindow.h"
#include "../windows/configEditorWindow.h"
#include "autoinput/config.h"
#include "autoinput/logger.h"
#include <raylib.h>
#include <imgui.h>
#include <filesystem>

namespace autoinput::ui
{
    namespace
    {
        namespace fs = std::filesystem;
    }

    UiApplication::UiApplication()
        : m_windowManager{ std::make_unique<WindowManager>() }
    {
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
            
            if (auto* main = m_windowManager->findAs<MainWindow>("main"))
            {
                if (main->shouldExit()) m_shouldClose = true;
            }
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

        m_windowManager->addWindow<MainWindow>("main", *m_windowManager);
        m_windowManager->addWindow<SettingsEditorWindow>("settings");
        m_windowManager->addWindow<ConfigEditorWindow>("config-editor");
        
        m_windowManager->open("main");
    }

    void UiApplication::shutdown()
    {
        ImGui_ImplRaylib_Shutdown();
        ImGui::DestroyContext();
        CloseWindow();
    }

    void UiApplication::handleInput()
    {
    }

    void UiApplication::update()
    {
    }

    void UiApplication::render()
    {
        BeginDrawing();
        ClearBackground(DARKGRAY);

        ImGui_ImplRaylib_NewFrame();
        ImGui::NewFrame();

        m_windowManager->render();

        ImGui::Render();
        ImGui_ImplRaylib_RenderDrawData(ImGui::GetDrawData());

        EndDrawing();
    }
}

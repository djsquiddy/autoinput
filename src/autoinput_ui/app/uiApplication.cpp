/**
 * @file uiApplication.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "uiApplication.h"
#include "../core/uibackend.h"
#include "../core/windowManager.h"
#include "../windows/mainWindow.h"
#include "../windows/settingsEditorWindow.h"
#include "../windows/configEditorWindow.h"
#include "../windows/runtimeWindow.h"
#include "../windows/runtimeDashboardWindow.h"
#include "autoinput/config.h"
#include "autoinput/logger.h"
#include "autoinput/services/automationRuntimeClient.h"
#include <raylib.h>

namespace autoinput::ui
{
    UiApplication::UiApplication()
        : m_uiBackend{ createUiBackend() }
        , m_windowManager{ std::make_unique<WindowManager>() }
        , m_runtimeClient{ autoinput::services::createAutomationRuntimeClient(services::AutomationRuntimeClientMode::InProcess) }
    {
    }

    UiApplication::~UiApplication() = default;

    void UiApplication::run()
    {
        initialize();

        const auto* main = m_windowManager->findAs<MainWindow>("main");
        while (!m_shouldClose && !m_uiBackend->windowShouldClose())
        {
            handleInput();
            update();
            render();

            if (main && main->shouldExit())
            {
                m_shouldClose = true;
            }
        }

        shutdown();
    }

    autoinput::services::IAutomationRuntimeClient& UiApplication::getRuntimeClient() const
    {
        return *m_runtimeClient;
    }

    void UiApplication::initialize()
    {
        Logger::info("Initializing UI Application...");
        m_uiBackend->init();

        m_windowManager->addWindow<MainWindow>("main", *m_windowManager);
        m_windowManager->addWindow<SettingsEditorWindow>("settings");
        m_windowManager->addWindow<ConfigEditorWindow>("config-editor");
        m_windowManager->addWindow<RuntimeWindow>("runtime", getRuntimeClient());
        m_windowManager->addWindow<RuntimeDashboardWindow>("runtime-dashboard", getRuntimeClient());
        
        m_windowManager->open("main");
    }

    void UiApplication::shutdown()
    {
        CloseWindow();
    }

    void UiApplication::handleInput()
    {
        // No-op
    }

    void UiApplication::update()
    {
        // No-op
    }

    void UiApplication::render()
    {
        m_uiBackend->newFrame();
        m_windowManager->render();
        m_uiBackend->render();
    }
}

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
#include "../windows/commandRunnerWindow.h"
#include "../windows/logViewerWindow.h"
#include "../windows/backendDiagnosticsWindow.h"
#include "../windows/sequenceRecorderWindow.h"
#include "../windows/sequenceEditorWindow.h"
#include "../windows/configManagerWindow.h"
#include "../windows/hotkeyManagerWindow.h"
#include "../windows/applicationPickerWindow.h"
#include "../windows/validationReportWindow.h"
#include "../windows/setupWizardWindow.h"
#include "../windows/notificationTesterWindow.h"
#include "../windows/commandPaletteWindow.h"
#include "../windows/aboutWindow.h"
#include "../windows/importExportWindow.h"
#include "../windows/backupRestoreWindow.h"
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
        m_windowManager->addWindow<CommandRunnerWindow>("command-runner", getRuntimeClient(), SystemEnvironment::instance());
        m_windowManager->addWindow<LogViewerWindow>("logs", SystemEnvironment::instance());
        m_windowManager->addWindow<BackendDiagnosticsWindow>("backend-diagnostics", getRuntimeClient(), SystemEnvironment::instance());
        m_windowManager->addWindow<SequenceRecorderWindow>("sequence-recorder", getRuntimeClient(), SystemEnvironment::instance());
        m_windowManager->addWindow<SequenceEditorWindow>("sequence-editor");
        m_windowManager->addWindow<ConfigManagerWindow>("config-manager", *m_windowManager, SystemEnvironment::instance());
        m_windowManager->addWindow<HotkeyManagerWindow>("hotkey-manager", *m_windowManager, getRuntimeClient(), SystemEnvironment::instance());
        m_windowManager->addWindow<ApplicationPickerWindow>("application-picker", *m_windowManager, getRuntimeClient(), SystemEnvironment::instance());
        m_windowManager->addWindow<windows::ValidationReportWindow>("validation-report", *m_windowManager, SystemEnvironment::instance());
        m_windowManager->addWindow<SetupWizardWindow>("setup-wizard", *m_windowManager, getRuntimeClient(), SystemEnvironment::instance());
        m_windowManager->addWindow<NotificationTesterWindow>("notification-tester", getRuntimeClient());
        m_windowManager->addWindow<CommandPaletteWindow>("command-palette", *m_windowManager);
        m_windowManager->addWindow<AboutWindow>("about", getRuntimeClient(), SystemEnvironment::instance());
        m_windowManager->addWindow<ImportExportWindow>("import-export");
        m_windowManager->addWindow<BackupRestoreWindow>("backup-restore");
        
        m_windowManager->open("main");

        // Open setup wizard if not completed
        Settings settings;
        if (settings.load() && !settings.getDefaults().setupCompleted)
        {
            m_windowManager->open("setup-wizard");
        }
    }

    void UiApplication::shutdown()
    {
        CloseWindow();
    }

    void UiApplication::handleInput()
    {
        // Global shortcut for Command Palette (Ctrl+P or Ctrl+Shift+P)
        if ((IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) && IsKeyPressed(KEY_P))
        {
            m_windowManager->open("command-palette");
        }
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

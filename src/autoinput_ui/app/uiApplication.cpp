/**
 * @file uiApplication.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "uiApplication.h"
#include "../core/localization.h"
#include "../core/windowIds.h"
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

    services::IAutomationRuntimeClient& UiApplication::getRuntimeClient() const
    {
        return *m_runtimeClient;
    }

    void UiApplication::initialize()
    {
        Logger::info("Initializing UI Application...");

        // Load localization
        std::filesystem::path locPath = SystemEnvironment::instance().executableDirectoryPath() / "resources" / "localization" / "en-US.toml";
        if (!Localization::get().loadFromFile(locPath))
        {
            // Fallback to current directory if not found relative to exe
            Localization::get().loadFromFile("resources/localization/en-US.toml");
        }

        m_uiBackend->init();

        m_windowManager->addWindow<MainWindow>(std::string(WindowIds::Main), *m_windowManager);
        m_windowManager->addWindow<SettingsEditorWindow>(std::string(WindowIds::Settings));
        m_windowManager->addWindow<ConfigEditorWindow>(std::string(WindowIds::ConfigEditor));
        m_windowManager->addWindow<RuntimeWindow>(std::string(WindowIds::Runtime), getRuntimeClient());
        m_windowManager->addWindow<RuntimeDashboardWindow>(std::string(WindowIds::RuntimeDashboard), getRuntimeClient());
        m_windowManager->addWindow<CommandRunnerWindow>(std::string(WindowIds::CommandRunner), getRuntimeClient(), SystemEnvironment::instance());
        m_windowManager->addWindow<LogViewerWindow>(std::string(WindowIds::Logs), SystemEnvironment::instance());
        m_windowManager->addWindow<BackendDiagnosticsWindow>(std::string(WindowIds::BackendDiagnostics), getRuntimeClient(), SystemEnvironment::instance());
        m_windowManager->addWindow<SequenceRecorderWindow>(std::string(WindowIds::SequenceRecorder), getRuntimeClient(), SystemEnvironment::instance());
        m_windowManager->addWindow<SequenceEditorWindow>(std::string(WindowIds::SequenceEditor));
        m_windowManager->addWindow<ConfigManagerWindow>(std::string(WindowIds::ConfigManager), *m_windowManager, SystemEnvironment::instance());
        m_windowManager->addWindow<HotkeyManagerWindow>(std::string(WindowIds::HotkeyManager), *m_windowManager, getRuntimeClient(), SystemEnvironment::instance());
        m_windowManager->addWindow<ApplicationPickerWindow>(std::string(WindowIds::ApplicationPicker), *m_windowManager, getRuntimeClient(), SystemEnvironment::instance());
        m_windowManager->addWindow<windows::ValidationReportWindow>(std::string(WindowIds::ValidationReport), *m_windowManager, SystemEnvironment::instance());
        m_windowManager->addWindow<SetupWizardWindow>(std::string(WindowIds::SetupWizard), *m_windowManager, getRuntimeClient(), SystemEnvironment::instance());
        m_windowManager->addWindow<NotificationTesterWindow>(std::string(WindowIds::NotificationTester), getRuntimeClient());
        m_windowManager->addWindow<CommandPaletteWindow>(std::string(WindowIds::CommandPalette), *m_windowManager);
        m_windowManager->addWindow<AboutWindow>(std::string(WindowIds::About), getRuntimeClient(), SystemEnvironment::instance());
        m_windowManager->addWindow<ImportExportWindow>(std::string(WindowIds::ImportExport));
        m_windowManager->addWindow<BackupRestoreWindow>(std::string(WindowIds::BackupRestore));
        
        m_windowManager->open(WindowIds::Main);

        // Open setup wizard if not completed
        Settings settings;
        if (settings.load() && !settings.getDefaults().setupCompleted)
        {
            m_windowManager->open(WindowIds::SetupWizard);
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
            m_windowManager->open(WindowIds::CommandPalette);
        }
    }

    void UiApplication::update()
    {
        m_windowManager->update();
    }

    void UiApplication::render()
    {
        m_uiBackend->newFrame();
        m_windowManager->render();
        m_uiBackend->render();
    }
}

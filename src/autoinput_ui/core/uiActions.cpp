/**
 * @file uiActions.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "uiActions.h"
#include "windowIds.h"
#include "windowManager.h"
#include <algorithm>

namespace autoinput::ui
{
    std::vector<UiAction> UiActionRegistry::getActions()
    {
        return {
            { .id = "open-settings", .label = "Settings", .category = "Window", .shortcut = "", .targetWindowId = std::string(WindowIds::Settings), .callback = nullptr },
            { .id = "open-config-editor", .label = "Config Editor", .category = "Window", .shortcut = "", .targetWindowId = std::string(WindowIds::ConfigEditor), .callback = nullptr },
            { .id = "open-config-manager", .label = "Config Manager", .category = "Window", .shortcut = "", .targetWindowId = std::string(WindowIds::ConfigManager), .callback = nullptr },
            { .id = "open-command-runner", .label = "Command Runner", .category = "Runtime", .shortcut = "", .targetWindowId = std::string(WindowIds::CommandRunner), .callback = nullptr },
            { .id = "open-runtime-dashboard", .label = "Runtime Dashboard", .category = "Runtime", .shortcut = "", .targetWindowId = std::string(WindowIds::RuntimeDashboard), .callback = nullptr },
            { .id = "open-logs", .label = "Logs", .category = "Window", .shortcut = "", .targetWindowId = std::string(WindowIds::Logs), .callback = nullptr },
            { .id = "open-backend-diagnostics", .label = "Backend Diagnostics", .category = "Runtime", .shortcut = "", .targetWindowId = std::string(WindowIds::BackendDiagnostics), .callback = nullptr },
            { .id = "open-sequence-recorder", .label = "Sequence Recorder", .category = "Runtime", .shortcut = "", .targetWindowId = std::string(WindowIds::SequenceRecorder), .callback = nullptr },
            { .id = "open-sequence-editor", .label = "Sequence Editor", .category = "Window", .shortcut = "", .targetWindowId = std::string(WindowIds::SequenceEditor), .callback = nullptr },
            { .id = "open-hotkey-manager", .label = "Hotkey Manager", .category = "Window", .shortcut = "", .targetWindowId = std::string(WindowIds::HotkeyManager), .callback = nullptr },
            { .id = "open-application-picker", .label = "Application Picker", .category = "Window", .shortcut = "", .targetWindowId = std::string(WindowIds::ApplicationPicker), .callback = nullptr },
            { .id = "open-validation-report", .label = "Validation Report", .category = "Window", .shortcut = "", .targetWindowId = std::string(WindowIds::ValidationReport), .callback = nullptr },
            { .id = "open-setup-wizard", .label = "Setup Wizard", .category = "Window", .shortcut = "", .targetWindowId = std::string(WindowIds::SetupWizard), .callback = nullptr },
            { .id = "open-notification-tester", .label = "Notification Tester", .category = "Runtime", .shortcut = "", .targetWindowId = std::string(WindowIds::NotificationTester), .callback = nullptr },
            { .id = "open-import-export", .label = "Import / Export", .category = "Window", .shortcut = "", .targetWindowId = std::string(WindowIds::ImportExport), .callback = nullptr },
            { .id = "open-backup-restore", .label = "Backup / Restore", .category = "Window", .shortcut = "", .targetWindowId = std::string(WindowIds::BackupRestore), .callback = nullptr },
            { .id = "open-about", .label = "About AutoInput", .category = "Window", .shortcut = "", .targetWindowId = std::string(WindowIds::About), .callback = nullptr },
            { .id = "open-advanced-runtime", .label = "Advanced Runtime Control", .category = "Tools", .shortcut = "", .targetWindowId = std::string(WindowIds::Runtime), .callback = nullptr },
            { .id = "validate-all", .label = "Validate All Configs", .category = "Config", .shortcut = "", .targetWindowId = std::string(WindowIds::ValidationReport), .callback = nullptr }
        };
    }

    bool UiActionRegistry::execute(const std::string_view id, WindowManager& windowManager)
    {
        auto actions = getActions();
        const auto it = std::ranges::find_if(actions, [&](const UiAction& a) { return a.id == id; });
        if (it == actions.end())
        {
            return false;
        }
        if (it->callback)
        {
            it->callback(windowManager);
        }
        else if (!it->targetWindowId.empty())
        {
            windowManager.open(it->targetWindowId);
        }
        return true;
    }
}

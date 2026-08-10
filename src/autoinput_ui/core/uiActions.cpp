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
            { .id = "open-settings", .labelKey = "windows.settings", .categoryKey = "actionCategories.window", .shortcut = "", .targetWindowId = std::string(WindowIds::Settings), .callback = nullptr },
            { .id = "open-config-editor", .labelKey = "windows.configEditor", .categoryKey = "actionCategories.window", .shortcut = "", .targetWindowId = std::string(WindowIds::ConfigEditor), .callback = nullptr },
            { .id = "open-config-manager", .labelKey = "windows.configManager", .categoryKey = "actionCategories.window", .shortcut = "", .targetWindowId = std::string(WindowIds::ConfigManager), .callback = nullptr },
            { .id = "open-command-runner", .labelKey = "windows.commandRunner", .categoryKey = "actionCategories.runtime", .shortcut = "", .targetWindowId = std::string(WindowIds::CommandRunner), .callback = nullptr },
            { .id = "open-runtime-dashboard", .labelKey = "windows.runtimeDashboard", .categoryKey = "actionCategories.runtime", .shortcut = "", .targetWindowId = std::string(WindowIds::RuntimeDashboard), .callback = nullptr },
            { .id = "open-logs", .labelKey = "windows.logs", .categoryKey = "actionCategories.window", .shortcut = "", .targetWindowId = std::string(WindowIds::Logs), .callback = nullptr },
            { .id = "open-backend-diagnostics", .labelKey = "windows.backendDiagnostics", .categoryKey = "actionCategories.runtime", .shortcut = "", .targetWindowId = std::string(WindowIds::BackendDiagnostics), .callback = nullptr },
            { .id = "open-sequence-recorder", .labelKey = "windows.sequenceRecorder", .categoryKey = "actionCategories.runtime", .shortcut = "", .targetWindowId = std::string(WindowIds::SequenceRecorder), .callback = nullptr },
            { .id = "open-sequence-editor", .labelKey = "windows.sequenceEditor", .categoryKey = "actionCategories.window", .shortcut = "", .targetWindowId = std::string(WindowIds::SequenceEditor), .callback = nullptr },
            { .id = "open-hotkey-manager", .labelKey = "windows.hotkeyManager", .categoryKey = "actionCategories.window", .shortcut = "", .targetWindowId = std::string(WindowIds::HotkeyManager), .callback = nullptr },
            { .id = "open-application-picker", .labelKey = "windows.applicationPicker", .categoryKey = "actionCategories.window", .shortcut = "", .targetWindowId = std::string(WindowIds::ApplicationPicker), .callback = nullptr },
            { .id = "open-validation-report", .labelKey = "windows.validationReport", .categoryKey = "actionCategories.window", .shortcut = "", .targetWindowId = std::string(WindowIds::ValidationReport), .callback = nullptr },
            { .id = "open-setup-wizard", .labelKey = "windows.setupWizard", .categoryKey = "actionCategories.window", .shortcut = "", .targetWindowId = std::string(WindowIds::SetupWizard), .callback = nullptr },
            { .id = "open-notification-tester", .labelKey = "windows.notificationTester", .categoryKey = "actionCategories.runtime", .shortcut = "", .targetWindowId = std::string(WindowIds::NotificationTester), .callback = nullptr },
            { .id = "open-import-export", .labelKey = "windows.importExport", .categoryKey = "actionCategories.window", .shortcut = "", .targetWindowId = std::string(WindowIds::ImportExport), .callback = nullptr },
            { .id = "open-backup-restore", .labelKey = "windows.backupRestore", .categoryKey = "actionCategories.window", .shortcut = "", .targetWindowId = std::string(WindowIds::BackupRestore), .callback = nullptr },
            { .id = "open-about", .labelKey = "windows.about", .categoryKey = "actionCategories.window", .shortcut = "", .targetWindowId = std::string(WindowIds::About), .callback = nullptr },
            { .id = "open-advanced-runtime", .labelKey = "windows.advancedRuntime", .categoryKey = "actionCategories.tools", .shortcut = "", .targetWindowId = std::string(WindowIds::Runtime), .callback = nullptr },
            { .id = "validate-all", .labelKey = "actions.validateAll", .categoryKey = "actionCategories.config", .shortcut = "", .targetWindowId = std::string(WindowIds::ValidationReport), .callback = nullptr }
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

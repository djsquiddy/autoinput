/**
 * @file windowIds.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_CORE_WINDOW_IDS_H
#define INCLUDE_AUTOINPUT_UI_CORE_WINDOW_IDS_H
#pragma once

#include <string_view>

namespace autoinput::ui::WindowIds
{
    constexpr std::string_view Main{ "main" };
    constexpr std::string_view Settings{ "settings" };
    constexpr std::string_view ConfigEditor{ "config-editor" };
    constexpr std::string_view Runtime{ "runtime" };
    constexpr std::string_view RuntimeDashboard{ "runtime-dashboard" };
    constexpr std::string_view CommandRunner{ "command-runner" };
    constexpr std::string_view Logs{ "logs" };
    constexpr std::string_view BackendDiagnostics{ "backend-diagnostics" };
    constexpr std::string_view SequenceRecorder{ "sequence-recorder" };
    constexpr std::string_view SequenceEditor{ "sequence-editor" };
    constexpr std::string_view ConfigManager{ "config-manager" };
    constexpr std::string_view HotkeyManager{ "hotkey-manager" };
    constexpr std::string_view ApplicationPicker{ "application-picker" };
    constexpr std::string_view ValidationReport{ "validation-report" };
    constexpr std::string_view SetupWizard{ "setup-wizard" };
    constexpr std::string_view NotificationTester{ "notification-tester" };
    constexpr std::string_view CommandPalette{ "command-palette" };
    constexpr std::string_view About{ "about" };
    constexpr std::string_view ImportExport{ "import-export" };
    constexpr std::string_view BackupRestore{ "backup-restore" };
}

#endif // INCLUDE_AUTOINPUT_UI_CORE_WINDOW_IDS_H

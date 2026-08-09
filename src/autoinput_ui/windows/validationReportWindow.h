/**
 * @file validationReportWindow.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_WINDOWS_VALIDATION_REPORT_WINDOW_H
#define INCLUDE_AUTOINPUT_UI_WINDOWS_VALIDATION_REPORT_WINDOW_H
#pragma once

#include "autoinput_ui/core/uiWindow.h"
#include "autoinput/configValidator.h"
#include "autoinput/services/configService.h"
#include "autoinput/settings.h"
#include <vector>
#include <string>

namespace autoinput::ui
{
    class WindowManager;
}

namespace autoinput::ui::windows
{
    struct ValidationIssue
    {
        std::string sourceName;
        std::string sourcePath;
        std::string type; // "Settings", "Config", etc.
        ValidationError error;
    };

    class ValidationReportWindow : public UiWindow
    {
    public:
        ValidationReportWindow(WindowManager& windowManager, const IEnvironment& environment);
        ~ValidationReportWindow() override = default;

    protected:
        void renderContent() override;

    private:
        void runCurrentConfigValidation();
        void runAllConfigsValidation();
        void runSettingsValidation();
        void clearResults();
        void copyReport();
        void openInEditor(const ValidationIssue& issue);

        WindowManager& m_windowManager;
        const IEnvironment& m_environment;
        services::ConfigService m_configService;
        Settings m_settings;

        std::vector<ValidationIssue> m_issues;
        std::string m_statusMessage;
    };
}

#endif // INCLUDE_AUTOINPUT_UI_WINDOWS_VALIDATION_REPORT_WINDOW_H

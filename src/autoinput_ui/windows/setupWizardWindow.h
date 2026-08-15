/**
 * @file setupWizardWindow.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_WINDOWS_SETUP_WIZARD_WINDOW_H
#define INCLUDE_AUTOINPUT_UI_WINDOWS_SETUP_WIZARD_WINDOW_H
#pragma once

#include "../core/uiWindow.h"
#include "autoinput/platform/environment.h"
#include "autoinput/services/automationRuntimeClient.h"
#include "autoinput/config/settings.h"
#include "autoinput/config/config.h"

namespace autoinput::ui
{
    class WindowManager;

    class SetupWizardWindow : public UiWindow
    {
    public:
        SetupWizardWindow(WindowManager& windowManager, 
                          services::IAutomationRuntimeClient& runtimeClient,
                          const IEnvironment& environment);

    protected:
        void renderContent() override;
        void update() override;
        void onOpen() override;
        int getFlags() const override;
        bool hasCloseButton() const override { return true; }

    private:
        enum class Step
        {
            Welcome,
            Locations,
            EndKeyAndNotification,
            Diagnostics,
            FirstConfig,
            Summary
        };

        void renderWelcome();
        void renderLocations();
        void renderEndKeyAndNotification();
        void renderDiagnostics();
        void renderFirstConfig();
        void renderSummary();

        void renderButtons();
        bool validateCurrentStep();
        void saveAndFinish();
        
        void startCapture();
        void stopCapture();

        WindowManager& m_windowManager;
        services::IAutomationRuntimeClient& m_runtimeClient;
        const IEnvironment& m_environment;

        Step m_currentStep{ Step::Welcome };
        Settings m_settings;
        ConfigData m_newConfig;
        std::string m_newConfigName{ "default" };
        
        bool m_isDiagnosticsPassed{ false };
        std::string m_diagnosticsMessage;

        // Step 3 values
        std::string m_endKey;
        std::string m_notificationMode;
        std::string m_uiLanguage;

        // Capture state
        bool m_isCapturing = false;
        uint32_t m_captureStartEventCount = 0;
    };
}

#endif // INCLUDE_AUTOINPUT_UI_WINDOWS_SETUP_WIZARD_WINDOW_H

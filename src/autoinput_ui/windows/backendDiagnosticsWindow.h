/**
 * @file backendDiagnosticsWindow.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_WINDOWS_BACKEND_DIAGNOSTICS_WINDOW_H
#define INCLUDE_AUTOINPUT_UI_WINDOWS_BACKEND_DIAGNOSTICS_WINDOW_H
#pragma once

#include "../core/uiWindow.h"
#include "autoinput/services/automationRuntimeClient.h"
#include "autoinput/environment.h"
#include <string>

namespace autoinput::ui
{
    /**
     * @brief A window for displaying backend and runtime diagnostics.
     */
    class BackendDiagnosticsWindow final : public UiWindow
    {
    public:
        BackendDiagnosticsWindow(services::IAutomationRuntimeClient& runtimeClient, const IEnvironment& environment);

    protected:
        void renderContent() override;
        void onOpen() override;

    private:
        void refreshDiagnostics();
        void pingRuntime();
        void sendTestNotification();

        services::IAutomationRuntimeClient& m_runtimeClient;
        const IEnvironment& m_environment;

        std::string m_platformName;
        std::string m_backendName;
        bool m_transportConnected{ false };
        std::string m_lastError;
        BackendCapabilities m_capabilities;
        
        bool m_pingPending{ false };
        std::string m_pingResult;
        bool m_notificationPending{ false };
        std::string m_notificationResult;
    };
}

#endif // INCLUDE_AUTOINPUT_UI_WINDOWS_BACKEND_DIAGNOSTICS_WINDOW_H

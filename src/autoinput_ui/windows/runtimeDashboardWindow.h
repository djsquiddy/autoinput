/**
 * @file runtimeDashboardWindow.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_WINDOWS_RUNTIME_DASHBOARD_WINDOW_H
#define INCLUDE_AUTOINPUT_UI_WINDOWS_RUNTIME_DASHBOARD_WINDOW_H
#pragma once

#include "../core/uiWindow.h"
#include "autoinput/services/automationRuntimeClient.h"

namespace autoinput::ui
{
    /**
     * @brief A dashboard window summarizing the current runtime state.
     */
    class RuntimeDashboardWindow final : public UiWindow
    {
    public:
        explicit RuntimeDashboardWindow(services::IAutomationRuntimeClient& runtimeClient);

    protected:
        void renderContent() override;

    private:
        services::IAutomationRuntimeClient& m_runtimeClient;
        char m_configInput[256]{ "" };
        std::string m_lastOperationMessage;
    };
}

#endif // INCLUDE_AUTOINPUT_UI_WINDOWS_RUNTIME_DASHBOARD_WINDOW_H

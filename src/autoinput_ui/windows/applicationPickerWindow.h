/**
 * @file applicationPickerWindow.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_WINDOWS_APPLICATION_PICKER_WINDOW_H
#define INCLUDE_AUTOINPUT_UI_WINDOWS_APPLICATION_PICKER_WINDOW_H
#pragma once

#include "../core/uiWindow.h"
#include "autoinput/services/automationRuntimeClient.h"
#include "autoinput/config/config.h"
#include <vector>
#include <string>
#include <optional>

namespace autoinput::ui
{
    class WindowManager;

    class ApplicationPickerWindow final : public UiWindow
    {
    public:
        ApplicationPickerWindow(WindowManager& windowManager, services::IAutomationRuntimeClient& runtimeClient, const IEnvironment& environment);

    protected:
        void renderContent() override;
        void update() override;

    private:
        void refreshWindows();
        void useAsTarget();
        void addToGlobalBlacklist();
        void addToCurrentConfigBlacklist();
        void copyIdentifier();
        void copyWindowTitle();
        void testMatch();

        WindowManager& m_windowManager;
        services::IAutomationRuntimeClient& m_runtimeClient;
        const IEnvironment& m_environment;

        std::vector<AppWindowInfo> m_windows;
        std::optional<AppWindowInfo> m_foregroundWindow;
        int m_selectedWindowIndex = -1;
        std::string m_statusMessage;
        std::string m_searchText;
        
        // Test match state
        std::string m_matchResult;
        bool m_matchSuccess = false;
    };
}

#endif // INCLUDE_AUTOINPUT_UI_WINDOWS_APPLICATION_PICKER_WINDOW_H

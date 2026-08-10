/**
 * @file aboutWindow.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_ABOUT_WINDOW_H
#define INCLUDE_AUTOINPUT_ABOUT_WINDOW_H
#pragma once
#include "ui.h"
#include "autoinput/services/automationRuntimeClient.h"
#include "autoinput/platform/environment.h"

namespace autoinput::ui
{
    /**
     * @brief A simple about window showing application information and diagnostics.
     */
    class AboutWindow final : public UiWindow
    {
    public:
        /**
         * @brief Constructs a new AboutWindow.
         */
        AboutWindow(services::IAutomationRuntimeClient& runtimeClient, const IEnvironment& environment)
            : UiWindow("About / Diagnostics", "windows.about"), m_runtimeClient(runtimeClient), m_environment(environment)
        {
        }

    protected:
        /**
         * @brief Renders the about window content (version, author, etc.).
         */
        void renderContent() override;

    private:
        services::IAutomationRuntimeClient& m_runtimeClient;
        const IEnvironment& m_environment;
    };
}

#endif // INCLUDE_AUTOINPUT_ABOUT_WINDOW_H

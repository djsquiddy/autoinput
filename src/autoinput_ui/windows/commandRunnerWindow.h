/**
 * @file commandRunnerWindow.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_WINDOWS_COMMAND_RUNNER_WINDOW_H
#define INCLUDE_AUTOINPUT_UI_WINDOWS_COMMAND_RUNNER_WINDOW_H
#pragma once

#include "../core/uiWindow.h"
#include "autoinput/services/automationRuntimeClient.h"
#include "autoinput/services/configService.h"
#include "autoinput/config/config.h"
#include <vector>
#include <string>

namespace autoinput::ui
{
    /**
     * @brief A window that allows selecting and running specific commands from configurations.
     */
    class CommandRunnerWindow final : public UiWindow
    {
    public:
        explicit CommandRunnerWindow(services::IAutomationRuntimeClient& runtimeClient, const IEnvironment& environment);

        void onOpen() override;
        void renderContent() override;

    private:
        void refreshConfigs();
        void loadSelectedConfig();

        services::IAutomationRuntimeClient& m_runtimeClient;
        services::ConfigService m_configService;
        
        std::vector<std::string> m_availableConfigs;
        int m_selectedConfigIndex{ -1 };
        
        ConfigData m_loadedConfig;
        std::vector<std::string> m_availableCommands;
        int m_selectedCommandIndex{ -1 };

        std::string m_statusMessage;
        bool m_isError{ false };
    };
}

#endif // INCLUDE_AUTOINPUT_UI_WINDOWS_COMMAND_RUNNER_WINDOW_H

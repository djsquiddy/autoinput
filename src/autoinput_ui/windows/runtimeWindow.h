/**
 * @file runtimeWindow.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_WINDOWS_RUNTIME_WINDOW_H
#define INCLUDE_AUTOINPUT_UI_WINDOWS_RUNTIME_WINDOW_H
#pragma once

#include "../core/uiWindow.h"
#include "autoinput/services/automationRuntimeClient.h"
#include <string>

namespace autoinput::ui
{
    constexpr size_t MaxConfigNameLength = 256;
    class RuntimeWindow final : public UiWindow
    {
    public:
        explicit RuntimeWindow(services::IAutomationRuntimeClient& runtimeClient);
        ~RuntimeWindow() override = default;

    protected:
        void renderContent() override;

    private:
        services::IAutomationRuntimeClient& m_runtimeClient;
        char m_configName[MaxConfigNameLength]{ "autoinput" };
        std::string m_lastMessage;
    };
}

#endif // INCLUDE_AUTOINPUT_UI_WINDOWS_RUNTIME_WINDOW_H

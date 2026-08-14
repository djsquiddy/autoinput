/**
 * @file hotkeyManagerWindow.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_WINDOWS_HOTKEY_MANAGER_WINDOW_H
#define INCLUDE_AUTOINPUT_UI_WINDOWS_HOTKEY_MANAGER_WINDOW_H
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

    struct HotkeyEntry
    {
        enum class Type { Command, Sequence, GlobalEnd };
        Type type;
        std::string name;
        std::string hotkey;
        bool isValid = true;
        bool hasConflict = false;
        size_t index = 0; // Index in the original config vector
    };

    class HotkeyManagerWindow final : public UiWindow
    {
    public:
        HotkeyManagerWindow(WindowManager& windowManager, services::IAutomationRuntimeClient& runtimeClient, const IEnvironment& environment);

        void onOpen() override;

    protected:
        void renderContent() override;
        void update() override;
        void save() override;

    private:
        void refreshConfigs();
        void loadSelectedConfig();
        void validateHotkeys();
        void startCapture(HotkeyEntry& entry);
        void stopCapture();
        void applyCapturedHotkey();

        WindowManager& m_windowManager;
        services::IAutomationRuntimeClient& m_runtimeClient;
        const IEnvironment& m_environment;
        
        std::vector<std::string> m_availableConfigs;
        int m_selectedConfigIndex = 0;
        std::string m_currentConfigName;
        std::optional<ConfigData> m_configData;
        
        std::vector<HotkeyEntry> m_entries;
        std::string m_statusMessage;
        
        // Capture state
        bool m_isCapturing = false;
        HotkeyEntry* m_captureTarget = nullptr;
        uint32_t m_captureStartEventCount = 0;
    };
}

#endif // INCLUDE_AUTOINPUT_UI_WINDOWS_HOTKEY_MANAGER_WINDOW_H

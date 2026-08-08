/**
 * @file  configEditorWindow.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_CONFIG_EDITOR_WINDOW_H
#define INCLUDE_AUTOINPUT_UI_CONFIG_EDITOR_WINDOW_H
#pragma once

#include "autoinput/config.h"
#include <vector>
#include <string>
#include <filesystem>

namespace autoinput
{
    struct ValidationError;
}

namespace autoinput::ui
{
    class ConfigEditorWindow
    {
    public:
        ConfigEditorWindow();

        void render();
        void open();
        void close();
        bool isOpen() const { return m_isOpen; }

    private:
        void loadConfig(const std::string& nameOrPath);
        void saveConfig(bool forceUser = true);
        void validate();
        void refreshConfigList();
        void createNewConfig();
        void duplicateConfig();

        bool m_isOpen = false;
        bool m_isDirty = false;
        bool m_shouldFocus = false;
        bool m_showSaveConfirmation = false;
        autoinput::ConfigData m_draft;
        std::string m_currentConfigName;
        std::filesystem::path m_currentConfigPath;
        std::vector<std::string> m_availableConfigs;
        std::vector<autoinput::ValidationError> m_validationErrors;
        std::string m_statusMessage;

        void renderGlobalSettings();
        void renderCommands();
        void renderSequences();
    };
}
#endif // INCLUDE_AUTOINPUT_UI_CONFIG_EDITOR_WINDOW_H

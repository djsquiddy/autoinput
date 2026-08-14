/**
 * @file configEditorWindow.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_WINDOWS_CONFIG_EDITOR_WINDOW_H
#define INCLUDE_AUTOINPUT_UI_WINDOWS_CONFIG_EDITOR_WINDOW_H
#pragma once

#include "../core/uiWindow.h"
#include "../editors/globalSettingsEditor.h"
#include "autoinput/config/config.h"
#include <vector>
#include <string>
#include <filesystem>

namespace autoinput
{
    struct ValidationError;
}

namespace autoinput::ui
{
    /**
     * @brief Window for editing automation configurations.
     * 
     * Supports creating, loading, modifying, and saving automation configs.
     * Includes tabs for global settings, commands, and sequence viewing.
     */
    class ConfigEditorWindow final : public UiWindow
    {
    public:
        ConfigEditorWindow();

        void onOpen() override;

        /**
         * @brief Loads a specific configuration.
         * @param nameOrPath Name of the config or direct path to file.
         */
        void loadConfig(const std::string& nameOrPath);

    protected:
        void renderContent() override;
        void save() override;

    private:
        /**
         * @brief Scans the configs directory and refreshes the list of available configs.
         */
        void refreshConfigList();

        /**
         * @brief Saves the current configuration.
         * @param forceUser If true, saves to the user-specific config directory.
         */
        void saveConfig(bool forceUser);

        /**
         * @brief Validates the current configuration draft.
         */
        void validate();

        /**
         * @brief Resets the draft to a new, empty configuration.
         */
        void createNewConfig();

        /**
         * @brief Duplicates the current configuration into a new one.
         */
        void duplicateConfig();

        autoinput::ConfigData m_draft;
        std::string m_currentConfigName;
        std::filesystem::path m_currentConfigPath;
        
        std::vector<std::string> m_availableConfigs;
        std::vector<autoinput::ValidationError> m_validationErrors;
        std::string m_statusMessage;

        /**
         * @brief Renders the top-level toolbar (New, Save, Refresh, etc.).
         */
        void renderToolbar();

        /**
         * @brief Renders the main tab bar (Global Settings, Commands, Sequences).
         */
        void renderTabs();

        /**
         * @brief Renders the global settings tab content.
         */
        void renderGlobalSettingsTab();

        /**
         * @brief Renders the command editor tab content.
         */
        void renderCommandsTab();

        /**
         * @brief Renders the sequence viewer tab content.
         */
        void renderSequencesTab();
    };
}

#endif // INCLUDE_AUTOINPUT_UI_WINDOWS_CONFIG_EDITOR_WINDOW_H

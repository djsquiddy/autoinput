/**
 * @file settingsEditorWindow.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_WINDOWS_SETTINGS_EDITOR_WINDOW_H
#define INCLUDE_AUTOINPUT_UI_WINDOWS_SETTINGS_EDITOR_WINDOW_H
#pragma once

#include "../core/uiWindow.h"
#include "../editors/globalSettingsEditor.h"
#include "autoinput/config/settings.h"
#include <vector>
#include <string>

namespace autoinput
{
    struct ValidationError;
}

namespace autoinput::ui
{
    /**
     * @brief Window for editing global application settings.
     * 
     * Handles loading, editing, validating, and saving application-wide settings.
     */
    class SettingsEditorWindow final : public UiWindow
    {
    public:
        SettingsEditorWindow();

    protected:
        void onOpen() override;
        void renderContent() override;
        void save() override;

    private:
        /**
         * @brief Loads settings from disk.
         */
        void loadSettings();

        /**
         * @brief Resets current editor state to default values.
         */
        void resetToDefaults();

        /**
         * @brief Validates the current editor state.
         */
        void validate();

        autoinput::Settings m_settings;
        editors::GlobalSettings m_editorSettings;
        std::vector<autoinput::ValidationError> m_validationErrors;
        std::string m_statusMessage;
    };
}

#endif // INCLUDE_AUTOINPUT_UI_WINDOWS_SETTINGS_EDITOR_WINDOW_H

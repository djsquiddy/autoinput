/**
* @file   settingsEditorWindow.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_SETTINGS_EDITOR_WINDOW_H
#define INCLUDE_AUTOINPUT_UI_SETTINGS_EDITOR_WINDOW_H
#pragma once

#include "autoinput/settings.h"
#include <vector>
#include <string>

namespace autoinput
{
    struct ValidationError;
}

namespace autoinput::ui
{
    class SettingsEditorWindow
    {
    public:
        SettingsEditorWindow();

        void render();
        void open();
        void close();
        bool isOpen() const { return m_isOpen; }

    private:
        void loadSettings();
        void saveSettings(bool toUserFile);
        void resetToDefaults();
        void validate();

        bool m_isOpen = false;
        bool m_isDirty = false;
        autoinput::Settings m_settings;
        autoinput::DefaultSettings m_draft;
        std::vector<autoinput::ValidationError> m_validationErrors;
        std::string m_statusMessage;

        // UI Buffers
        struct Buffers {
            char end[64]{};
            char application[256]{};
            std::vector<std::string> blacklist;
            bool appendBlacklist = true;
            int statusNotificationModeIdx = 0;
            int logLevelIdx = 0;
        } m_buffers;

        void syncBuffersFromDraft();
        void syncDraftFromBuffers();
    };
}
#endif // INCLUDE_AUTOINPUT_UI_SETTINGS_EDITOR_WINDOW_H

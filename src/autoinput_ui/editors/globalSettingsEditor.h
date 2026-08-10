/**
 * @file globalSettingsEditor.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_EDITORS_GLOBAL_SETTINGS_EDITOR_H
#define INCLUDE_AUTOINPUT_UI_EDITORS_GLOBAL_SETTINGS_EDITOR_H
#pragma once

#include <string>
#include <vector>

namespace autoinput::ui::editors
{
    /**
     * @brief A structure representing global settings shared between different configuration types.
     */
    struct GlobalSettings
    {
        std::string endKey;                   /**< Hotkey to end the current automation. */
        std::string application;               /**< Target application name. */
        std::vector<std::string> blacklist;   /**< List of processes where automation should not run. */
        bool appendBlacklist = true;          /**< Whether to append to the global blacklist. */
        std::string statusNotificationMode;   /**< How notifications should be shown. */
        std::string logLevel;                 /**< Logging verbosity level. */
        std::string uiLanguage;               /**< Language code for UI localization. */
    };

    /**
     * @brief Renders an editor for the global settings structure.
     * @param settings The settings object to edit.
     * @return true if any setting was modified.
     */
    bool renderGlobalSettingsEditor(GlobalSettings& settings);
}

#endif // INCLUDE_AUTOINPUT_UI_EDITORS_GLOBAL_SETTINGS_EDITOR_H

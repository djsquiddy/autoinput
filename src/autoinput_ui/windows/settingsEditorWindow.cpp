/**
 * @file settingsEditorWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "settingsEditorWindow.h"
#include "../widgets/basicWidgets.h"
#include "autoinput/configValidator.h"
#include "autoinput/config.h"
#include <imgui.h>

namespace autoinput::ui
{
    SettingsEditorWindow::SettingsEditorWindow()
        : UiWindow("Settings")
    {
        loadSettings();
    }

    void SettingsEditorWindow::onOpen()
    {
        loadSettings();
    }

    void SettingsEditorWindow::loadSettings()
    {
        m_settings.load();
        auto defaults = m_settings.getDefaults();
        
        m_editorSettings.endKey = defaults.end;
        m_editorSettings.application = defaults.application;
        m_editorSettings.blacklist = defaults.blacklist;
        m_editorSettings.appendBlacklist = defaults.appendBlacklist;
        m_editorSettings.statusNotificationMode = defaults.statusNotificationMode;
        m_editorSettings.logLevel = defaults.logLevel;

        clearDirty();
        m_statusMessage = "Settings loaded.";
        m_validationErrors.clear();
    }

    void SettingsEditorWindow::save()
    {
        DefaultSettings defaults;
        defaults.end = m_editorSettings.endKey;
        defaults.application = m_editorSettings.application;
        defaults.blacklist = m_editorSettings.blacklist;
        defaults.appendBlacklist = m_editorSettings.appendBlacklist;
        defaults.statusNotificationMode = m_editorSettings.statusNotificationMode;
        defaults.logLevel = m_editorSettings.logLevel;

        m_settings.setDefaults(defaults);
        
        auto path = autoinput::getUserConfigsPath() / defaults::SettingFileName;
        std::filesystem::create_directories(path.parent_path());

        if (m_settings.save(path))
        {
            m_statusMessage = "Settings saved to " + path.string();
            clearDirty();
        }
        else
        {
            m_statusMessage = "Failed to save settings!";
        }
    }

    void SettingsEditorWindow::resetToDefaults()
    {
        auto defaults = autoinput::DefaultSettings{};
        m_editorSettings.endKey = defaults.end;
        m_editorSettings.application = defaults.application;
        m_editorSettings.blacklist = defaults.blacklist;
        m_editorSettings.appendBlacklist = defaults.appendBlacklist;
        m_editorSettings.statusNotificationMode = defaults.statusNotificationMode;
        m_editorSettings.logLevel = defaults.logLevel;
        
        markDirty();
        m_statusMessage = "Reset to defaults (not saved yet).";
    }

    void SettingsEditorWindow::validate()
    {
        autoinput::ConfigData tempConfig;
        tempConfig.endKey = m_editorSettings.endKey;
        tempConfig.application = m_editorSettings.application;
        tempConfig.blacklist = m_editorSettings.blacklist;
        tempConfig.appendBlacklist = m_editorSettings.appendBlacklist;
        tempConfig.statusNotificationMode = m_editorSettings.statusNotificationMode;
        tempConfig.logLevel = m_editorSettings.logLevel;

        m_validationErrors = autoinput::validateConfigData(tempConfig);
        if (m_validationErrors.empty())
        {
            m_statusMessage = "Settings are valid.";
        }
        else
        {
            m_statusMessage = "Settings have validation errors.";
        }
    }

    void SettingsEditorWindow::renderContent()
    {
        if (isDirty())
        {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "You have unsaved changes!");
        }

        if (editors::renderGlobalSettingsEditor(m_editorSettings))
        {
            markDirty();
        }

        ImGui::Separator();
        if (ImGui::Button("Reload"))
        {
            loadSettings();
        }
        ImGui::SameLine();
        if (ImGui::Button("Save"))
        {
            save();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset to Defaults"))
        {
            resetToDefaults();
        }
        ImGui::SameLine();
        if (ImGui::Button("Validate"))
        {
            validate();
        }

        if (!m_statusMessage.empty())
        {
            widgets::StatusText("Status: " + m_statusMessage);
        }

        widgets::ValidationErrors(m_validationErrors);
    }
}

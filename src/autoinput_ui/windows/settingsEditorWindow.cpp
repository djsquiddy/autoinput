/**
 * @file settingsEditorWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "settingsEditorWindow.h"
#include "../widgets/basicWidgets.h"
#include "../core/localization.h"
#include "autoinput/configValidator.h"
#include "autoinput/config.h"
#include <imgui.h>

namespace autoinput::ui
{
    SettingsEditorWindow::SettingsEditorWindow()
        : UiWindow("Settings", "windows.settings")
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
        m_editorSettings.uiLanguage = defaults.uiLanguage;
 
        clearDirty();
        m_statusMessage = Localization::get().text("status.settingsLoaded");
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
        defaults.uiLanguage = m_editorSettings.uiLanguage;
 
        m_settings.setDefaults(defaults);
        
        auto path = autoinput::getUserConfigsPath() / defaults::SettingFileName;
        std::filesystem::create_directories(path.parent_path());

        if (m_settings.save(path))
        {
            m_statusMessage = Localization::get().format("status.settingsSavedTo", path.string());
            clearDirty();
        }
        else
        {
            m_statusMessage = Localization::get().text("status.failedToSaveSettings");
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
        m_editorSettings.uiLanguage = defaults.uiLanguage;
         
        markDirty();
        m_statusMessage = Localization::get().text("status.resetToDefaultsNotSaved");
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
            m_statusMessage = Localization::get().text("status.settingsValid");
        }
        else
        {
            m_statusMessage = Localization::get().text("status.settingsInvalid");
        }
    }

    void SettingsEditorWindow::renderContent()
    {
        auto& loc = Localization::get();
        if (isDirty())
        {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", loc.text("labels.unsavedChanges").data());
        }
 
        if (editors::renderGlobalSettingsEditor(m_editorSettings))
        {
            markDirty();
        }
 
        ImGui::Separator();
        if (ImGui::Button(loc.text("buttons.reload").data()))
        {
            loadSettings();
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.save").data()))
        {
            save();
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.resetToDefaults").data()))
        {
            resetToDefaults();
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.validate").data()))
        {
            validate();
        }
 
        if (!m_statusMessage.empty())
        {
            widgets::StatusText(std::string(loc.text("labels.status")) + ": " + m_statusMessage);
        }
 
        widgets::ValidationErrors(m_validationErrors);
    }
}

/**
 * @file settingsEditorWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "settingsEditorWindow.h"
#include "../widgets/basicWidgets.h"
#include "../core/localization.h"
#include "autoinput/config/configValidator.h"
#include "autoinput/config/config.h"
#include <imgui.h>

namespace autoinput::ui
{
    SettingsEditorWindow::SettingsEditorWindow(services::IAutomationRuntimeClient& runtimeClient)
        : UiWindow("Settings", "windows.settings")
        , m_runtimeClient(runtimeClient)
    {
        loadSettings();
    }

    void SettingsEditorWindow::onOpen()
    {
        loadSettings();
    }

    void SettingsEditorWindow::update()
    {
        if (m_isCapturing)
        {
            uint32_t currentCount = m_runtimeClient.getRecordedEventCount();
            if (currentCount > m_captureStartEventCount)
            {
                auto seq = m_runtimeClient.getRecordedSequence();
                stopCapture();

                if (seq && !seq->events.empty())
                {
                    std::string bestKey;
                    for (const auto& event : seq->events)
                    {
                        if (event.type == RecordedEventType::KeyDown && event.key.has_value())
                        {
                            std::string k = *event.key;
                            if (bestKey.empty()) bestKey = k;
                            if (k.find('+') != std::string::npos || 
                                (k.size() >= 2 && k[0] == 'f' && std::isdigit(k[1])) ||
                                (!k.empty() && std::isprint(static_cast<unsigned char>(k.back()))))
                            {
                                bestKey = k;
                                break;
                            }
                        }
                    }

                    if (!bestKey.empty())
                    {
                        m_editorSettings.endKey = bestKey;
                        markDirty();
                        m_statusMessage = Localization::get().format("status.capturedKey", bestKey);
                    }
                }
            }
        }
    }

    void SettingsEditorWindow::startCapture()
    {
        if (m_isCapturing) stopCapture();

        m_isCapturing = true;
        m_captureStartEventCount = m_runtimeClient.getRecordedEventCount();

        SequenceConfig config;
        config.recordKeyboardEvents = true;
        config.recordMouseMoves = false;
        config.recordMouseClicks = false;
        config.recordDelays = false;
        config.name = "CaptureHotkey";

        auto res = m_runtimeClient.startRecording(config);
        if (res.success)
        {
            m_statusMessage = Localization::get().text("status.pressAnyKey");
        }
        else
        {
            m_statusMessage = Localization::get().format("status.failedToStartCapture", res.message);
            m_isCapturing = false;
        }
    }

    void SettingsEditorWindow::stopCapture()
    {
        if (m_isCapturing)
        {
            m_runtimeClient.stopRecording();
            m_isCapturing = false;
        }
    }

    void SettingsEditorWindow::loadSettings()
    {
        m_settings.load();
        auto defaults = m_settings.getDefaults();
        
        m_editorSettings.endKey = defaults.end;
        m_editorSettings.application = defaults.application;
        m_editorSettings.blacklist = defaults.blacklist;
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
 
        bool wasCapturing = m_isCapturing;
        if (editors::renderGlobalSettingsEditor(m_editorSettings, m_isCapturing))
        {
            markDirty();
        }

        if (m_isCapturing && !wasCapturing)
        {
            startCapture();
        }
        else if (!m_isCapturing && wasCapturing)
        {
            stopCapture();
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

/**
* @file   settingsEditorWindow.cpp
* @brief  Implementation of the SettingsEditorWindow class.
 * @author djsquiddy
 * @date August 2026
 */
#include "settingsEditorWindow.h"
#include <imgui.h>
#include <imgui_stdlib.h>
#include "autoinput/configValidator.h"
#include "autoinput/config.h"
#include <algorithm>
#include <cstring>

namespace autoinput::ui
{
    namespace
    {
        const char* statusNotificationModes[] = { "off", "console", "desktop", "both" };
        const char* logLevels[] = { "debug", "info", "warning", "error" };
    }

    SettingsEditorWindow::SettingsEditorWindow()
    {
        loadSettings();
    }

    void SettingsEditorWindow::open()
    {
        if (!m_isOpen)
        {
            m_isOpen = true;
            loadSettings();
        }
        m_shouldFocus = true;
    }

    void SettingsEditorWindow::close()
    {
        if (m_isDirty)
        {
            m_showSaveConfirmation = true;
        }
        else
        {
            m_isOpen = false;
        }
    }

    void SettingsEditorWindow::loadSettings()
    {
        m_settings.load();
        m_draft = m_settings.getDefaults();
        syncBuffersFromDraft();
        m_isDirty = false;
        m_statusMessage = "Settings loaded.";
        m_validationErrors.clear();
    }

    void SettingsEditorWindow::saveSettings(bool toUserFile)
    {
        syncDraftFromBuffers();
        m_settings.setDefaults(m_draft);

        std::filesystem::path path;
        if (toUserFile) {
            path = autoinput::getUserConfigsPath() / "settings.toml";
            std::filesystem::create_directories(path.parent_path());
        } else {
            path = autoinput::getConfigsPath() / "settings.toml";
        }

        if (m_settings.save(path)) {
            m_statusMessage = "Settings saved to " + path.string();
            m_isDirty = false;
        } else {
            m_statusMessage = "Failed to save settings to " + path.string();
        }
    }

    void SettingsEditorWindow::resetToDefaults()
    {
        m_draft = autoinput::DefaultSettings{};
        syncBuffersFromDraft();
        m_isDirty = true;
        m_statusMessage = "Reset to defaults (not saved yet).";
    }

    void SettingsEditorWindow::validate()
    {
        syncDraftFromBuffers();
        autoinput::ConfigData tempConfig;
        tempConfig.endKey = m_draft.end;
        tempConfig.application = m_draft.application;
        tempConfig.blacklist = m_draft.blacklist;
        tempConfig.appendBlacklist = m_draft.appendBlacklist;
        tempConfig.statusNotificationMode = m_draft.statusNotificationMode;
        tempConfig.logLevel = m_draft.logLevel;

        m_validationErrors = autoinput::validateConfigData(tempConfig);
        if (m_validationErrors.empty()) {
            m_statusMessage = "Settings are valid.";
        } else {
            m_statusMessage = "Settings have validation errors.";
        }
    }

    void SettingsEditorWindow::syncBuffersFromDraft()
    {
        std::strncpy(m_buffers.end, m_draft.end.c_str(), sizeof(m_buffers.end) - 1);
        std::strncpy(m_buffers.application, m_draft.application.c_str(), sizeof(m_buffers.application) - 1);

        m_buffers.blacklist = m_draft.blacklist;
        m_buffers.appendBlacklist = m_draft.appendBlacklist;

        auto itStatus = std::find(std::begin(statusNotificationModes), std::end(statusNotificationModes), m_draft.statusNotificationMode);
        if (itStatus != std::end(statusNotificationModes))
        {
            m_buffers.statusNotificationModeIdx = static_cast<int>(std::distance(std::begin(statusNotificationModes), itStatus));
        }
        else
        {
            m_buffers.statusNotificationModeIdx = 0;
        }

        auto itLog = std::find(std::begin(logLevels), std::end(logLevels), m_draft.logLevel);
        if (itLog != std::end(logLevels))
        {
            m_buffers.logLevelIdx = static_cast<int>(std::distance(std::begin(logLevels), itLog));
        }
        else
        {
            m_buffers.logLevelIdx = 1; // info
        }
    }

    void SettingsEditorWindow::syncDraftFromBuffers()
    {
        m_draft.end = m_buffers.end;
        m_draft.application = m_buffers.application;
        m_draft.blacklist = m_buffers.blacklist;
        m_draft.appendBlacklist = m_buffers.appendBlacklist;
        m_draft.statusNotificationMode = statusNotificationModes[m_buffers.statusNotificationModeIdx];
        m_draft.logLevel = logLevels[m_buffers.logLevelIdx];
    }

    void SettingsEditorWindow::render()
    {
        if (!m_isOpen && !m_showSaveConfirmation)
        {
            return;
        }

        if (m_shouldFocus)
        {
            ImGui::SetNextWindowFocus();
            m_shouldFocus = false;
        }

        bool openBefore = m_isOpen;
        if (ImGui::Begin("Settings", &m_isOpen, ImGuiWindowFlags_AlwaysAutoResize))
        {
            if (m_isDirty)
            {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "You have unsaved changes!");
            }

            if (ImGui::InputText("End Hotkey", m_buffers.end, sizeof(m_buffers.end)))
            {
                m_isDirty = true;
            }

            if (ImGui::InputText("Application", m_buffers.application, sizeof(m_buffers.application)))
            {
                m_isDirty = true;
            }

            if (ImGui::Checkbox("Append Blacklist", &m_buffers.appendBlacklist))
            {
                m_isDirty = true;
            }

            // Blacklist Editor
            ImGui::Separator();
            ImGui::Text("Blacklist (Application names)");
            for (size_t i = 0; i < m_buffers.blacklist.size(); ++i)
            {
                ImGui::PushID(static_cast<int>(i));
                if (ImGui::InputText("##item", &m_buffers.blacklist[i]))
                {
                    m_isDirty = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Remove"))
                {
                    m_buffers.blacklist.erase(m_buffers.blacklist.begin() + i);
                    m_isDirty = true;
                }
                ImGui::PopID();
            }
            if (ImGui::Button("Add Application"))
            {
                m_buffers.blacklist.emplace_back("");
                m_isDirty = true;
            }

            ImGui::Separator();
            if (ImGui::Combo("Notification Mode", &m_buffers.statusNotificationModeIdx, statusNotificationModes, IM_ARRAYSIZE(statusNotificationModes)))
            {
                m_isDirty = true;
            }
            if (ImGui::Combo("Log Level", &m_buffers.logLevelIdx, logLevels, IM_ARRAYSIZE(logLevels)))
            {
                m_isDirty = true;
            }

            ImGui::Separator();
            if (ImGui::Button("Load"))
            {
                loadSettings();
            }
            ImGui::SameLine();
            if (ImGui::Button("Reload"))
            {
                loadSettings();
            }
            ImGui::SameLine();
            if (ImGui::Button("Save"))
            {
                saveSettings(true);
            }
            ImGui::SameLine();
            if (ImGui::Button("Save As User Settings"))
            {
                saveSettings(true);
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
                ImGui::Text("Status: %s", m_statusMessage.c_str());
            }

            if (!m_validationErrors.empty())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Validation Errors:");
                for (const auto& error : m_validationErrors) {

                    ImGui::BulletText("%s", error.message.c_str());
                }
            }
        }
        ImGui::End();

        if (openBefore && !m_isOpen && m_isDirty)
        {
            m_isOpen = true;
            m_showSaveConfirmation = true;
        }

        if (m_showSaveConfirmation)
        {
            ImGui::OpenPopup("Save Changes?");
            if (ImGui::BeginPopupModal("Save Changes?", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
            {
                ImGui::Text("You have unsaved changes. Do you want to save them before closing?");
                ImGui::Separator();

                if (ImGui::Button("Save", ImVec2(120, 0)))
                {
                    saveSettings(true);
                    m_isOpen = false;
                    m_showSaveConfirmation = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Discard", ImVec2(120, 0)))
                {
                    m_isDirty = false;
                    m_isOpen = false;
                    m_showSaveConfirmation = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel", ImVec2(120, 0)))
                {
                    m_showSaveConfirmation = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
        }
    }
}

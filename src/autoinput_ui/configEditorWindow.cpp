/**
 * @file   configEditorWindow.cpp
* @brief  Implementation of the ConfigEditorWindow class.
 * @author djsquiddy
 * @date August 2026
 */
#include "configEditorWindow.h"
#include <imgui.h>
#include <imgui_stdlib.h>
#include "autoinput/configValidator.h"
#include "autoinput/logger.h"
#include <algorithm>

namespace autoinput::ui
{
    namespace
    {
        const char* actionNames[] = { "click", "hold" };
        const char* statusNotificationModes[] = { "off", "console", "desktop", "both" };
        const char* logLevels[] = { "debug", "info", "warning", "error" };
    }

    ConfigEditorWindow::ConfigEditorWindow()
    {
        refreshConfigList();
    }

    void ConfigEditorWindow::open()
    {
        if (!m_isOpen)
        {
            m_isOpen = true;
            refreshConfigList();
        }
        m_shouldFocus = true;
    }

    void ConfigEditorWindow::close()
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

    void ConfigEditorWindow::refreshConfigList()
    {
        m_availableConfigs = autoinput::listAvailableConfigs();
    }

    void ConfigEditorWindow::loadConfig(const std::string& nameOrPath)
    {
        auto path = autoinput::getConfigFilePath(nameOrPath);
        auto data = autoinput::loadConfigData(path);
        if (data)
        {
            m_draft = std::move(*data);
            m_currentConfigName = nameOrPath;
            m_currentConfigPath = path;
            m_isDirty = false;
            m_statusMessage = "Loaded " + path.string();
            m_validationErrors.clear();
        }
        else
        {
            m_statusMessage = "Failed to load " + nameOrPath;
        }
    }

    void ConfigEditorWindow::saveConfig(bool forceUser)
    {
        std::filesystem::path path = m_currentConfigPath;
        if (forceUser || path.empty() || path.string().find("configs") != std::string::npos)
        {
            // If it's a system config or new, save to user directory
            path = autoinput::getUserConfigsPath() / (m_currentConfigName + ".toml");
            std::filesystem::create_directories(path.parent_path());
        }

        if (autoinput::saveConfigData(m_draft, path))
        {
            m_currentConfigPath = path;
            m_isDirty = false;
            m_statusMessage = "Saved to " + path.string();
            refreshConfigList();
        }
        else
        {
            m_statusMessage = "Failed to save to " + path.string();
        }
    }

    void ConfigEditorWindow::validate()
    {
        m_validationErrors = autoinput::validateConfigData(m_draft);
        if (m_validationErrors.empty()) {
            m_statusMessage = "Configuration is valid.";
        } else {
            m_statusMessage = "Configuration has validation errors.";
        }
    }

    void ConfigEditorWindow::createNewConfig()
    {
        m_draft = autoinput::ConfigData{};
        m_currentConfigName = "new_config";
        m_currentConfigPath = "";
        m_isDirty = true;
        m_statusMessage = "New configuration created.";
        m_validationErrors.clear();
    }

    void ConfigEditorWindow::duplicateConfig()
    {
        m_currentConfigName += "_copy";
        m_currentConfigPath = "";
        m_isDirty = true;
        m_statusMessage = "Configuration duplicated (renamed).";
    }

    void ConfigEditorWindow::render()
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
        if (ImGui::Begin("Config Editor", &m_isOpen))
        {
            // Toolbar
            if (ImGui::Button("New"))
            {
                createNewConfig();
            }
            ImGui::SameLine();
            if (ImGui::Button("Refresh List"))
            {
                refreshConfigList();
            }

            ImGui::SameLine();
            ImGui::SetNextItemWidth(200);
            if (ImGui::BeginCombo("Load Config", m_currentConfigName.c_str()))
            {
                for (const auto& name : m_availableConfigs)
                {
                    if (ImGui::Selectable(name.c_str(), m_currentConfigName == name))
                    {
                        loadConfig(name);
                    }
                }
                ImGui::EndCombo();
            }

            ImGui::Separator();

            if (m_isDirty)
            {
                ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Unsaved changes in [%s]", m_currentConfigName.c_str());
            }
            else
            {
                ImGui::Text("Editing [%s]", m_currentConfigName.c_str());
            }

            if (ImGui::InputText("Config Name", &m_currentConfigName))
            {
                m_isDirty = true;
            }

            if (ImGui::BeginTabBar("ConfigTabs"))
            {
                if (ImGui::BeginTabItem("Global Settings"))
                {
                    renderGlobalSettings();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Commands"))
                {
                    renderCommands();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("Sequences (Read-only)"))
                {
                    renderSequences();
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }

            ImGui::Separator();
            if (ImGui::Button("Save"))
            {
                saveConfig(false);
            }
            ImGui::SameLine();
            if (ImGui::Button("Save As User"))
            {
                saveConfig(true);
            }
            ImGui::SameLine();
            if (ImGui::Button("Duplicate"))
            {
                duplicateConfig();
            }
            ImGui::SameLine();
            if (ImGui::Button("Validate"))
            {
                validate();
            }
            ImGui::SameLine();
            if (ImGui::Button("Reload"))
            {
                if (!m_currentConfigName.empty()) loadConfig(m_currentConfigName);
            }

            if (!m_statusMessage.empty())
            {
                ImGui::Text("Status: %s", m_statusMessage.c_str());
            }

            if (!m_validationErrors.empty())
            {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Validation Errors:");
                for (const auto& error : m_validationErrors)
                {
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
                ImGui::Text("You have unsaved changes in [%s]. Do you want to save them before closing?", m_currentConfigName.c_str());
                ImGui::Separator();

                if (ImGui::Button("Save", ImVec2(120, 0)))
                {
                    saveConfig(false);
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

    void ConfigEditorWindow::renderGlobalSettings() {
        if (ImGui::InputText("End Hotkey", &m_draft.endKey)) m_isDirty = true;
        if (ImGui::InputText("Application", &m_draft.application)) m_isDirty = true;
        if (ImGui::Checkbox("Append Blacklist", &m_draft.appendBlacklist)) m_isDirty = true;

        ImGui::Text("Blacklist:");
        for (size_t i = 0; i < m_draft.blacklist.size(); ++i)
        {
            ImGui::PushID(static_cast<int>(i));
            if (ImGui::InputText("##item", &m_draft.blacklist[i])) m_isDirty = true;
            ImGui::SameLine();
            if (ImGui::Button("Remove")) {
                m_draft.blacklist.erase(m_draft.blacklist.begin() + i);
                m_isDirty = true;
            }
            ImGui::PopID();
        }
        if (ImGui::Button("Add to Blacklist"))
        {
            m_draft.blacklist.emplace_back("");
            m_isDirty = true;
        }

        // Combo for notification mode
        int modeIdx = 0;
        for (int i = 0; i < 4; ++i) if (m_draft.statusNotificationMode == statusNotificationModes[i]) modeIdx = i;
        if (ImGui::Combo("Notification Mode", &modeIdx, statusNotificationModes, 4))
        {
            m_draft.statusNotificationMode = statusNotificationModes[modeIdx];
            m_isDirty = true;
        }

        // Combo for log level
        int levelIdx = 1;
        for (int i = 0; i < 4; ++i) if (m_draft.logLevel == logLevels[i]) levelIdx = i;
        if (ImGui::Combo("Log Level", &levelIdx, logLevels, 4))
        {
            m_draft.logLevel = logLevels[levelIdx];
            m_isDirty = true;
        }
    }

    void ConfigEditorWindow::renderCommands()
    {
        if (ImGui::Button("Add Command"))
        {
            m_draft.commands.emplace_back(autoinput::CommandData{ .name = "new_command" });
            m_isDirty = true;
        }

        for (size_t i = 0; i < m_draft.commands.size(); ++i)
        {
            auto& cmd = m_draft.commands[i];
            ImGui::PushID(static_cast<int>(i));

            char label[128];
            snprintf(label, sizeof(label), "Command %zu: %s", i, cmd.name.c_str());

            if (ImGui::CollapsingHeader(label))
            {
                if (ImGui::InputText("Name", &cmd.name)) m_isDirty = true;
                if (ImGui::InputText("Exclusive Group", &cmd.exclusiveGroup)) m_isDirty = true;

                int actionIdx = (cmd.action == "hold") ? 1 : 0;
                if (ImGui::Combo("Action", &actionIdx, actionNames, 2))
                {
                    cmd.action = actionNames[actionIdx];
                    m_isDirty = true;
                }

                // Buttons
                if (ImGui::TreeNode("Buttons"))
                {
                    for (size_t j = 0; j < cmd.buttons.size(); ++j)
                    {
                        ImGui::PushID(static_cast<int>(j));
                        if (ImGui::InputText("##btn", &cmd.buttons[j])) m_isDirty = true;
                        ImGui::SameLine();
                        if (ImGui::Button("Remove"))
                        {
                            cmd.buttons.erase(cmd.buttons.begin() + j);
                            m_isDirty = true;
                        }
                        ImGui::PopID();
                    }
                    if (ImGui::Button("Add Button"))
                    {
                        cmd.buttons.emplace_back("");
                        m_isDirty = true;
                    }
                    ImGui::TreePop();
                }

                // Keys
                if (ImGui::TreeNode("Keys"))
                {
                    for (size_t j = 0; j < cmd.keys.size(); ++j)
                    {
                        ImGui::PushID(static_cast<int>(j));
                        if (ImGui::InputText("##key", &cmd.keys[j])) m_isDirty = true;
                        ImGui::SameLine();
                        if (ImGui::Button("Remove"))
                        {
                            cmd.keys.erase(cmd.keys.begin() + j);
                            m_isDirty = true;
                        }
                        ImGui::PopID();
                    }
                    if (ImGui::Button("Add Key"))
                    {
                        cmd.keys.emplace_back("");
                        m_isDirty = true;
                    }
                    ImGui::TreePop();
                }

                // Timing
                ImGui::Text("Timing:");
                if (ImGui::InputText("Press Wait (e.g. 25ms, 100ms..200ms)", &cmd.pressWait)) m_isDirty = true;
                if (ImGui::InputText("Release Wait", &cmd.releaseWait)) m_isDirty = true;

                if (ImGui::Button("Duplicate Command"))
                {
                    m_draft.commands.insert(m_draft.commands.begin() + i + 1, cmd);
                    m_isDirty = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Remove Command"))
                {
                    m_draft.commands.erase(m_draft.commands.begin() + i);
                    m_isDirty = true;
                }
                ImGui::SameLine();
                if (i > 0 && ImGui::Button("Move Up"))
                {
                    std::swap(m_draft.commands[i], m_draft.commands[i-1]);
                    m_isDirty = true;
                }
                ImGui::SameLine();
                if (i < m_draft.commands.size() - 1 && ImGui::Button("Move Down"))
                {
                    std::swap(m_draft.commands[i], m_draft.commands[i+1]);
                    m_isDirty = true;
                }
            }
            ImGui::PopID();
        }
    }

    void ConfigEditorWindow::renderSequences()
    {
        ImGui::Text("Recorded Sequences (View Only):");
        for (const auto& seq : m_draft.sequences)
        {
            if (ImGui::CollapsingHeader(seq.name.c_str()))
            {
                ImGui::Text("Start Hotkey: %s", seq.start.c_str());
                ImGui::Text("Repeat: %s", seq.repeat ? "Yes" : "No");
                ImGui::Text("Events: %zu", seq.events.size());
            }
        }
        ImGui::Text("Full sequence editing TODO.");
    }
}
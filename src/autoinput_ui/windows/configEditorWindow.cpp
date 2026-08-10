/**
 * @file configEditorWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "configEditorWindow.h"
#include "../widgets/basicWidgets.h"
#include "../widgets/formWidgets.h"
#include "../editors/commandEditor.h"
#include "../editors/sequenceViewer.h"
#include "autoinput/configValidator.h"
#include "autoinput/logger.h"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <algorithm>

namespace autoinput::ui
{
    ConfigEditorWindow::ConfigEditorWindow()
        : UiWindow("Config Editor", "windows.configEditor")
    {
        refreshConfigList();
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
            clearDirty();
            m_statusMessage = "Loaded " + path.string();
            m_validationErrors.clear();
        }
        else
        {
            m_statusMessage = "Failed to load " + nameOrPath;
        }
    }

    void ConfigEditorWindow::save()
    {
        saveConfig(false);
    }

    void ConfigEditorWindow::saveConfig(bool forceUser)
    {
        std::filesystem::path path = m_currentConfigPath;
        if (forceUser || path.empty() || path.string().find("configs") != std::string::npos)
        {
            path = autoinput::getUserConfigsPath() / (m_currentConfigName + ".toml");
            std::filesystem::create_directories(path.parent_path());
        }

        if (autoinput::saveConfigData(m_draft, path))
        {
            m_currentConfigPath = path;
            clearDirty();
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
        if (m_validationErrors.empty())
        {
            m_statusMessage = "Configuration is valid.";
        }
        else
        {
            m_statusMessage = "Configuration has validation errors.";
        }
    }

    void ConfigEditorWindow::createNewConfig()
    {
        m_draft = autoinput::ConfigData{};
        m_currentConfigName = "new_config";
        m_currentConfigPath = "";
        markDirty();
        m_statusMessage = "New configuration created.";
        m_validationErrors.clear();
    }

    void ConfigEditorWindow::duplicateConfig()
    {
        m_currentConfigName += "_copy";
        m_currentConfigPath = "";
        markDirty();
        m_statusMessage = "Configuration duplicated (renamed).";
    }

    void ConfigEditorWindow::renderContent()
    {
        renderToolbar();
        
        ImGui::Separator();

        if (isDirty())
        {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Unsaved changes in [%s]", m_currentConfigName.c_str());
        }
        else
        {
            ImGui::Text("Editing [%s]", m_currentConfigName.c_str());
        }

        if (widgets::StringInput("Config Name", m_currentConfigName))
        {
            markDirty();
        }

        renderTabs();

        ImGui::Separator();
        if (ImGui::Button("Save")) saveConfig(false);
        ImGui::SameLine();
        if (ImGui::Button("Save As User")) saveConfig(true);
        ImGui::SameLine();
        if (ImGui::Button("Duplicate")) duplicateConfig();
        ImGui::SameLine();
        if (ImGui::Button("Validate")) validate();
        ImGui::SameLine();
        if (ImGui::Button("Reload"))
        {
            if (!m_currentConfigName.empty())
            {
                loadConfig(m_currentConfigName);
            }
        }

        if (!m_statusMessage.empty())
        {
            widgets::StatusText("Status: " + m_statusMessage);
        }

        widgets::ValidationErrors(m_validationErrors);
    }

    void ConfigEditorWindow::renderToolbar()
    {
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
    }

    void ConfigEditorWindow::renderTabs()
    {
        if (ImGui::BeginTabBar("ConfigTabs"))
        {
            if (ImGui::BeginTabItem("Global Settings"))
            {
                renderGlobalSettingsTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Commands"))
            {
                renderCommandsTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Sequences (Read-only)"))
            {
                renderSequencesTab();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }

    void ConfigEditorWindow::renderGlobalSettingsTab()
    {
        editors::GlobalSettings settings;
        settings.endKey = m_draft.endKey;
        settings.application = m_draft.application;
        settings.blacklist = m_draft.blacklist;
        settings.appendBlacklist = m_draft.appendBlacklist;
        settings.statusNotificationMode = m_draft.statusNotificationMode;
        settings.logLevel = m_draft.logLevel;

        if (editors::renderGlobalSettingsEditor(settings))
        {
            m_draft.endKey = settings.endKey;
            m_draft.application = settings.application;
            m_draft.blacklist = settings.blacklist;
            m_draft.appendBlacklist = settings.appendBlacklist;
            m_draft.statusNotificationMode = settings.statusNotificationMode;
            m_draft.logLevel = settings.logLevel;
            markDirty();
        }
    }

    void ConfigEditorWindow::renderCommandsTab()
    {
        if (ImGui::Button("Add Command"))
        {
            m_draft.commands.emplace_back(autoinput::CommandData{ .name = "new_command" });
            markDirty();
        }

        for (size_t i = 0; i < m_draft.commands.size(); ++i)
        {
            auto& cmd = m_draft.commands[i];
            ImGui::PushID(static_cast<int>(i));

            std::string label = "Command " + std::to_string(i) + ": " + cmd.name;
            if (ImGui::CollapsingHeader(label.c_str()))
            {
                if (editors::renderCommandEditor(cmd))
                {
                    markDirty();
                }

                if (ImGui::Button("Duplicate Command"))
                {
                    m_draft.commands.insert(m_draft.commands.begin() + i + 1, cmd);
                    markDirty();
                }
                ImGui::SameLine();
                if (ImGui::Button("Remove Command"))
                {
                    m_draft.commands.erase(m_draft.commands.begin() + i);
                    markDirty();
                    ImGui::PopID();
                    break;
                }
                ImGui::SameLine();
                if (i > 0 && ImGui::Button("Move Up"))
                {
                    std::swap(m_draft.commands[i], m_draft.commands[i-1]);
                    markDirty();
                }
                ImGui::SameLine();
                if (i < m_draft.commands.size() - 1 && ImGui::Button("Move Down"))
                {
                    std::swap(m_draft.commands[i], m_draft.commands[i+1]);
                    markDirty();
                }
            }
            ImGui::PopID();
        }
    }

    // ReSharper disable once CppMemberFunctionMayBeConst
    void ConfigEditorWindow::renderSequencesTab() // NOLINT(*-make-member-function-const)
    {
        editors::renderSequenceViewer(m_draft.sequences);
    }
}

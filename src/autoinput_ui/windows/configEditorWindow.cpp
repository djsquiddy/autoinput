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
#include "../core/localization.h"
#include "autoinput/config/configValidator.h"
#include "autoinput/support/logger.h"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <algorithm>

namespace autoinput::ui
{
    ConfigEditorWindow::ConfigEditorWindow(services::IAutomationRuntimeClient& runtimeClient)
        : UiWindow("Config Editor", "windows.configEditor")
        , m_runtimeClient(runtimeClient)
    {
        refreshConfigList();
    }

    void ConfigEditorWindow::update()
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
                        auto& loc = Localization::get();
                        if (m_isCapturingEndKey)
                        {
                            m_draft.endKey = bestKey;
                        }
                        else if (m_captureCommandIndex >= 0 && m_captureCommandIndex < static_cast<int>(m_draft.commands.size()))
                        {
                            auto& cmd = m_draft.commands[m_captureCommandIndex];
                            if (m_captureStartKeyIndex >= 0 && m_captureStartKeyIndex < static_cast<int>(cmd.startKeys.size()))
                            {
                                cmd.startKeys[m_captureStartKeyIndex] = bestKey;
                            }
                        }
                        
                        m_statusMessage = loc.format("status.capturedKey", bestKey);
                        markDirty();
                    }
                }
                
                m_captureCommandIndex = -1;
                m_captureStartKeyIndex = -1;
                m_isCapturingEndKey = false;
            }
        }
    }

    void ConfigEditorWindow::startCapture()
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

    void ConfigEditorWindow::stopCapture()
    {
        if (m_isCapturing)
        {
            m_runtimeClient.stopRecording();
            m_isCapturing = false;
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
            auto& loc = Localization::get();
            m_draft = std::move(*data);
            m_currentConfigName = nameOrPath;
            m_currentConfigPath = path;
            clearDirty();
            m_statusMessage = loc.format("status.configLoadedPath", path.string());
            m_validationErrors.clear();
        }
        else
        {
            m_statusMessage = Localization::get().format("status.configLoadFailedPath", nameOrPath);
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
            auto& loc = Localization::get();
            m_currentConfigPath = path;
            clearDirty();
            m_statusMessage = loc.format("status.configSavedTo", path.string());
            refreshConfigList();
        }
        else
        {
            m_statusMessage = Localization::get().format("status.configSaveFailedTo", path.string());
        }
    }

    void ConfigEditorWindow::validate()
    {
        auto& loc = Localization::get();
        m_validationErrors = autoinput::validateConfigData(m_draft);
        if (m_validationErrors.empty())
        {
            m_statusMessage = loc.text("status.configValid");
        }
        else
        {
            m_statusMessage = loc.text("status.configInvalid");
        }
    }

    void ConfigEditorWindow::createNewConfig()
    {
        m_draft = autoinput::ConfigData{};
        m_currentConfigName = "new_config";
        m_currentConfigPath = "";
        markDirty();
        m_statusMessage = Localization::get().text("status.newConfigCreated");
        m_validationErrors.clear();
    }

    void ConfigEditorWindow::duplicateConfig()
    {
        m_currentConfigName += "_copy";
        m_currentConfigPath = "";
        markDirty();
        m_statusMessage = Localization::get().text("status.configDuplicated");
    }

    void ConfigEditorWindow::renderContent()
    {
        auto& loc = Localization::get();
        renderToolbar();
        
        ImGui::Separator();
 
        if (isDirty())
        {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", loc.format("status.unsavedChangesIn", m_currentConfigName).c_str());
        }
        else if (!m_currentConfigName.empty())
        {
            ImGui::Text("%s", loc.format("status.editingConfig", m_currentConfigName).c_str());
        }
 
        if (widgets::StringInput(loc.text("labels.configName").data(), m_currentConfigName))
        {
            markDirty();
        }

        renderTabs();
 
        ImGui::Separator();
        if (ImGui::Button(loc.text("labels.actions").data()))
        {
            ImGui::OpenPopup("ConfigActionsPopup");
        }
        
        if (ImGui::BeginPopup("ConfigActionsPopup"))
        {
            if (ImGui::MenuItem(loc.text("buttons.save").data())) saveConfig(false);
            if (ImGui::MenuItem(loc.text("buttons.saveAsUser").data())) saveConfig(true);
            if (ImGui::MenuItem(loc.text("buttons.duplicate").data())) duplicateConfig();
            ImGui::Separator();
            if (ImGui::MenuItem(loc.text("buttons.validate").data())) validate();
            if (ImGui::MenuItem(loc.text("buttons.reload").data()))
            {
                if (!m_currentConfigName.empty()) loadConfig(m_currentConfigName);
            }
            ImGui::EndPopup();
        }

        if (!m_statusMessage.empty())
        {
            widgets::StatusText(std::string(loc.text("labels.status")) + ": " + m_statusMessage);
        }

        widgets::ValidationErrors(m_validationErrors);
    }

    void ConfigEditorWindow::renderToolbar()
    {
        auto& loc = Localization::get();
        if (ImGui::Button(loc.text("buttons.new").data()))
        {
            createNewConfig();
        }
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.refresh").data()))
        {
            refreshConfigList();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(200);
        if (ImGui::BeginCombo(loc.text("actions.loadConfig").data(), m_currentConfigName.c_str()))
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
        auto& loc = Localization::get();
        if (ImGui::BeginTabBar("ConfigTabs"))
        {
            if (ImGui::BeginTabItem(loc.text("labels.globalSettings").data()))
            {
                renderGlobalSettingsTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(loc.text("labels.commands").data()))
            {
                renderCommandsTab();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem(loc.text("labels.sequencesReadOnly").data()))
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
        settings.statusNotificationMode = m_draft.statusNotificationMode;
        settings.logLevel = m_draft.logLevel;

        bool captureState = m_isCapturing && m_isCapturingEndKey;
        bool wasCapturing = captureState;
        if (editors::renderGlobalSettingsEditor(settings, captureState))
        {
            m_draft.endKey = settings.endKey;
            m_draft.application = settings.application;
            m_draft.blacklist = settings.blacklist;
            m_draft.statusNotificationMode = settings.statusNotificationMode;
            m_draft.logLevel = settings.logLevel;
            markDirty();
        }

        if (captureState && !wasCapturing)
        {
            m_isCapturingEndKey = true;
            startCapture();
        }
        else if (!captureState && wasCapturing)
        {
            stopCapture();
            m_isCapturingEndKey = false;
        }
    }

    void ConfigEditorWindow::renderCommandsTab()
    {
        auto& loc = Localization::get();
        if (ImGui::Button(loc.text("buttons.add").data()))
        {
            ImGui::OpenPopup("AddElementPopup");
        }
        
        if (ImGui::BeginPopup("AddElementPopup"))
        {
            if (ImGui::MenuItem(loc.text("labels.command").data()))
            {
                m_draft.commands.emplace_back(autoinput::CommandData{ .name = "new_command" });
                markDirty();
            }
            if (ImGui::MenuItem(loc.text("labels.sequence").data()))
            {
                m_draft.sequences.emplace_back(autoinput::RecordedSequence{ .name = "new_sequence" });
                markDirty();
            }
            ImGui::EndPopup();
        }

        for (size_t i = 0; i < m_draft.commands.size(); ++i)
        {
            auto& cmd = m_draft.commands[i];
            ImGui::PushID(static_cast<int>(i));

            std::string label = loc.format("labels.commandLabel", i, cmd.name);
            label += "###command_" + std::to_string(i);
            if (ImGui::CollapsingHeader(label.c_str()))
            {
                int captureIndex = (m_captureCommandIndex == static_cast<int>(i)) ? m_captureStartKeyIndex : -1;
                int originalCaptureIndex = captureIndex;
                if (editors::renderCommandEditor(cmd, captureIndex))
                {
                    markDirty();
                }
                
                if (captureIndex != originalCaptureIndex)
                {
                    if (captureIndex != -1)
                    {
                        m_captureCommandIndex = static_cast<int>(i);
                        m_captureStartKeyIndex = captureIndex;
                        startCapture();
                    }
                    else
                    {
                        stopCapture();
                        m_captureCommandIndex = -1;
                        m_captureStartKeyIndex = -1;
                    }
                }

                if (ImGui::Button(loc.text("labels.actions").data()))
                {
                    ImGui::OpenPopup("CommandActionsPopup");
                }
                
                if (ImGui::BeginPopup("CommandActionsPopup"))
                {
                    if (ImGui::MenuItem(loc.text("buttons.duplicate").data()))
                    {
                        m_draft.commands.insert(m_draft.commands.begin() + i + 1, cmd);
                        markDirty();
                    }
                    if (ImGui::MenuItem(loc.text("buttons.remove").data()))
                    {
                        m_draft.commands.erase(m_draft.commands.begin() + i);
                        markDirty();
                        ImGui::EndPopup();
                        ImGui::PopID();
                        break;
                    }
                    ImGui::Separator();
                    if (i > 0 && ImGui::MenuItem(loc.text("buttons.moveUp").data()))
                    {
                        std::swap(m_draft.commands[i], m_draft.commands[i-1]);
                        markDirty();
                    }
                    if (i < m_draft.commands.size() - 1 && ImGui::MenuItem(loc.text("buttons.moveDown").data()))
                    {
                        std::swap(m_draft.commands[i], m_draft.commands[i+1]);
                        markDirty();
                    }
                    ImGui::EndPopup();
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

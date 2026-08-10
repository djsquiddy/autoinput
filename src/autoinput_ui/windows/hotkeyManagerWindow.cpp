/**
 * @file hotkeyManagerWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "hotkeyManagerWindow.h"
#include "../widgets/basicWidgets.h"
#include "../core/windowIds.h"
#include "../core/localization.h"
#include "autoinput/logger.h"
#include "autoinput/services/configService.h"
#include "autoinput/configValidator.h"
#include "../core/windowManager.h"
#include "configEditorWindow.h"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <format>
#include <algorithm>
#include <unordered_map>

namespace autoinput::ui
{
    HotkeyManagerWindow::HotkeyManagerWindow(WindowManager& windowManager, services::IAutomationRuntimeClient& runtimeClient, const IEnvironment& environment)
        : UiWindow("Hotkey Manager", "windows.hotkeyManager")
        , m_windowManager(windowManager)
        , m_runtimeClient(runtimeClient)
        , m_environment(environment)
    {
        refreshConfigs();
        if (!m_availableConfigs.empty())
        {
            loadSelectedConfig();
        }
    }

    void HotkeyManagerWindow::refreshConfigs()
    {
        m_availableConfigs = listAvailableConfigs();
        if (m_availableConfigs.empty())
        {
            m_availableConfigs.push_back("default");
        }
        
        if (m_selectedConfigIndex >= static_cast<int>(m_availableConfigs.size()))
        {
            m_selectedConfigIndex = 0;
        }
    }

    void HotkeyManagerWindow::loadSelectedConfig()
    {
        m_currentConfigName = m_availableConfigs[m_selectedConfigIndex];
        const auto configPath = getConfigFilePath(m_currentConfigName, m_environment);
        
        m_configData = loadConfigData(configPath);
        m_entries.clear();
        
        if (m_configData)
        {
            // Commands
            for (size_t i = 0; i < m_configData->commands.size(); ++i)
            {
                const auto& cmd = m_configData->commands[i];
                HotkeyEntry entry;
                entry.type = HotkeyEntry::Type::Command;
                entry.name = cmd.name;
                entry.hotkey = cmd.startKeys.empty() ? "" : cmd.startKeys[0];
                entry.index = i;
                m_entries.push_back(std::move(entry));
            }
            
            // Sequences
            for (size_t i = 0; i < m_configData->sequences.size(); ++i)
            {
                const auto& seq = m_configData->sequences[i];
                HotkeyEntry entry;
                entry.type = HotkeyEntry::Type::Sequence;
                entry.name = seq.name;
                entry.hotkey = seq.start;
                entry.index = i;
                m_entries.push_back(std::move(entry));
            }
            
            // Global End Key
            HotkeyEntry endEntry;
            endEntry.type = HotkeyEntry::Type::GlobalEnd;
            endEntry.name = "Global End Key";
            endEntry.hotkey = m_configData->endKey;
            endEntry.index = 0;
            m_entries.push_back(std::move(endEntry));
            
            validateHotkeys();
        }
        else
        {
            m_statusMessage = std::format("Failed to load config: {}", m_currentConfigName);
        }
        clearDirty();
    }

    void HotkeyManagerWindow::validateHotkeys()
    {
        std::unordered_map<std::string, int> hotkeyCounts;
        for (auto& entry : m_entries)
        {
            entry.hasConflict = false;
            entry.isValid = true;
            
            if (entry.hotkey.empty()) continue;
            
            hotkeyCounts[entry.hotkey]++;
            
            // Validate key name (very basic check, could use Key::fromString)
            try {
                auto k = Key::fromString(entry.hotkey);
                if (k.character.empty() && k.modifier == KeyModifier::None)
                {
                    entry.isValid = false;
                }
            } catch (...) {
                entry.isValid = false;
            }
        }
        
        for (auto& entry : m_entries)
        {
            if (!entry.hotkey.empty() && hotkeyCounts[entry.hotkey] > 1)
            {
                entry.hasConflict = true;
            }
        }
    }

    void HotkeyManagerWindow::startCapture(HotkeyEntry& entry)
    {
        if (m_isCapturing) stopCapture();
        
        m_isCapturing = true;
        m_captureTarget = &entry;
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
            m_statusMessage = "Press any key combination...";
        }
        else
        {
            m_statusMessage = std::format("Failed to start capture: {}", res.message);
            m_isCapturing = false;
            m_captureTarget = nullptr;
        }
    }

    void HotkeyManagerWindow::stopCapture()
    {
        if (m_isCapturing)
        {
            m_runtimeClient.stopRecording();
            m_isCapturing = false;
            m_captureTarget = nullptr;
        }
    }

    void HotkeyManagerWindow::update()
    {
        if (m_isCapturing && m_captureTarget)
        {
            uint32_t currentCount = m_runtimeClient.getRecordedEventCount();
            if (currentCount > m_captureStartEventCount)
            {
                applyCapturedHotkey();
            }
        }
    }

    void HotkeyManagerWindow::applyCapturedHotkey()
    {
        auto seq = m_runtimeClient.getRecordedSequence();
        m_runtimeClient.stopRecording();
        m_isCapturing = false;
        
        if (seq && !seq->events.empty())
        {
            // Find the best KeyDown event (prefer one with a character or function key)
            std::string bestKey;
            for (const auto& event : seq->events)
            {
                if (event.type == RecordedEventType::KeyDown && event.key.has_value())
                {
                    std::string k = *event.key;
                    if (bestKey.empty())
                    {
                        bestKey = k;
                    }
                    
                    // If we find one with a '+' (modifier combination) or an 'f' (function key)
                    // or a non-control character, it's likely better than just a raw modifier.
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
                m_captureTarget->hotkey = bestKey;
                m_statusMessage = std::format("Captured: {}", m_captureTarget->hotkey);
                markDirty();
                validateHotkeys();
            }
        }
        
        m_captureTarget = nullptr;
    }

    void HotkeyManagerWindow::save()
    {
        if (!m_configData) return;
        
        for (const auto& entry : m_entries)
        {
            switch (entry.type)
            {
                case HotkeyEntry::Type::Command:
                    if (entry.index < m_configData->commands.size())
                    {
                        if (entry.hotkey.empty())
                        {
                            m_configData->commands[entry.index].startKeys.clear();
                        }
                        else
                        {
                            if (m_configData->commands[entry.index].startKeys.empty())
                                m_configData->commands[entry.index].startKeys.push_back(entry.hotkey);
                            else
                                m_configData->commands[entry.index].startKeys[0] = entry.hotkey;
                        }
                    }
                    break;
                case HotkeyEntry::Type::Sequence:
                    if (entry.index < m_configData->sequences.size())
                    {
                        m_configData->sequences[entry.index].start = entry.hotkey;
                    }
                    break;
                case HotkeyEntry::Type::GlobalEnd:
                    m_configData->endKey = entry.hotkey;
                    break;
            }
        }
        
        const auto configPath = getConfigFilePath(m_currentConfigName, m_environment);
        if (saveConfigData(*m_configData, configPath))
        {
            m_statusMessage = std::format("Saved hotkeys to {}.", m_currentConfigName);
            clearDirty();
        }
        else
        {
            m_statusMessage = std::format("Failed to save config {}.", m_currentConfigName);
        }
    }

    void HotkeyManagerWindow::renderContent()
    {
        auto& loc = Localization::get();
        // Toolbar
        if (ImGui::BeginCombo(loc.text("labels.configuration").data(), m_availableConfigs[m_selectedConfigIndex].c_str()))
        {
            for (int i = 0; i < static_cast<int>(m_availableConfigs.size()); i++)
            {
                const bool isSelected = (m_selectedConfigIndex == i);
                if (ImGui::Selectable(m_availableConfigs[i].c_str(), isSelected))
                {
                    m_selectedConfigIndex = i;
                    loadSelectedConfig();
                }
            }
            ImGui::EndCombo();
        }
        
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.reload").data()))
        {
            refreshConfigs();
            loadSelectedConfig();
        }
        
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.save").data()))
        {
            save();
        }
        
        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.validate").data()))
        {
            validateHotkeys();
            m_statusMessage = "Validation complete.";
        }
        
        ImGui::Separator();
        
        if (m_isCapturing)
        {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "Capturing hotkey for '%s'... Press any key.", m_captureTarget ? m_captureTarget->name.c_str() : "");
            if (ImGui::Button(loc.text("buttons.cancel").data()))
            {
                stopCapture();
            }
            ImGui::Separator();
        }
 
        // Table
        if (ImGui::BeginTable("HotkeyTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY, ImVec2(0, 400)))
        {
            ImGui::TableSetupColumn(loc.text("labels.type").data(), ImGuiTableColumnFlags_WidthFixed, 80.0f);
            ImGui::TableSetupColumn(loc.text("labels.name").data());
            ImGui::TableSetupColumn(loc.text("labels.hotkey").data());
            ImGui::TableSetupColumn(loc.text("labels.status").data(), ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn(loc.text("labels.actions").data(), ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableHeadersRow();
            
            for (auto& entry : m_entries)
            {
                ImGui::TableNextRow();
                
                // Type
                ImGui::TableNextColumn();
                switch (entry.type)
                {
                    case HotkeyEntry::Type::Command: ImGui::Text("%s", loc.text("labels.command").data()); break;
                    case HotkeyEntry::Type::Sequence: ImGui::Text("%s", loc.text("labels.sequence").data()); break;
                    case HotkeyEntry::Type::GlobalEnd: ImGui::Text("%s", loc.text("labels.global").data()); break;
                }
                
                // Name
                ImGui::TableNextColumn();
                ImGui::Text("%s", entry.name.c_str());
                
                // Hotkey
                ImGui::TableNextColumn();
                ImGui::PushID(&entry);
                if (ImGui::InputText("##hotkey", &entry.hotkey))
                {
                    markDirty();
                    validateHotkeys();
                }
                ImGui::PopID();
                
                // Status
                ImGui::TableNextColumn();
                if (entry.hasConflict)
                {
                    ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "%s", loc.text("labels.conflict").data());
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Duplicate hotkey detected.");
                }
                else if (!entry.isValid)
                {
                    ImGui::TextColored(ImVec4(1, 0, 0, 1), "%s", loc.text("labels.invalid").data());
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Invalid key name.");
                }
                else
                {
                    ImGui::TextColored(ImVec4(0, 1, 0, 1), "%s", loc.text("buttons.ok").data());
                }
                
                // Actions
                ImGui::TableNextColumn();
                ImGui::PushID(&entry);
                if (ImGui::Button(loc.text("labels.capture").data()))
                {
                    startCapture(entry);
                }
                
                if (entry.type != HotkeyEntry::Type::GlobalEnd)
                {
                    ImGui::SameLine();
                    if (ImGui::Button(loc.text("buttons.edit").data()))
                    {
                        if (auto editor = m_windowManager.findAs<ConfigEditorWindow>(WindowIds::ConfigEditor))
                        {
                            editor->loadConfig(m_currentConfigName);
                            m_windowManager.open(WindowIds::ConfigEditor);
                            // We can't easily jump to a specific command/sequence yet without adding more API to ConfigEditorWindow
                        }
                    }
                }
                ImGui::PopID();
            }
            
            ImGui::EndTable();
        }
        
        ImGui::Separator();
        widgets::StatusText(m_statusMessage);
        
        if (isDirty())
        {
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "* %s", loc.text("labels.unsavedChanges").data());
        }
    }
}

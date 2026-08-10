/**
 * @file sequenceEditorWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "sequenceEditorWindow.h"
#include "../widgets/basicWidgets.h"
#include "../widgets/formWidgets.h"
#include "autoinput/configValidator.h"
#include "autoinput/logger.h"
#include <imgui.h>
#include <imgui_stdlib.h>
#include <algorithm>
#include <format>
#include <regex>

namespace autoinput::ui
{
    SequenceEditorWindow::SequenceEditorWindow()
        : UiWindow("Sequence Editor", "windows.sequenceEditor")
    {
        refreshConfigList();
    }

    void SequenceEditorWindow::refreshConfigList()
    {
        m_availableConfigs = autoinput::listAvailableConfigs();
    }

    void SequenceEditorWindow::loadConfig(const std::string& nameOrPath)
    {
        auto path = autoinput::getConfigFilePath(nameOrPath);
        auto data = autoinput::loadConfigData(path);
        if (data)
        {
            m_configData = std::move(*data);
            m_currentConfigName = nameOrPath;
            m_currentConfigPath = path;
            m_selectedSequenceIndex = -1;
            clearDirty();
            m_statusMessage = "Loaded " + path.string();
            m_validationErrors.clear();
        }
        else
        {
            m_statusMessage = "Failed to load " + nameOrPath;
        }
    }

    void SequenceEditorWindow::save()
    {
        saveConfig(false);
    }

    void SequenceEditorWindow::saveConfig(bool forceUser)
    {
        std::filesystem::path path = m_currentConfigPath;
        if (forceUser || path.empty() || path.string().find("configs") != std::string::npos)
        {
            path = autoinput::getUserConfigsPath() / (m_currentConfigName + ".toml");
            std::filesystem::create_directories(path.parent_path());
        }

        if (autoinput::saveConfigData(m_configData, path))
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

    void SequenceEditorWindow::validate()
    {
        m_validationErrors = autoinput::validateConfigData(m_configData);
        if (m_validationErrors.empty())
        {
            m_statusMessage = "Configuration is valid.";
        }
        else
        {
            m_statusMessage = "Configuration has validation errors.";
        }
    }

    void SequenceEditorWindow::duplicateConfig()
    {
        m_currentConfigName += "_copy";
        m_currentConfigPath = "";
        markDirty();
        m_statusMessage = "Configuration duplicated (renamed).";
    }

    void SequenceEditorWindow::normalizeDelays(bool removeZeros)
    {
        if (m_selectedSequenceIndex < 0 || m_selectedSequenceIndex >= static_cast<int>(m_configData.sequences.size()))
            return;

        auto& seq = m_configData.sequences[m_selectedSequenceIndex];
        bool changed = false;

        std::regex delayRegex(R"(^(\d+)(?:\.\.(\d+))?(ms|s)$)", std::regex_constants::icase);
        
        auto it = seq.events.begin();
        while (it != seq.events.end())
        {
            std::smatch match;
            if (std::regex_match(it->delay, match, delayRegex))
            {
                int val1 = std::stoi(match[1].str());
                std::string unit = match[3].str();
                
                if (unit == "ms" && val1 < 5 && val1 > 0)
                {
                    it->delay = "5ms";
                    changed = true;
                }
                
                if (removeZeros && val1 == 0 && !match[2].matched)
                {
                    // If it's a zero delay and we want to remove it, we could remove the event if it's type Invalid
                    if (it->type == RecordedEventType::Invalid)
                    {
                        it = seq.events.erase(it);
                        changed = true;
                        continue;
                    }
                }
            }
            ++it;
        }

        if (changed) markDirty();
    }

    void SequenceEditorWindow::renderContent()
    {
        renderToolbar();
        
        ImGui::Separator();

        if (isDirty())
        {
            ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Unsaved changes in [%s]", m_currentConfigName.c_str());
        }
        else if (!m_currentConfigName.empty())
        {
            ImGui::Text("Editing [%s]", m_currentConfigName.c_str());
        }

        renderSequenceSelector();
        ImGui::Separator();
        renderSequenceEditor();

        ImGui::Separator();
        if (ImGui::Button("Save")) saveConfig(false);
        ImGui::SameLine();
        if (ImGui::Button("Save As")) saveConfig(true);
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

    void SequenceEditorWindow::renderToolbar()
    {
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

    void SequenceEditorWindow::renderSequenceSelector()
    {
        if (m_currentConfigName.empty())
        {
            ImGui::TextDisabled("Select a configuration to edit sequences.");
            return;
        }

        if (m_configData.sequences.empty())
        {
            ImGui::Text("No sequences found.");
            if (ImGui::Button("Add Sequence"))
            {
                m_configData.sequences.push_back({ .name = "new_sequence", .start = "f1" });
                m_selectedSequenceIndex = 0;
                markDirty();
            }
            return;
        }

        std::string preview = "Select Sequence...";
        if (m_selectedSequenceIndex >= 0 && m_selectedSequenceIndex < static_cast<int>(m_configData.sequences.size()))
        {
            preview = m_configData.sequences[m_selectedSequenceIndex].name;
        }

        if (ImGui::BeginCombo("Sequence", preview.c_str()))
        {
            for (int i = 0; i < static_cast<int>(m_configData.sequences.size()); ++i)
            {
                if (ImGui::Selectable(m_configData.sequences[i].name.c_str(), m_selectedSequenceIndex == i))
                {
                    m_selectedSequenceIndex = i;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        if (ImGui::Button("Add New"))
        {
            m_configData.sequences.push_back({ .name = "new_sequence", .start = "f1" });
            m_selectedSequenceIndex = static_cast<int>(m_configData.sequences.size()) - 1;
            markDirty();
        }
        ImGui::SameLine();
        if (m_selectedSequenceIndex >= 0 && ImGui::Button("Delete Seq"))
        {
            m_configData.sequences.erase(m_configData.sequences.begin() + m_selectedSequenceIndex);
            m_selectedSequenceIndex = -1;
            markDirty();
        }
    }

    void SequenceEditorWindow::renderSequenceEditor()
    {
        if (m_selectedSequenceIndex < 0 || m_selectedSequenceIndex >= static_cast<int>(m_configData.sequences.size()))
        {
            return;
        }

        auto& seq = m_configData.sequences[m_selectedSequenceIndex];

        if (widgets::StringInput("Sequence Name", seq.name)) markDirty();
        if (widgets::StringInput("Start Hotkey", seq.start)) markDirty();
        if (ImGui::Checkbox("Repeat", &seq.repeat)) markDirty();

        ImGui::Separator();
        ImGui::Text("Sequence Steps (%zu):", seq.events.size());
        
        if (ImGui::Button("Normalize Delays")) normalizeDelays(false);
        ImGui::SameLine();
        if (ImGui::Button("Remove Zero Delays")) normalizeDelays(true);
        
        ImGui::Text("Insert:");
        ImGui::SameLine();
        if (ImGui::Button("Delay")) insertEvent(RecordedEventType::Invalid, seq.events.size());
        ImGui::SameLine();
        if (ImGui::Button("Key")) insertEvent(RecordedEventType::KeyDown, seq.events.size());
        ImGui::SameLine();
        if (ImGui::Button("Mouse")) insertEvent(RecordedEventType::MouseDown, seq.events.size());
        ImGui::SameLine();
        if (ImGui::Button("Move")) insertEvent(RecordedEventType::MouseMove, seq.events.size());

        ImGui::BeginChild("StepsList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing() * 2), true);
        
        if (ImGui::BeginTable("StepsTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable))
        {
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Delay", ImGuiTableColumnFlags_WidthFixed, 100.0f);
            ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 220.0f);
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < seq.events.size(); ++i)
            {
                ImGui::PushID(static_cast<int>(i));
                ImGui::TableNextRow();
                renderStepEditor(seq.events[i], i);
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();
    }

    void SequenceEditorWindow::renderStepEditor(autoinput::RecordedEvent& event, size_t index)
    {
        auto& seq = m_configData.sequences[m_selectedSequenceIndex];

        ImGui::TableNextColumn();
        const char* typeNames[] = { "Delay", "Key Down", "Key Up", "Mouse Down", "Mouse Up", "Mouse Move" };
        int typeIdx = static_cast<int>(event.type);
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (ImGui::Combo("##type", &typeIdx, typeNames, IM_ARRAYSIZE(typeNames)))
        {
            event.type = static_cast<RecordedEventType>(typeIdx);
            markDirty();
        }

        ImGui::TableNextColumn();
        ImGui::SetNextItemWidth(-FLT_MIN);
        if (widgets::WaitDurationEditor("##delay", event.delay)) markDirty();

        ImGui::TableNextColumn();
        if (event.type == RecordedEventType::KeyDown || event.type == RecordedEventType::KeyUp)
        {
            std::string key = event.key.value_or("");
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputText("##key", &key))
            {
                event.key = key;
                markDirty();
            }
        }
        else if (event.type == RecordedEventType::MouseDown || event.type == RecordedEventType::MouseUp)
        {
            std::string btn = event.button.value_or("left");
            static const std::string_view buttons[] = { "left", "right", "middle", "back", "forward" };
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (widgets::StringCombo("##button", btn, buttons))
            {
                event.button = btn;
                markDirty();
            }
        }
        else if (event.type == RecordedEventType::MouseMove)
        {
            int x = event.x.value_or(0);
            int y = event.y.value_or(0);
            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x * 0.45f);
            if (ImGui::InputInt("##x", &x, 0, 0)) { event.x = x; markDirty(); }
            ImGui::SameLine();
            ImGui::SetNextItemWidth(-FLT_MIN);
            if (ImGui::InputInt("##y", &y, 0, 0)) { event.y = y; markDirty(); }
        }
        else
        {
            ImGui::TextDisabled("N/A");
        }

        ImGui::TableNextColumn();
        if (ImGui::Button("Up") && index > 0)
        {
            std::swap(seq.events[index], seq.events[index - 1]);
            markDirty();
        }
        ImGui::SameLine();
        if (ImGui::Button("Down") && index < seq.events.size() - 1)
        {
            std::swap(seq.events[index], seq.events[index + 1]);
            markDirty();
        }
        ImGui::SameLine();
        if (ImGui::Button("Dup"))
        {
            seq.events.insert(seq.events.begin() + index + 1, event);
            markDirty();
        }
        ImGui::SameLine();
        if (ImGui::Button("Del"))
        {
            seq.events.erase(seq.events.begin() + index);
            markDirty();
        }
    }

    void SequenceEditorWindow::insertEvent(autoinput::RecordedEventType type, size_t index)
    {
        auto& seq = m_configData.sequences[m_selectedSequenceIndex];
        autoinput::RecordedEvent ev;
        ev.type = type;
        ev.delay = "50ms";
        
        if (type == RecordedEventType::KeyDown || type == RecordedEventType::KeyUp) ev.key = "a";
        else if (type == RecordedEventType::MouseDown || type == RecordedEventType::MouseUp) ev.button = "left";
        else if (type == RecordedEventType::MouseMove) { ev.x = 0; ev.y = 0; }
        
        if (index >= seq.events.size()) seq.events.push_back(ev);
        else seq.events.insert(seq.events.begin() + index, ev);
        
        markDirty();
    }
}

/**
 * @file sequenceRecorderWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "sequenceRecorderWindow.h"
#include "../widgets/basicWidgets.h"
#include "autoinput/services/configService.h"
#include "autoinput/logger.h"
#include <imgui.h>
#include <format>
#include <algorithm>

namespace autoinput::ui
{
    SequenceRecorderWindow::SequenceRecorderWindow(services::IAutomationRuntimeClient& runtimeClient, const IEnvironment& environment)
        : UiWindow("Sequence Recorder", "windows.sequenceRecorder"), m_runtimeClient(runtimeClient), m_environment(environment)
    {
        refreshConfigs();
    }

    void SequenceRecorderWindow::refreshConfigs()
    {
        m_availableConfigs = listAvailableConfigs();
        if (m_availableConfigs.empty())
        {
            m_availableConfigs.push_back("default");
        }
        
        // Ensure selected index is valid
        if (m_selectedConfigIndex >= static_cast<int>(m_availableConfigs.size()))
        {
            m_selectedConfigIndex = 0;
        }
    }

    void SequenceRecorderWindow::update()
    {
        m_isRecording = m_runtimeClient.isRecording();
        m_isPaused = m_runtimeClient.isRecordingPaused();
        uint32_t newCount = m_runtimeClient.getRecordedEventCount();
        
        if (newCount != m_eventCount)
        {
            m_eventCount = newCount;
            // If recording, we might want to fetch the new events, 
            // but for simplicity we fetch the whole sequence when it changes or finished.
            if (m_isRecording)
            {
                auto seq = m_runtimeClient.getRecordedSequence();
                if (seq)
                {
                    m_recordedSequence = std::move(seq);
                    markDirty();
                }
            }
        }
    }

    void SequenceRecorderWindow::startRecording()
    {
        SequenceConfig config{
            .recordMouseMoves = m_recordMouseMoves,
            .recordMouseClicks = m_recordMouseClicks,
            .recordKeyboardEvents = m_recordKeyboardEvents,
            .recordDelays = m_recordDelays,
            .name = m_sequenceName,
            .startKey = m_startKey,
            .endKey = m_endKey,
            .playStartKey = m_startKey, // Default play key same as record start
            .mouseSampleDelay = m_mouseSampleDelay
        };
        
        auto res = m_runtimeClient.startRecording(config);
        if (res.success)
        {
            m_statusMessage = "Recording started...";
            m_recordingStartTime = std::chrono::steady_clock::now();
            m_eventCount = 0;
            m_recordedSequence = std::nullopt;
            markDirty();
        }
        else
        {
            m_statusMessage = std::format("Failed to start recording: {}", res.message);
        }
    }

    void SequenceRecorderWindow::stopRecording()
    {
        auto res = m_runtimeClient.stopRecording();
        if (res.success)
        {
            m_statusMessage = "Recording stopped.";
            // Fetch final sequence
            auto seq = m_runtimeClient.getRecordedSequence();
            if (seq)
            {
                m_recordedSequence = std::move(seq);
            }
        }
        else
        {
            m_statusMessage = std::format("Failed to stop recording: {}", res.message);
        }
    }

    void SequenceRecorderWindow::pauseRecording()
    {
        auto res = m_runtimeClient.pauseRecording();
        if (res.success)
        {
            m_statusMessage = "Recording paused.";
        }
    }

    void SequenceRecorderWindow::resumeRecording()
    {
        auto res = m_runtimeClient.resumeRecording();
        if (res.success)
        {
            m_statusMessage = "Recording resumed.";
        }
    }

    void SequenceRecorderWindow::discardRecording()
    {
        auto res = m_runtimeClient.discardRecording();
        if (res.success)
        {
            m_statusMessage = "Recording discarded.";
            m_recordedSequence = std::nullopt;
            m_eventCount = 0;
            clearDirty();
        }
    }

    void SequenceRecorderWindow::save()
    {
        saveSequence();
    }

    void SequenceRecorderWindow::discardChanges()
    {
        discardRecording();
        UiWindow::discardChanges();
    }

    void SequenceRecorderWindow::saveSequence()
    {
        if (!m_recordedSequence)
        {
            m_statusMessage = "No sequence to save.";
            return;
        }

        if (m_selectedConfigIndex >= static_cast<int>(m_availableConfigs.size()))
        {
            m_statusMessage = "Invalid config selected.";
            return;
        }

        std::string configName = m_availableConfigs[m_selectedConfigIndex];
        const auto configPath = getConfigFilePath(configName, m_environment);
        
        auto configDataOpt = loadConfigData(configPath);
        ConfigData configData;
        if (configDataOpt)
        {
            configData = *configDataOpt;
        }

        // Add or update sequence
        bool found = false;
        for (auto& seq : configData.sequences)
        {
            if (seq.name == m_recordedSequence->name)
            {
                seq = *m_recordedSequence;
                found = true;
                break;
            }
        }
        
        if (!found)
        {
            configData.sequences.push_back(*m_recordedSequence);
        }

        if (saveConfigData(configData, configPath))
        {
            m_statusMessage = std::format("Sequence '{}' saved to config '{}'.", m_recordedSequence->name, configName);
            clearDirty();
        }
        else
        {
            m_statusMessage = std::format("Failed to save sequence to config '{}'.", configName);
        }
    }

    void SequenceRecorderWindow::renderContent()
    {
        renderRecorderControls();
        ImGui::Separator();
        
        if (!m_isRecording && !m_isPaused && (!m_recordedSequence || m_recordedSequence->events.empty()))
        {
            renderSettings();
        }
        else
        {
            renderEventList();
        }
        
        ImGui::Separator();
        renderStatus();
    }

    void SequenceRecorderWindow::renderRecorderControls()
    {
        if (!m_isRecording && !m_isPaused)
        {
            if (ImGui::Button("Start Recording"))
            {
                startRecording();
            }
        }
        else
        {
            if (m_isPaused)
            {
                if (ImGui::Button("Resume"))
                {
                    resumeRecording();
                }
            }
            else
            {
                if (ImGui::Button("Pause"))
                {
                    pauseRecording();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop"))
            {
                stopRecording();
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Discard"))
        {
            if (isDirty())
            {
                ImGui::OpenPopup("Confirm Discard");
            }
            else
            {
                discardRecording();
            }
        }

        if (ImGui::BeginPopupModal("Confirm Discard", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("Are you sure you want to discard the current recording?\nThis action cannot be undone.");
            ImGui::Separator();
            if (ImGui::Button("Yes", ImVec2(120, 0)))
            {
                discardRecording();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("No"))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();
        ImGui::BeginDisabled(m_isRecording || !m_recordedSequence || m_recordedSequence->events.empty());
        if (ImGui::Button("Save Sequence"))
        {
            saveSequence();
        }
        ImGui::EndDisabled();
    }

    void SequenceRecorderWindow::renderSettings()
    {
        ImGui::Text("Recording Settings");
        ImGui::InputText("Sequence Name", m_sequenceName, sizeof(m_sequenceName));
        
        if (ImGui::BeginCombo("Save to Config", m_availableConfigs[m_selectedConfigIndex].c_str()))
        {
            for (int i = 0; i < static_cast<int>(m_availableConfigs.size()); i++)
            {
                const bool isSelected = (m_selectedConfigIndex == i);
                if (ImGui::Selectable(m_availableConfigs[i].c_str(), isSelected))
                {
                    m_selectedConfigIndex = i;
                }
            }
            ImGui::EndCombo();
        }
        ImGui::SameLine();
        if (ImGui::Button("Refresh"))
        {
            refreshConfigs();
        }

        ImGui::Checkbox("Record Mouse Moves", &m_recordMouseMoves);
        ImGui::Checkbox("Record Mouse Clicks", &m_recordMouseClicks);
        ImGui::Checkbox("Record Keyboard Events", &m_recordKeyboardEvents);
        ImGui::Checkbox("Record Delays", &m_recordDelays);
        ImGui::InputText("Start Key", m_startKey, sizeof(m_startKey));
        ImGui::InputText("End Key", m_endKey, sizeof(m_endKey));
        ImGui::InputText("Mouse Sample Delay", m_mouseSampleDelay, sizeof(m_mouseSampleDelay));
    }

    void SequenceRecorderWindow::renderEventList()
    {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_recordingStartTime);
        
        ImGui::Text("Events: %u | Time: %02lld:%02lld", m_eventCount, elapsed.count() / 60, elapsed.count() % 60);
        
        if (isDirty())
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "[Unsaved]");
        }

        ImGui::BeginChild("EventList", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), true);
        if (m_recordedSequence)
        {
            for (const auto& event : m_recordedSequence->events)
            {
                ImVec4 color(1, 1, 1, 1);
                std::string text;
                
                switch (event.type)
                {
                    case RecordedEventType::KeyDown:
                        color = ImVec4(0.4f, 0.8f, 1.0f, 1.0f);
                        text = std::format("[Key Down]  {} (delay: {})", event.key.value_or("?"), event.delay);
                        break;
                    case RecordedEventType::KeyUp:
                        color = ImVec4(0.4f, 0.6f, 0.9f, 1.0f);
                        text = std::format("[Key Up]    {} (delay: {})", event.key.value_or("?"), event.delay);
                        break;
                    case RecordedEventType::MouseDown:
                        color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
                        text = std::format("[Mouse Down] {} at ({}, {}) (delay: {})", event.button.value_or("?"), event.x.value_or(0), event.y.value_or(0), event.delay);
                        break;
                    case RecordedEventType::MouseUp:
                        color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
                        text = std::format("[Mouse Up]   {} at ({}, {}) (delay: {})", event.button.value_or("?"), event.x.value_or(0), event.y.value_or(0), event.delay);
                        break;
                    case RecordedEventType::MouseMove:
                        color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
                        text = std::format("[Mouse Move] to ({}, {}) (delay: {})", event.x.value_or(0), event.y.value_or(0), event.delay);
                        break;
                    default:
                        text = "Unknown event";
                        break;
                }
                ImGui::TextColored(color, "%s", text.c_str());
            }
            
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            {
                ImGui::SetScrollHereY(1.0f);
            }
        }
        ImGui::EndChild();
    }

    void SequenceRecorderWindow::renderStatus()
    {
        widgets::StatusText(m_statusMessage);
    }
}

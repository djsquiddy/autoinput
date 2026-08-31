/**
 * @file sequenceRecorderWindow.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "sequenceRecorderWindow.h"
#include "../core/localization.h"
#include "../widgets/basicWidgets.h"
#include "autoinput/services/configService.h"
#include "autoinput/support/logger.h"
#include <algorithm>
#include <format>
#include <imgui.h>

namespace autoinput::ui
{
    SequenceRecorderWindow::SequenceRecorderWindow(services::IAutomationRuntimeClient& runtimeClient,
                                                   const IEnvironment& environment)
        : UiWindow("Sequence Recorder", "windows.sequenceRecorder")
        , m_runtimeClient(runtimeClient)
        , m_environment(environment)
    {
        refreshConfigs();
    }

    void SequenceRecorderWindow::onOpen()
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
                    m_graphEditorState.syncWithSequence(*m_recordedSequence);
                    markDirty();
                }
            }
        }
    }

    void SequenceRecorderWindow::startRecording()
    {
        auto& loc = Localization::get();
        SequenceConfig config{ .recordMouseMoves = m_recordMouseMoves,
                               .recordMouseClicks = m_recordMouseClicks,
                               .recordKeyboardEvents = m_recordKeyboardEvents,
                               .recordDelays = m_recordDelays,
                               .name = m_sequenceName,
                               .startKey = m_startKey,
                               .endKey = m_endKey,
                               .playStartKey = m_startKey, // Default play key same as record start
                               .mouseSampleDelay = m_mouseSampleDelay };

        auto res = m_runtimeClient.startRecording(config);
        if (res.success)
        {
            m_statusMessage = loc.text("status.recordingStarted");
            m_recordingStartTime = std::chrono::steady_clock::now();
            m_eventCount = 0;
            m_recordedSequence = std::nullopt;
            m_graphEditorState = editors::SequenceGraphEditorState{};
            markDirty();
        }
        else
        {
            m_statusMessage = loc.format("status.failedToStartRecording", res.message);
        }
    }

    void SequenceRecorderWindow::stopRecording()
    {
        auto& loc = Localization::get();
        auto res = m_runtimeClient.stopRecording();
        if (res.success)
        {
            m_statusMessage = loc.text("status.recordingStopped");
            // Fetch final sequence
            auto seq = m_runtimeClient.getRecordedSequence();
            if (seq)
            {
                m_recordedSequence = std::move(seq);
                m_graphEditorState.rebuildFromSequence(*m_recordedSequence);
            }
        }
        else
        {
            m_statusMessage = loc.format("status.failedToStopRecording", res.message);
        }
    }

    void SequenceRecorderWindow::pauseRecording()
    {
        auto res = m_runtimeClient.pauseRecording();
        if (res.success)
        {
            m_statusMessage = Localization::get().text("status.recordingPaused");
        }
    }

    void SequenceRecorderWindow::resumeRecording()
    {
        auto res = m_runtimeClient.resumeRecording();
        if (res.success)
        {
            m_statusMessage = Localization::get().text("status.recordingResumed");
        }
    }

    void SequenceRecorderWindow::discardRecording()
    {
        auto res = m_runtimeClient.discardRecording();
        if (res.success)
        {
            m_statusMessage = Localization::get().text("status.recordingDiscarded");
            m_recordedSequence = std::nullopt;
            m_eventCount = 0;
            m_graphEditorState = editors::SequenceGraphEditorState{};
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
        auto& loc = Localization::get();
        if (!m_recordedSequence)
        {
            m_statusMessage = loc.text("status.noSequenceToSave");
            return;
        }

        if (m_selectedConfigIndex >= static_cast<int>(m_availableConfigs.size()))
        {
            m_statusMessage = loc.text("status.invalidConfigSelected");
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
            m_statusMessage = loc.format("status.sequenceSaved", m_recordedSequence->name, configName);
            clearDirty();
        }
        else
        {
            m_statusMessage = loc.format("status.failedToSaveSequence", configName);
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
            if (ImGui::BeginTabBar("SequenceRecorderTabs"))
            {
                if (ImGui::BeginTabItem("Events List"))
                {
                    renderEventList();
                    ImGui::EndTabItem();
                }

                if (ImGui::BeginTabItem("Visual Graph"))
                {
                    if (m_recordedSequence)
                    {
                        if (editors::renderSequenceGraphEditor(
                                *m_recordedSequence, m_graphEditorState, "SequenceRecorderGraph"))
                        {
                            m_eventCount = static_cast<uint32_t>(m_recordedSequence->events.size());
                            markDirty();
                        }
                    }
                    else
                    {
                        ImGui::TextDisabled("No recorded sequence available to display in graph.");
                    }
                    ImGui::EndTabItem();
                }

                ImGui::EndTabBar();
            }
        }

        ImGui::Separator();
        renderStatus();
    }

    void SequenceRecorderWindow::renderRecorderControls()
    {
        auto& loc = Localization::get();
        if (!m_isRecording && !m_isPaused)
        {
            if (ImGui::Button(loc.text("buttons.startRecording").data()))
            {
                startRecording();
            }
        }
        else
        {
            if (m_isPaused)
            {
                if (ImGui::Button(loc.text("buttons.resume").data()))
                {
                    resumeRecording();
                }
            }
            else
            {
                if (ImGui::Button(loc.text("buttons.pause").data()))
                {
                    pauseRecording();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button(loc.text("buttons.stop").data()))
            {
                stopRecording();
            }
        }

        ImGui::SameLine();
        if (ImGui::Button(loc.text("buttons.discard").data()))
        {
            if (isDirty())
            {
                ImGui::OpenPopup(loc.text("modals.discardRecordingTitle").data());
            }
            else
            {
                discardRecording();
            }
        }

        if (ImGui::BeginPopupModal(
                loc.text("modals.discardRecordingTitle").data(), nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text("%s", loc.text("modals.discardRecordingMessage").data());
            ImGui::Separator();
            if (ImGui::Button(loc.text("buttons.yes").data(), ImVec2(120, 0)))
            {
                discardRecording();
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button(loc.text("buttons.no").data()))
            {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::SameLine();
        ImGui::BeginDisabled(m_isRecording || !m_recordedSequence || m_recordedSequence->events.empty());
        if (ImGui::Button(loc.text("buttons.saveSequence").data()))
        {
            saveSequence();
        }
        ImGui::EndDisabled();
    }

    void SequenceRecorderWindow::renderSettings()
    {
        auto& loc = Localization::get();
        ImGui::Text("%s", loc.text("labels.recordingSettings").data());
        ImGui::InputText(loc.text("labels.sequenceName").data(), m_sequenceName, sizeof(m_sequenceName));

        if (ImGui::BeginCombo(loc.text("labels.saveToConfig").data(),
                              m_availableConfigs[m_selectedConfigIndex].c_str()))
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
        if (ImGui::Button(loc.text("buttons.refresh").data()))
        {
            refreshConfigs();
        }

        ImGui::Checkbox(loc.text("labels.recordMouseMoves").data(), &m_recordMouseMoves);
        ImGui::Checkbox(loc.text("labels.recordMouseClicks").data(), &m_recordMouseClicks);
        ImGui::Checkbox(loc.text("labels.recordKeyboardEvents").data(), &m_recordKeyboardEvents);
        ImGui::Checkbox(loc.text("labels.recordDelays").data(), &m_recordDelays);
        ImGui::InputText(loc.text("labels.startKey").data(), m_startKey, sizeof(m_startKey));
        ImGui::InputText(loc.text("labels.endKey").data(), m_endKey, sizeof(m_endKey));
        ImGui::InputText(loc.text("labels.mouseSampleDelay").data(), m_mouseSampleDelay, sizeof(m_mouseSampleDelay));
    }

    void SequenceRecorderWindow::renderEventList()
    {
        auto& loc = Localization::get();
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_recordingStartTime);

        ImGui::Text(
            "%s", loc.format("labels.eventsAndTime", m_eventCount, elapsed.count() / 60, elapsed.count() % 60).c_str());

        if (isDirty())
        {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1, 1, 0, 1), "%s", loc.text("labels.unsaved").data());
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
                    text = loc.format("labels.keyDown", event.key.value_or("?"), event.delay);
                    break;
                case RecordedEventType::KeyUp:
                    color = ImVec4(0.4f, 0.6f, 0.9f, 1.0f);
                    text = loc.format("labels.keyUp", event.key.value_or("?"), event.delay);
                    break;
                case RecordedEventType::MouseDown:
                    color = ImVec4(0.4f, 1.0f, 0.4f, 1.0f);
                    text = loc.format("labels.mouseDown",
                                      event.button.value_or("?"),
                                      event.x.value_or(0),
                                      event.y.value_or(0),
                                      event.delay);
                    break;
                case RecordedEventType::MouseUp:
                    color = ImVec4(0.2f, 0.8f, 0.2f, 1.0f);
                    text = loc.format("labels.mouseUp",
                                      event.button.value_or("?"),
                                      event.x.value_or(0),
                                      event.y.value_or(0),
                                      event.delay);
                    break;
                case RecordedEventType::MouseMove:
                    color = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
                    text = loc.format("labels.mouseMove", event.x.value_or(0), event.y.value_or(0), event.delay);
                    break;
                default: text = loc.text("labels.unknownEvent"); break;
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
} // namespace autoinput::ui

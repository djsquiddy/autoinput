/**
 * @file sequenceRecorderWindow.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_SEQUENCE_RECORDER_WINDOW_H
#define INCLUDE_AUTOINPUT_UI_SEQUENCE_RECORDER_WINDOW_H
#pragma once

#include "../core/uiWindow.h"
#include "../editors/sequenceGraphEditor.h"
#include "../graph/recorderGraphAdapter.h"
#include "autoinput/platform/environment.h"
#include "autoinput/services/automationRuntimeClient.h"
#include <chrono>
#include <string>
#include <vector>

namespace autoinput::ui
{
    class SequenceRecorderWindow final : public UiWindow
    {
    public:
        explicit SequenceRecorderWindow(services::IAutomationRuntimeClient& runtimeClient,
                                        const IEnvironment& environment);

        void onOpen() override;
        void renderContent() override;
        void update() override;

    protected:
        void save() override;
        void discardChanges() override;

    private:
        services::IAutomationRuntimeClient& m_runtimeClient;
        const IEnvironment& m_environment;

        // UI State
        char m_sequenceName[256]{ "new_sequence" };
        int m_selectedConfigIndex{ 0 };
        std::vector<std::string> m_availableConfigs;

        bool m_recordMouseMoves{ false };
        bool m_recordMouseClicks{ true };
        bool m_recordKeyboardEvents{ true };
        bool m_recordDelays{ true };
        char m_startKey[64]{ "f2" };
        char m_endKey[64]{ "f3" };
        char m_mouseSampleDelay[64]{ "100ms" };

        std::string m_statusMessage;

        // Recording state (cached from runtime)
        bool m_isRecording{ false };
        bool m_isPaused{ false };
        uint32_t m_eventCount{ 0 };
        std::optional<RecordedSequence> m_recordedSequence;
        std::chrono::steady_clock::time_point m_recordingStartTime;
        editors::SequenceGraphEditorState m_graphEditorState;

        void refreshConfigs();
        void startRecording();
        void stopRecording();
        void pauseRecording();
        void resumeRecording();
        void discardRecording();
        void saveSequence();

        void renderRecorderControls();
        void renderSettings();
        void renderEventList();
        void renderStatus();
    };
} // namespace autoinput::ui

#endif // INCLUDE_AUTOINPUT_UI_SEQUENCE_RECORDER_WINDOW_H

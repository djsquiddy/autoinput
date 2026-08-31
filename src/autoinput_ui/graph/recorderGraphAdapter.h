/**
 * @file recorderGraphAdapter.h
 * @brief Integration and workflow adapter connecting sequence recorder output with the graph editor model.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_GRAPH_RECORDER_GRAPH_ADAPTER_H
#define INCLUDE_AUTOINPUT_UI_GRAPH_RECORDER_GRAPH_ADAPTER_H

#include "autoinput/config/config.h"
#include "autoinput_ui/editors/sequenceGraphEditor.h"
#include "autoinput_ui/graph/graphModel.h"
#include "autoinput_ui/graph/graphValidator.h"
#include "autoinput_ui/graph/sequenceGraphAdapter.h"
#include "autoinput_ui/graph/sequenceGraphCompiler.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace autoinput::ui::graph
{
    /**
     * @brief Result of converting and validating recorder output into a sequence graph.
     */
    struct RecorderGraphGenerationResult
    {
        bool success{ false };
        GraphDocument graphDocument;
        ValidationResult validationResult;
        std::vector<std::string> warnings;
        std::string statusMessage;
    };

    /**
     * @brief Generates a GraphDocument from a RecordedSequence with robust error handling for partial/invalid events.
     *
     * @param sequence The input recorded sequence (may be partial, empty, or contain incomplete events).
     * @param options Layout options for graph generation.
     * @return Structured result containing the generated graph, validation report, and any warnings.
     */
    [[nodiscard]] RecorderGraphGenerationResult generateGraphFromRecordedSequence(
        const autoinput::RecordedSequence& sequence,
        const SequenceGraphOptions& options = SequenceGraphOptions::defaults());

    /**
     * @brief Sanitizes a RecordedSequence by ensuring default values for partial or missing fields without altering execution semantics.
     */
    [[nodiscard]] autoinput::RecordedSequence sanitizeRecordedSequence(const autoinput::RecordedSequence& sequence);

    /**
     * @brief Manages the non-UI workflow state connecting runtime sequence recording with the visual graph editor.
     */
    class RecorderGraphWorkflow
    {
    public:
        RecorderGraphWorkflow() = default;

        /**
         * @brief Resets workflow state when a new recording session begins.
         */
        void onRecordingStarted(std::string_view sequenceName = "new_sequence", std::string_view startKey = "f2",
                                std::string_view endKey = "f3");

        /**
         * @brief Updates workflow state with incoming recording events.
         */
        void onRecordingUpdated(bool isRecording, bool isPaused, uint32_t eventCount,
                                std::optional<autoinput::RecordedSequence> currentSequence);

        /**
         * @brief Finalizes recording and performs post-recording graph generation.
         */
        void onRecordingStopped(std::optional<autoinput::RecordedSequence> finalSequence);

        /**
         * @brief Discards the current recording and clears the graph.
         */
        void onRecordingDiscarded();

        /**
         * @brief Validates the current graph topology.
         */
        bool validateGraph();

        /**
         * @brief Compiles current graph edits back into the recorded sequence.
         */
        bool applyGraphEdits();

        /**
         * @brief Saves or merges the recorded sequence into the target config.
         */
        bool saveToConfig(autoinput::ConfigData& configData, std::string_view targetSequenceName = "") const;

        [[nodiscard]] bool isRecording() const noexcept { return m_isRecording; }
        [[nodiscard]] bool isPaused() const noexcept { return m_isPaused; }
        [[nodiscard]] uint32_t eventCount() const noexcept { return m_eventCount; }
        [[nodiscard]] bool hasRecordedSequence() const noexcept { return m_recordedSequence.has_value(); }
        [[nodiscard]] const std::optional<autoinput::RecordedSequence>& getRecordedSequence() const noexcept
        {
            return m_recordedSequence;
        }
        [[nodiscard]] std::optional<autoinput::RecordedSequence>& getRecordedSequence() noexcept
        {
            return m_recordedSequence;
        }
        [[nodiscard]] const editors::SequenceGraphEditorState& getGraphEditorState() const noexcept
        {
            return m_graphEditorState;
        }
        [[nodiscard]] editors::SequenceGraphEditorState& getGraphEditorState() noexcept { return m_graphEditorState; }
        [[nodiscard]] const GraphDocument& getGraphDocument() const noexcept
        {
            return m_graphEditorState.graphDocument;
        }
        [[nodiscard]] const ValidationResult& getValidationResult() const noexcept
        {
            return m_graphEditorState.validationResult;
        }
        [[nodiscard]] const std::string& getStatusMessage() const noexcept { return m_statusMessage; }

    private:
        bool m_isRecording{ false };
        bool m_isPaused{ false };
        uint32_t m_eventCount{ 0 };
        std::optional<autoinput::RecordedSequence> m_recordedSequence{ std::nullopt };
        editors::SequenceGraphEditorState m_graphEditorState;
        std::string m_statusMessage{ "Ready" };
    };

} // namespace autoinput::ui::graph

#endif // INCLUDE_AUTOINPUT_UI_GRAPH_RECORDER_GRAPH_ADAPTER_H

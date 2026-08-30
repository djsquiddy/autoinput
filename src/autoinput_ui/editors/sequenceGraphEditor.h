/**
 * @file sequenceGraphEditor.h
 * @brief Visual graph editor component for inspecting and editing RecordedSequence data.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_EDITORS_SEQUENCE_GRAPH_EDITOR_H
#define INCLUDE_AUTOINPUT_UI_EDITORS_SEQUENCE_GRAPH_EDITOR_H

#include "autoinput/config/config.h"
#include "autoinput_ui/graph/fallbackGraphViewer.h"
#include "autoinput_ui/graph/graphModel.h"
#include "autoinput_ui/graph/graphValidator.h"
#include "autoinput_ui/graph/sequenceGraphAdapter.h"
#include "autoinput_ui/graph/sequenceGraphCompiler.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace autoinput::ui::editors
{
    /**
     * @brief Detailed inspection info for a selected graph node.
     */
    struct SelectedNodeInspectionDetails
    {
        graph::NodeId nodeId{ graph::InvalidNodeId };
        graph::NodeKind kind{ graph::NodeKind::Unknown };
        std::string title;
        std::string subtitle;
        std::optional<std::size_t> sourceIndex{ std::nullopt };
        bool hasAssociatedEvent{ false };
        autoinput::RecordedEvent associatedEvent;
        std::vector<graph::ValidationIssue> validationIssues;
    };

    /**
     * @brief State container and non-UI controller for the sequence graph editor.
     */
    struct SequenceGraphEditorState
    {
        graph::GraphDocument graphDocument;
        graph::FallbackGraphViewerState viewerState;
        graph::ValidationResult validationResult;
        graph::SequenceGraphOptions adapterOptions{ graph::SequenceGraphOptions::defaults() };
        graph::SequenceCompileOptions compileOptions{ graph::SequenceCompileOptions::defaults() };
        std::string statusMessage{ "Ready" };
        bool isGraphSynchronized{ false };
        bool isEditingAllowed{ false };
        std::optional<std::string> lastCompilationError{ std::nullopt };
        std::size_t cachedSequenceEventCount{ 0 };

        /**
         * @brief Synchronizes the graph with the given sequence if out of sync or forced.
         */
        void syncWithSequence(const autoinput::RecordedSequence& sequence, bool force = false);

        /**
         * @brief Rebuilds the graph document from the given recorded sequence.
         */
        void rebuildFromSequence(const autoinput::RecordedSequence& sequence);

        /**
         * @brief Runs graph validation rules against the current graph document.
         */
        bool validateCurrentGraph();

        /**
         * @brief Compiles the current graph document into a RecordedSequence.
         */
        [[nodiscard]] graph::SequenceCompileResult compileGraph(
            const std::optional<autoinput::RecordedSequence>& sourceContext = std::nullopt);

        /**
         * @brief Compiles and applies graph changes back to the target sequence.
         */
        bool applyToSequence(autoinput::RecordedSequence& targetSequence);

        /**
         * @brief Extracts inspection details for the currently selected node.
         */
        [[nodiscard]] std::optional<SelectedNodeInspectionDetails> getSelectedNodeDetails(
            const autoinput::RecordedSequence& sequence) const;
    };

    // --- Non-UI Pure Helper Functions ---

    /**
     * @brief Resolves inspection details for a specific node in a graph document.
     */
    [[nodiscard]] std::optional<SelectedNodeInspectionDetails> resolveNodeInspectionDetails(
        const graph::GraphDocument& doc, graph::NodeId nodeId, const autoinput::RecordedSequence& sequence,
        const graph::ValidationResult& validationResult);

    /**
     * @brief Formats a summary string for an event's parameters.
     */
    [[nodiscard]] std::string formatEventFieldsSummary(const autoinput::RecordedEvent& event);

    // --- UI Rendering Functions ---

    /**
     * @brief Renders the sequence graph editor component using Dear ImGui.
     * @param sequence The sequence being viewed or edited.
     * @param state The editor component state.
     * @param editorId Unique ImGui identifier.
     * @return true if the sequence was modified (e.g., compiled from graph).
     */
    bool renderSequenceGraphEditor(autoinput::RecordedSequence& sequence, SequenceGraphEditorState& state,
                                   const char* editorId = "SequenceGraphEditor");

} // namespace autoinput::ui::editors

#endif // INCLUDE_AUTOINPUT_UI_EDITORS_SEQUENCE_GRAPH_EDITOR_H

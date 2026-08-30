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
#include <string_view>
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
     * @brief Snapshot representing editor state for undo/redo history.
     */
    struct GraphEditorSnapshot
    {
        graph::GraphDocument graphDocument;
        graph::NodeId selectedNodeId{ graph::InvalidNodeId };
        std::string statusMessage;
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
        bool isEditingAllowed{ true };
        std::optional<std::string> lastCompilationError{ std::nullopt };
        std::size_t cachedSequenceEventCount{ 0 };

        // Undo / Redo history
        std::vector<GraphEditorSnapshot> undoStack;
        std::vector<GraphEditorSnapshot> redoStack;
        std::size_t maxUndoSteps{ 50 };

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

        // --- Selection Operations ---

        void selectNode(graph::NodeId nodeId);
        void clearSelection();
        [[nodiscard]] graph::NodeId getSelectedNodeId() const noexcept;
        [[nodiscard]] bool hasSelection() const noexcept;

        // --- Undo / Redo Operations ---

        void pushUndoSnapshot();
        [[nodiscard]] bool canUndo() const noexcept;
        [[nodiscard]] bool canRedo() const noexcept;
        bool undo();
        bool redo();
        void clearUndoRedo();

        // --- Editable Graph Operations ---

        /**
         * @brief Adds a recorded event node into the linear graph chain.
         * @param event The event data to insert.
         * @param insertAfterNodeId Optional node after which to insert. If nullopt, inserts after selected or before End.
         * @return ID of the created node.
         */
        graph::NodeId addEventNode(const autoinput::RecordedEvent& event,
                                   std::optional<graph::NodeId> insertAfterNodeId = std::nullopt);

        /**
         * @brief Adds a dedicated wait/delay node into the linear graph chain.
         * @param delay Duration string (e.g., "100ms").
         * @param insertAfterNodeId Optional node after which to insert.
         * @return ID of the created node.
         */
        graph::NodeId addWaitNode(std::string_view delay = "100ms",
                                  std::optional<graph::NodeId> insertAfterNodeId = std::nullopt);

        /**
         * @brief Deletes a node and optionally reconnects the surrounding chain.
         * @param nodeId Node ID to delete (protects Start and End nodes).
         * @param reconnectChain When true, connects predecessor directly to successor.
         * @return True if node existed and was deleted.
         */
        bool deleteNode(graph::NodeId nodeId, bool reconnectChain = true);

        /**
         * @brief Deletes the currently selected node if valid.
         */
        bool deleteSelectedNode(bool reconnectChain = true);

        /**
         * @brief Updates event data and title/subtitle for an existing graph node.
         */
        bool updateNodeEvent(graph::NodeId nodeId, const autoinput::RecordedEvent& event);

        /**
         * @brief Updates delay string on an existing graph node.
         */
        bool updateNodeDelay(graph::NodeId nodeId, std::string_view newDelay);

        /**
         * @brief Moves an event node one position earlier in the linear execution chain.
         */
        bool moveNodeUp(graph::NodeId nodeId);

        /**
         * @brief Moves an event node one position later in the linear execution chain.
         */
        bool moveNodeDown(graph::NodeId nodeId);

        /**
         * @brief Reconstructs a clean linear chain Start -> Event1 -> ... -> EventN -> End.
         */
        bool reconnectLinearChain();

        /**
         * @brief Connects source node output to target node input.
         */
        bool connectNodes(graph::NodeId sourceNodeId, graph::NodeId targetNodeId);

        /**
         * @brief Auto-arranges node layout positions linearly.
         */
        void autoLayout(float startX = 50.0F, float startY = 100.0F, float stepX = 200.0F, float stepY = 0.0F);
    };

    // --- Non-UI Pure Helper Functions ---

    /**
     * @brief Resolves inspection details for a specific node in a graph document.
     */
    [[nodiscard]] std::optional<SelectedNodeInspectionDetails> resolveNodeInspectionDetails(
        const graph::GraphDocument& doc, graph::NodeId nodeId, const autoinput::RecordedSequence& sequence,
        const graph::ValidationResult& validationResult);

    /**
     * @brief Finds the first pin of the specified direction on a node.
     */
    [[nodiscard]] const graph::GraphPin* findPinOfDirection(const graph::GraphDocument& doc, graph::NodeId nodeId,
                                                            graph::PinDirection direction);

    /**
     * @brief Traverses the execution chain from Start to End and returns ordered NodeIds.
     */
    [[nodiscard]] std::vector<graph::NodeId> getLinearExecutionNodes(const graph::GraphDocument& doc);

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

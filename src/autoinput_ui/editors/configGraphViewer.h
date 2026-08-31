/**
 * @file configGraphViewer.h
 * @brief Visual graph viewer, editor, and diagnostics inspector for AutoInput configuration data.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_EDITORS_CONFIG_GRAPH_VIEWER_H
#define INCLUDE_AUTOINPUT_UI_EDITORS_CONFIG_GRAPH_VIEWER_H

#include "autoinput/config/config.h"
#include "autoinput_ui/graph/configDiagnostics.h"
#include "autoinput_ui/graph/configGraphAdapter.h"
#include "autoinput_ui/graph/fallbackGraphViewer.h"
#include "autoinput_ui/graph/graphModel.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace autoinput::ui::editors
{
    // Re-export core diagnostics types into editors namespace for backwards compatibility
    using graph::analyzeConfigDiagnostics;
    using graph::ConfigDiagnosticIssue;
    using graph::ConfigDiagnosticSeverity;
    using graph::configDiagnosticSeverityToString;
    using graph::ConfigDiagnosticsResult;

    /**
     * @brief Detailed inspection properties for a selected node in the config graph.
     */
    struct ConfigNodeInspectionDetails
    {
        graph::NodeId nodeId{ graph::InvalidNodeId };
        graph::NodeKind kind{ graph::NodeKind::Unknown };
        std::string title;
        std::string subtitle;
        std::optional<std::size_t> sourceIndex{ std::nullopt };
        std::optional<std::size_t> commandIndex{ std::nullopt };
        std::optional<std::size_t> controlIndex{ std::nullopt };
        std::optional<std::size_t> sequenceIndex{ std::nullopt };

        // Associated entity metadata
        std::vector<std::string> connectedInputs;
        std::vector<std::string> connectedControls;
        std::vector<std::string> connectedGroups;
        std::vector<std::string> connectedTargets;

        std::vector<ConfigDiagnosticIssue> diagnosticIssues;
    };

    /**
     * @brief Staged draft buffer for editing a command and its associated relationships.
     */
    struct CommandEditDraft
    {
        std::size_t commandIndex{ 0 };
        std::string name;
        std::string exclusiveGroup;
        std::vector<std::string> startKeys;
        std::vector<autoinput::CommandControlData> controls;
        bool isActive{ false };
        bool isDirty{ false };
        std::vector<ConfigDiagnosticIssue> draftIssues;

        [[nodiscard]] bool hasErrors() const noexcept
        {
            return std::ranges::any_of(
                draftIssues, [](const auto& issue) { return issue.severity == ConfigDiagnosticSeverity::Error; });
        }

        [[nodiscard]] bool hasWarnings() const noexcept
        {
            return std::ranges::any_of(
                draftIssues, [](const auto& issue) { return issue.severity == ConfigDiagnosticSeverity::Warning; });
        }
    };

    /**
     * @brief Extracts inspection details and relationship information for a specific node.
     *
     * @param config The source configuration.
     * @param doc The visual graph document.
     * @param nodeId The node identifier to inspect.
     * @param diagnostics Optional diagnostics results to correlate with the selected node.
     * @return Inspection details if the node exists, nullopt otherwise.
     */
    [[nodiscard]] std::optional<ConfigNodeInspectionDetails> inspectConfigGraphNode(
        const autoinput::ConfigData& config, const graph::GraphDocument& doc, graph::NodeId nodeId,
        const ConfigDiagnosticsResult* diagnostics = nullptr);

    /**
     * @brief State container and controller for the visual configuration graph viewer and relationship editor.
     */
    struct ConfigGraphViewerState
    {
        graph::GraphDocument graphDocument;
        graph::FallbackGraphViewerState viewerState;
        graph::ConfigGraphOptions adapterOptions{ graph::ConfigGraphOptions::defaults() };
        ConfigDiagnosticsResult diagnosticsResult;
        CommandEditDraft editDraft;
        std::string statusMessage{ "Ready" };
        bool isGraphSynchronized{ false };
        bool isEditingAllowed{ true };

        // Visibility / Filter Toggles
        bool showCommands{ true };
        bool showControls{ true };
        bool showSequences{ true };
        bool showInputs{ true };
        bool showExclusiveGroups{ true };
        bool showGlobalSettings{ true };
        bool showApplicationFilter{ true };
        bool showBlacklist{ true };

        // Panel visibility
        bool showInspectorPanel{ true };
        bool showDiagnosticsPanel{ true };
        bool showFilterToolbar{ true };

        // Cached counts to detect config modifications
        std::size_t cachedCommandCount{ 0 };
        std::size_t cachedSequenceCount{ 0 };
        std::string cachedAppName;
        std::string cachedEndKey;

        /**
         * @brief Synchronizes the graph with the given config if out of date or forced.
         */
        void syncWithConfig(const autoinput::ConfigData& config, bool force = false);

        /**
         * @brief Rebuilds the graph document from the current config and active filter toggles.
         */
        void rebuildFromConfig(const autoinput::ConfigData& config);

        /**
         * @brief Runs diagnostics analysis against the configuration.
         */
        void runDiagnostics(const autoinput::ConfigData& config);

        /**
         * @brief Selects a specific node by ID.
         */
        void selectNode(graph::NodeId nodeId);

        /**
         * @brief Clears current node/link selection.
         */
        void clearSelection();

        /**
         * @brief Returns the ID of the currently selected node, or InvalidNodeId if none.
         */
        [[nodiscard]] graph::NodeId getSelectedNodeId() const noexcept;

        /**
         * @brief Returns whether a node or link is currently selected.
         */
        [[nodiscard]] bool hasSelection() const noexcept;

        // --- Editing Operations ---

        /**
         * @brief Begins staging edits for the command at the specified index.
         * @return true if command was successfully loaded into the draft buffer.
         */
        bool beginCommandEdit(std::size_t commandIndex, const autoinput::ConfigData& config, bool force = false);

        /**
         * @brief Cancels and discards the active command editing draft.
         */
        void cancelCommandEdit();

        /**
         * @brief Updates the staged command name in the edit draft.
         */
        void setCommandName(std::string_view name);

        /**
         * @brief Updates the staged exclusive group in the edit draft.
         */
        void setExclusiveGroup(std::string_view group);

        /**
         * @brief Adds a start key to the staged command draft.
         */
        void addStartKey(std::string_view key);

        /**
         * @brief Removes a start key by index from the staged command draft.
         */
        bool removeStartKey(std::size_t index);

        /**
         * @brief Updates a start key at a given index in the staged command draft.
         */
        bool setStartKey(std::size_t index, std::string_view key);

        /**
         * @brief Adds a control to the staged command draft.
         */
        void addControl(std::string_view action = "start", std::string_view input = "mouse.left");

        /**
         * @brief Removes a control by index from the staged command draft.
         */
        bool removeControl(std::size_t index);

        /**
         * @brief Updates control action and input at a given index in the staged command draft.
         */
        bool updateControl(std::size_t index, std::string_view action, std::string_view input);

        /**
         * @brief Validates the staged draft against base configuration rules.
         * @return true if draft is valid without blocking errors.
         */
        bool validateDraft(const autoinput::ConfigData& baseConfig);

        /**
         * @brief Applies staged command changes to the target configuration if validation passes.
         * @return true if changes were applied, false if rejected due to validation errors.
         */
        bool applyCommandEdit(autoinput::ConfigData& targetConfig);
    };

    /**
     * @brief Renders the visual configuration graph viewer and relationship editor.
     *
     * @param config The mutable configuration to inspect and edit.
     * @param state The viewer/editor state and configuration.
     * @param viewerId Unique ImGui window/child identifier string.
     * @return true if configuration changes were applied to config, false otherwise.
     */
    bool renderConfigGraphViewer(autoinput::ConfigData& config, ConfigGraphViewerState& state,
                                 const char* viewerId = "ConfigGraphViewer");

    /**
     * @brief Renders the visual configuration graph viewer in read-only mode.
     *
     * @param config The configuration to inspect (read-only).
     * @param state The viewer state and configuration.
     * @param viewerId Unique ImGui window/child identifier string.
     * @return true if an interaction occurred (e.g. selection changed), false otherwise.
     */
    bool renderConfigGraphViewer(const autoinput::ConfigData& config, ConfigGraphViewerState& state,
                                 const char* viewerId = "ConfigGraphViewer");

} // namespace autoinput::ui::editors

#endif // INCLUDE_AUTOINPUT_UI_EDITORS_CONFIG_GRAPH_VIEWER_H

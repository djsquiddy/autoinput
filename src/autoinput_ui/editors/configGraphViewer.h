/**
 * @file configGraphViewer.h
 * @brief Read-only visual graph viewer and diagnostics inspector for AutoInput configuration data.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_EDITORS_CONFIG_GRAPH_VIEWER_H
#define INCLUDE_AUTOINPUT_UI_EDITORS_CONFIG_GRAPH_VIEWER_H

#include "autoinput/config/config.h"
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
    /**
     * @brief Severity levels for configuration diagnostic and validation findings.
     */
    enum class ConfigDiagnosticSeverity : std::uint8_t
    {
        Info,
        Warning,
        Error
    };

    /**
     * @brief Converts diagnostic severity enum to human-readable string.
     */
    [[nodiscard]] constexpr std::string_view configDiagnosticSeverityToString(
        ConfigDiagnosticSeverity severity) noexcept
    {
        switch (severity)
        {
        case ConfigDiagnosticSeverity::Info: return "Info";
        case ConfigDiagnosticSeverity::Warning: return "Warning";
        case ConfigDiagnosticSeverity::Error: return "Error";
        default: return "Unknown";
        }
    }

    /**
     * @brief A single diagnostic issue detected in configuration data or graph relationships.
     */
    struct ConfigDiagnosticIssue
    {
        ConfigDiagnosticSeverity severity{ ConfigDiagnosticSeverity::Warning };
        std::string message;
        std::string category; ///< e.g. "Command", "Control", "Sequence", "Input Conflict", "Global Settings"
        std::optional<std::size_t> commandIndex{ std::nullopt };
        std::optional<std::size_t> controlIndex{ std::nullopt };
        std::optional<std::size_t> sequenceIndex{ std::nullopt };
        std::optional<graph::NodeId> associatedNodeId{ std::nullopt };
        std::string suggestedFix;
    };

    /**
     * @brief Aggregate result of configuration diagnostics analysis.
     */
    struct ConfigDiagnosticsResult
    {
        std::vector<ConfigDiagnosticIssue> issues;

        [[nodiscard]] bool hasErrors() const noexcept
        {
            return std::ranges::any_of(issues, [](const auto& issue)
                                       { return issue.severity == ConfigDiagnosticSeverity::Error; });
        }

        [[nodiscard]] bool hasWarnings() const noexcept
        {
            return std::ranges::any_of(issues, [](const auto& issue)
                                       { return issue.severity == ConfigDiagnosticSeverity::Warning; });
        }

        [[nodiscard]] std::size_t errorCount() const noexcept
        {
            return static_cast<std::size_t>(std::ranges::count_if(
                issues, [](const auto& issue) { return issue.severity == ConfigDiagnosticSeverity::Error; }));
        }

        [[nodiscard]] std::size_t warningCount() const noexcept
        {
            return static_cast<std::size_t>(std::ranges::count_if(
                issues, [](const auto& issue) { return issue.severity == ConfigDiagnosticSeverity::Warning; }));
        }

        [[nodiscard]] std::size_t infoCount() const noexcept
        {
            return static_cast<std::size_t>(std::ranges::count_if(
                issues, [](const auto& issue) { return issue.severity == ConfigDiagnosticSeverity::Info; }));
        }
    };

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
     * @brief Analyzes ConfigData for potential configuration conflicts, missing fields, and anomalies.
     *
     * Detects:
     * - Duplicate start inputs across commands and sequences
     * - Commands sharing key or button input triggers
     * - Empty command names
     * - Missing command actions
     * - Controls with empty input bindings or actions
     * - Sequences with empty start keys or names
     * - Standard configuration validator warnings/errors
     *
     * @param config The configuration to evaluate.
     * @param doc Optional generated graph document used to map issues directly to NodeIds.
     * @return Aggregate diagnostic result.
     */
    [[nodiscard]] ConfigDiagnosticsResult analyzeConfigDiagnostics(const autoinput::ConfigData& config,
                                                                   const graph::GraphDocument* doc = nullptr);

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
     * @brief State container and controller for the read-only visual configuration graph viewer.
     */
    struct ConfigGraphViewerState
    {
        graph::GraphDocument graphDocument;
        graph::FallbackGraphViewerState viewerState;
        graph::ConfigGraphOptions adapterOptions{ graph::ConfigGraphOptions::defaults() };
        ConfigDiagnosticsResult diagnosticsResult;
        std::string statusMessage{ "Ready" };
        bool isGraphSynchronized{ false };

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
    };

    /**
     * @brief Renders the read-only visual configuration graph viewer.
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

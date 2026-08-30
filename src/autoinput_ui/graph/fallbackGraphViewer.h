/**
 * @file fallbackGraphViewer.h
 * @brief Simple dependency-free ImGui fallback graph viewer for GraphDocument.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_GRAPH_FALLBACK_GRAPH_VIEWER_H
#define INCLUDE_AUTOINPUT_UI_GRAPH_FALLBACK_GRAPH_VIEWER_H

#include "graphModel.h"
#include "graphValidator.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace autoinput::ui::graph
{
    /**
     * @brief Layout modes supported by the fallback graph viewer.
     */
    enum class FallbackGraphViewMode : std::uint8_t
    {
        Split,    ///< Split view with list, canvas, and inspector panels
        ListOnly, ///< Tabular list and details view
        Canvas    ///< 2D canvas preview and inspector
    };

    /**
     * @brief Converts view mode enum to string representation.
     */
    [[nodiscard]] constexpr std::string_view fallbackGraphViewModeToString(FallbackGraphViewMode mode) noexcept
    {
        switch (mode)
        {
        case FallbackGraphViewMode::Split: return "Split";
        case FallbackGraphViewMode::ListOnly: return "List";
        case FallbackGraphViewMode::Canvas: return "Canvas";
        default: return "Unknown";
        }
    }

    /**
     * @brief State container for interactive fallback graph viewer.
     */
    struct FallbackGraphViewerState
    {
        NodeId selectedNodeId{ InvalidNodeId };
        LinkId selectedLinkId{ InvalidLinkId };
        std::string searchFilter;
        std::optional<NodeKind> filterKind{ std::nullopt };
        FallbackGraphViewMode viewMode{ FallbackGraphViewMode::Split };
        bool showValidationDetails{ true };
        bool showOnlyProblematicNodes{ false };
        float canvasZoom{ 1.0F };
        NodePosition canvasOffset{ .x = 0.0F, .y = 0.0F };

        void selectNode(NodeId id) noexcept
        {
            selectedNodeId = id;
            selectedLinkId = InvalidLinkId;
        }

        void selectLink(LinkId id) noexcept { selectedLinkId = id; }

        void clearSelection() noexcept
        {
            selectedNodeId = InvalidNodeId;
            selectedLinkId = InvalidLinkId;
        }

        [[nodiscard]] bool isNodeSelected(NodeId id) const noexcept
        {
            return selectedNodeId != InvalidNodeId && selectedNodeId == id;
        }

        [[nodiscard]] bool isLinkSelected(LinkId id) const noexcept
        {
            return selectedLinkId != InvalidLinkId && selectedLinkId == id;
        }

        [[nodiscard]] bool hasSelection() const noexcept
        {
            return selectedNodeId != InvalidNodeId || selectedLinkId != InvalidLinkId;
        }
    };

    // --- Non-UI Pure Helper Functions (Deterministic & Testable) ---

    /**
     * @brief Formats a readable header string for a graph node.
     */
    [[nodiscard]] std::string formatNodeHeader(const GraphNode& node);

    /**
     * @brief Formats pin summary text (e.g., "in [Input]").
     */
    [[nodiscard]] std::string formatPinSummary(const GraphPin& pin);

    /**
     * @brief Formats link summary text (e.g., "Link #1: Start (out) -> RecordedEvent (in)").
     */
    [[nodiscard]] std::string formatLinkSummary(const GraphDocument& doc, const GraphLink& link);

    /**
     * @brief Extracts validation issues associated with a specific node.
     */
    [[nodiscard]] std::vector<ValidationIssue> getNodeValidationIssues(const ValidationResult& result, NodeId nodeId);

    /**
     * @brief Extracts validation issues associated with a specific link.
     */
    [[nodiscard]] std::vector<ValidationIssue> getLinkValidationIssues(const ValidationResult& result, LinkId linkId);

    /**
     * @brief Returns incoming link IDs connected to a node's input pins.
     */
    [[nodiscard]] std::vector<LinkId> getNodeIncomingLinks(const GraphDocument& doc, NodeId nodeId);

    /**
     * @brief Returns outgoing link IDs connected to a node's output pins.
     */
    [[nodiscard]] std::vector<LinkId> getNodeOutgoingLinks(const GraphDocument& doc, NodeId nodeId);

    /**
     * @brief Computes 2D bounding box (min and max coordinates) encompassing all graph nodes.
     */
    [[nodiscard]] std::pair<NodePosition, NodePosition> computeGraphBoundingBox(const GraphDocument& doc);

    /**
     * @brief Filters nodes by title/subtitle text substring and optional node kind.
     */
    [[nodiscard]] std::vector<NodeId> filterGraphNodes(const GraphDocument& doc, std::string_view searchFilter = {},
                                                       std::optional<NodeKind> kindFilter = std::nullopt,
                                                       const ValidationResult* validationResult = nullptr,
                                                       bool onlyProblematic = false);

    // --- UI Rendering Functions ---

    /**
     * @brief Renders the dependency-free Dear ImGui fallback graph viewer.
     * @param doc The graph document to display.
     * @param validationResult Validation findings to visualize alongside topology.
     * @param state Viewer state (selection, search filter, layout).
     * @param viewerId Unique ImGui window/child identifier string.
     */
    void renderFallbackGraphViewer(const GraphDocument& doc, const ValidationResult& validationResult,
                                   FallbackGraphViewerState& state, const char* viewerId = "FallbackGraphViewer");

    /**
     * @brief Overload that automatically executes graph validation before rendering.
     */
    void renderFallbackGraphViewer(const GraphDocument& doc, FallbackGraphViewerState& state,
                                   const ValidationOptions& validationOptions = ValidationOptions::sequenceGraph(),
                                   const char* viewerId = "FallbackGraphViewer");

} // namespace autoinput::ui::graph

#endif // INCLUDE_AUTOINPUT_UI_GRAPH_FALLBACK_GRAPH_VIEWER_H

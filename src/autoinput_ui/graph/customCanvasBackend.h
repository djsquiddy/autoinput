/**
 * @file customCanvasBackend.h
 * @brief Custom Dear ImGui draw-list based node canvas backend prototype.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_GRAPH_CUSTOM_CANVAS_BACKEND_H
#define INCLUDE_AUTOINPUT_UI_GRAPH_CUSTOM_CANVAS_BACKEND_H

#include "graphModel.h"
#include "nodeEditorBackend.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace autoinput::ui::graph
{
    // =========================================================================
    // Pure Non-UI Mathematical and Topological Helper Functions (Deterministic)
    // =========================================================================

    /**
     * @brief Transforms canvas-space coordinates to screen-space coordinates.
     * @param canvasPos Position in canvas coordinate space.
     * @param canvasOffset Viewport panning offset.
     * @param zoom Viewport zoom scale factor (default 1.0f).
     * @param screenOrigin Top-left coordinate of the canvas viewport on screen.
     * @return Screen-space coordinate position.
     */
    [[nodiscard]] constexpr NodePosition canvasToScreen(const NodePosition& canvasPos, const NodePosition& canvasOffset,
                                                        float zoom = 1.0F,
                                                        const NodePosition& screenOrigin = { .x = 0.0F,
                                                                                             .y = 0.0F }) noexcept
    {
        return NodePosition{
            .x = screenOrigin.x + canvasOffset.x + (canvasPos.x * zoom),
            .y = screenOrigin.y + canvasOffset.y + (canvasPos.y * zoom),
        };
    }

    /**
     * @brief Transforms screen-space coordinates to canvas-space coordinates.
     * @param screenPos Position in screen coordinate space.
     * @param canvasOffset Viewport panning offset.
     * @param zoom Viewport zoom scale factor.
     * @param screenOrigin Top-left coordinate of the canvas viewport on screen.
     * @return Canvas-space coordinate position.
     */
    [[nodiscard]] inline NodePosition screenToCanvas(const NodePosition& screenPos, const NodePosition& canvasOffset,
                                                     float zoom = 1.0F,
                                                     const NodePosition& screenOrigin = { .x = 0.0F,
                                                                                          .y = 0.0F }) noexcept
    {
        const float safeZoom = (std::abs(zoom) > 0.0001F) ? zoom : 1.0F;
        return NodePosition{
            .x = (screenPos.x - screenOrigin.x - canvasOffset.x) / safeZoom,
            .y = (screenPos.y - screenOrigin.y - canvasOffset.y) / safeZoom,
        };
    }

    /**
     * @brief Checks if a point in canvas space is within a node bounding rectangle.
     * @param point Point to hit-test (canvas space).
     * @param nodePos Top-left position of the node in canvas space.
     * @param nodeSize Width and height dimensions of the node.
     * @return True if the point lies inside or on the boundaries of the rectangle.
     */
    [[nodiscard]] constexpr bool isPointInNode(const NodePosition& point, const NodePosition& nodePos,
                                               const NodePosition& nodeSize) noexcept
    {
        return point.x >= nodePos.x && point.x <= (nodePos.x + nodeSize.x) && point.y >= nodePos.y &&
               point.y <= (nodePos.y + nodeSize.y);
    }

    /**
     * @brief Checks if a point is within a circular pin socket.
     * @param point Point to hit-test.
     * @param pinCenter Center coordinates of the pin socket.
     * @param radius Hit radius of the pin socket.
     * @return True if the point is within the pin circle.
     */
    [[nodiscard]] inline bool isPointInPin(const NodePosition& point, const NodePosition& pinCenter,
                                           float radius = 8.0F) noexcept
    {
        const float dx = point.x - pinCenter.x;
        const float dy = point.y - pinCenter.y;
        return ((dx * dx) + (dy * dy)) <= (radius * radius);
    }

    /**
     * @brief Computes the local offset of a pin socket relative to a node's top-left corner.
     * @param direction Input or Output pin direction.
     * @param nodeWidth Width of the node box.
     * @param pinIndex Index of the pin among pins of the same direction.
     * @param totalPins Total count of pins of this direction on the node.
     * @param headerHeight Vertical height of the node header bar.
     * @param pinSpacing Vertical distance allocated per pin slot.
     * @return Local (x, y) offset relative to node position.
     */
    // NOLINTBEGIN(bugprone-easily-swappable-parameters)
    [[nodiscard]] inline NodePosition computePinLocalOffset(PinDirection direction, float nodeWidth,
                                                            std::size_t pinIndex = 0, std::size_t totalPins = 1,
                                                            float headerHeight = 28.0F,
                                                            float pinSpacing = 24.0F) noexcept
    {
        const float x = (direction == PinDirection::Input) ? 0.0F : nodeWidth;
        const float effectiveSpacing = (totalPins > 0) ? pinSpacing : 24.0F;
        const float y = headerHeight + ((static_cast<float>(pinIndex) + 0.5F) * effectiveSpacing);
        return NodePosition{ .x = x, .y = y };
    }

    /**
     * @brief Computes the absolute position of a pin socket.
     * @param nodePos Top-left position of the node.
     * @param nodeWidth Width of the node box.
     * @param direction Input or Output pin direction.
     * @param pinIndex Index of the pin among pins of the same direction.
     * @param totalPins Total count of pins of this direction on the node.
     * @param headerHeight Vertical height of the node header bar.
     * @param pinSpacing Vertical distance allocated per pin slot.
     * @return Absolute position of the pin socket.
     */
    [[nodiscard]] inline NodePosition computePinPosition(const NodePosition& nodePos, float nodeWidth,
                                                         PinDirection direction, std::size_t pinIndex = 0,
                                                         std::size_t totalPins = 1, float headerHeight = 28.0F,
                                                         float pinSpacing = 24.0F) noexcept
    {
        const auto offset = computePinLocalOffset(direction, nodeWidth, pinIndex, totalPins, headerHeight, pinSpacing);
        return NodePosition{ .x = nodePos.x + offset.x, .y = nodePos.y + offset.y };
    }
    // NOLINTEND(bugprone-easily-swappable-parameters)

    /**
     * @brief Computes cubic Bézier curve control points connecting two pin endpoints.
     * @param startPos Starting endpoint coordinate (e.g., output pin).
     * @param endPos Ending endpoint coordinate (e.g., input pin).
     * @param defaultCurvature Horizontal curvature distance.
     * @return Pair containing control point 1 and control point 2.
     */
    [[nodiscard]] inline std::pair<NodePosition, NodePosition> computeLinkBezierControlPoints(
        const NodePosition& startPos, const NodePosition& endPos, float defaultCurvature = 50.0F) noexcept
    {
        const float dx = endPos.x - startPos.x;
        float curvature = defaultCurvature;
        if (dx < 0.0F)
        {
            // Reverse curve curvature expansion
            curvature = std::max(defaultCurvature, std::abs(dx) * 0.5F);
        }
        else
        {
            curvature = std::max(defaultCurvature, dx * 0.4F);
        }

        const NodePosition cp1{ .x = startPos.x + curvature, .y = startPos.y };
        const NodePosition cp2{ .x = endPos.x - curvature, .y = endPos.y };
        return { cp1, cp2 };
    }

    /**
     * @brief Applies mouse drag delta to a canvas coordinate.
     * @param currentPos Current position.
     * @param dragDelta Screen-space drag delta.
     * @param zoom Viewport zoom factor.
     * @return Updated coordinate position.
     */
    [[nodiscard]] inline NodePosition applyNodeDragDelta(const NodePosition& currentPos, const NodePosition& dragDelta,
                                                         float zoom = 1.0F) noexcept
    {
        const float safeZoom = (std::abs(zoom) > 0.0001F) ? zoom : 1.0F;
        return NodePosition{
            .x = currentPos.x + (dragDelta.x / safeZoom),
            .y = currentPos.y + (dragDelta.y / safeZoom),
        };
    }

    // =========================================================================
    // Custom Dear ImGui Draw-List Backend Implementation
    // =========================================================================

    /**
     * @brief In-project custom Dear ImGui draw-list based node editor canvas backend.
     */
    class CustomCanvasNodeEditorBackend : public INodeEditorBackend
    {
    public:
        CustomCanvasNodeEditorBackend();
        ~CustomCanvasNodeEditorBackend() override = default;
        CustomCanvasNodeEditorBackend(const CustomCanvasNodeEditorBackend&) = delete;
        CustomCanvasNodeEditorBackend& operator=(const CustomCanvasNodeEditorBackend&) = delete;
        CustomCanvasNodeEditorBackend(CustomCanvasNodeEditorBackend&&) = delete;
        CustomCanvasNodeEditorBackend& operator=(CustomCanvasNodeEditorBackend&&) = delete;

        [[nodiscard]] NodeEditorBackendType backendType() const noexcept override;
        [[nodiscard]] const NodeEditorCapabilities& capabilities() const noexcept override;

        void initialize() override;
        void shutdown() override;

        void beginCanvas(const char* editorId = "CustomCanvasNodeEditor") override;
        void endCanvas() override;

        void beginNode(NodeId nodeId) override;
        void endNode() override;

        void beginNodeTitle() override;
        void endNodeTitle() override;

        void beginInputPin(PinId pinId) override;
        void endInputPin() override;

        void beginOutputPin(PinId pinId) override;
        void endOutputPin() override;

        void drawLink(LinkId linkId, PinId startPinId, PinId endPinId) override;

        [[nodiscard]] std::optional<CreatedLinkEvent> queryCreatedLink() override;
        [[nodiscard]] std::optional<DeletedLinkEvent> queryDeletedLink() override;
        [[nodiscard]] std::vector<NodeId> querySelectedNodes() override;
        [[nodiscard]] std::vector<LinkId> querySelectedLinks() override;

        void setNodePosition(NodeId nodeId, const NodePosition& position) override;
        [[nodiscard]] std::optional<NodePosition> getNodePosition(NodeId nodeId) const override;

        // Custom canvas state inspection and manipulation helpers
        [[nodiscard]] const NodePosition& canvasOffset() const noexcept { return m_canvasOffset; }
        void setCanvasOffset(const NodePosition& offset) noexcept { m_canvasOffset = offset; }

        [[nodiscard]] float canvasZoom() const noexcept { return m_canvasZoom; }
        void setCanvasZoom(float zoom) noexcept { m_canvasZoom = std::clamp(zoom, 0.2F, 3.0F); }

        [[nodiscard]] NodeId selectedNodeId() const noexcept { return m_selectedNodeId; }
        void setSelectedNodeId(NodeId nodeId) noexcept { m_selectedNodeId = nodeId; }

        [[nodiscard]] LinkId selectedLinkId() const noexcept { return m_selectedLinkId; }
        void setSelectedLinkId(LinkId linkId) noexcept { m_selectedLinkId = linkId; }

        void clearSelection() noexcept
        {
            m_selectedNodeId = InvalidNodeId;
            m_selectedLinkId = InvalidLinkId;
        }

    private:
        struct PinRecord
        {
            PinId pinId{ InvalidPinId };
            NodeId nodeId{ InvalidNodeId };
            PinDirection direction{ PinDirection::Input };
            NodePosition screenPos{ .x = 0.0F, .y = 0.0F };
        };

        struct NodeRecord
        {
            NodeId nodeId{ InvalidNodeId };
            NodePosition canvasPos{ .x = 0.0F, .y = 0.0F };
            NodePosition screenPos{ .x = 0.0F, .y = 0.0F };
            NodePosition size{ .x = 160.0F, .y = 70.0F };
            std::string title;
            std::vector<PinId> inputPins;
            std::vector<PinId> outputPins;
        };

        NodeEditorCapabilities m_capabilities;
        NodePosition m_canvasOffset{ .x = 0.0F, .y = 0.0F };
        float m_canvasZoom{ 1.0F };
        NodePosition m_canvasScreenOrigin{ .x = 0.0F, .y = 0.0F };
        NodePosition m_canvasSize{ .x = 0.0F, .y = 0.0F };

        NodeId m_selectedNodeId{ InvalidNodeId };
        LinkId m_selectedLinkId{ InvalidLinkId };
        NodeId m_activeDraggingNodeId{ InvalidNodeId };

        std::unordered_map<NodeId, NodePosition> m_nodePositions;
        std::unordered_map<PinId, PinRecord> m_pins;

        // Frame building state
        std::optional<NodeRecord> m_currentNode;
        bool m_inNodeTitle{ false };
        std::optional<PinId> m_currentInputPin;
        std::optional<PinId> m_currentOutputPin;
    };

    /**
     * @brief Factory helper creating a new CustomCanvasNodeEditorBackend instance.
     */
    [[nodiscard]] std::unique_ptr<INodeEditorBackend> createCustomCanvasBackend();

} // namespace autoinput::ui::graph

#endif // INCLUDE_AUTOINPUT_UI_GRAPH_CUSTOM_CANVAS_BACKEND_H

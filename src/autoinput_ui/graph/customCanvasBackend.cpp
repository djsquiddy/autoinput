/**
 * @file customCanvasBackend.cpp
 * @brief Implementation of custom Dear ImGui draw-list node canvas backend prototype.
 * @author djsquiddy
 * @date August 2026
 */
#include "customCanvasBackend.h"

#include <algorithm>
#include <cmath>
#include <format>

#if __has_include(<imgui.h>)
#include <imgui.h>
#define AUTOINPUT_UI_INTERNAL_HAS_IMGUI 1
#endif

namespace autoinput::ui::graph
{
    CustomCanvasNodeEditorBackend::CustomCanvasNodeEditorBackend()
    {
        m_capabilities.isAvailable = true;
        m_capabilities.supportsCanvas = true;
        m_capabilities.supportsPositions = true;
        m_capabilities.supportsLinkCreationQuery = false;
        m_capabilities.supportsLinkDeletionQuery = false;
        m_capabilities.supportsSelectionQuery = true;
        m_capabilities.supportsGroups = false;
        m_capabilities.supportsComments = false;
        m_capabilities.supportsMinimap = false;
        m_capabilities.supportsZoom = true;
        m_capabilities.supportsMultiSelect = false;
        m_capabilities.backendName = "CustomCanvas";
        m_capabilities.description = "In-project Dear ImGui draw-list canvas backend prototype (dependency-free)";
    }

    NodeEditorBackendType CustomCanvasNodeEditorBackend::backendType() const noexcept
    {
        return NodeEditorBackendType::CustomCanvas;
    }

    const NodeEditorCapabilities& CustomCanvasNodeEditorBackend::capabilities() const noexcept
    {
        return m_capabilities;
    }

    void CustomCanvasNodeEditorBackend::initialize()
    {
        m_selectedNodeId = InvalidNodeId;
        m_selectedLinkId = InvalidLinkId;
        m_activeDraggingNodeId = InvalidNodeId;
        m_canvasOffset = { .x = 0.0F, .y = 0.0F };
        m_canvasZoom = 1.0F;
        m_nodePositions.clear();
        m_pins.clear();
    }

    void CustomCanvasNodeEditorBackend::shutdown()
    {
        m_nodePositions.clear();
        m_pins.clear();
        m_currentNode.reset();
    }

    void CustomCanvasNodeEditorBackend::beginCanvas(const char* editorId)
    {
        m_pins.clear();

#ifdef AUTOINPUT_UI_INTERNAL_HAS_IMGUI
        ImGui::PushID(editorId);

        ImVec2 avail = ImGui::GetContentRegionAvail();
        if (avail.x < 100.0F)
        {
            avail.x = 100.0F;
        }
        if (avail.y < 100.0F)
        {
            avail.y = 100.0F;
        }

        m_canvasSize = { .x = avail.x, .y = avail.y };
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        m_canvasScreenOrigin = { .x = p0.x, .y = p0.y };
        ImVec2 p1 = ImVec2(p0.x + avail.x, p0.y + avail.y);

        // Create an invisible button covering the canvas for mouse input hit-testing
        ImGui::InvisibleButton("CanvasBackgroundArea", avail,
                               ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonMiddle |
                                   ImGuiButtonFlags_MouseButtonRight);
        const bool isCanvasHovered = ImGui::IsItemHovered();
        const bool isCanvasActive = ImGui::IsItemActive();
        const ImGuiIO& io = ImGui::GetIO();

        // Canvas panning: middle-mouse drag, right-mouse drag, or left-mouse drag on empty space
        if (isCanvasActive &&
            (ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0F) ||
             ImGui::IsMouseDragging(ImGuiMouseButton_Right, 0.0F) ||
             (m_activeDraggingNodeId == InvalidNodeId && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0F))))
        {
            m_canvasOffset.x += io.MouseDelta.x;
            m_canvasOffset.y += io.MouseDelta.y;
        }

        // Canvas zoom via mouse wheel when hovered
        if (isCanvasHovered && io.MouseWheel != 0.0F)
        {
            const float newZoom = std::clamp(m_canvasZoom + (io.MouseWheel * 0.1F), 0.2F, 3.0F);
            m_canvasZoom = newZoom;
        }

        // Draw background canvas rectangle and border
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddRectFilled(p0, p1, IM_COL32(28, 30, 36, 255));
        drawList->AddRect(p0, p1, IM_COL32(55, 58, 68, 255));

        // Draw grid lines
        const float gridStep = 32.0F * m_canvasZoom;
        if (gridStep > 4.0F)
        {
            for (float x = std::fmod(m_canvasOffset.x, gridStep); x < avail.x; x += gridStep)
            {
                if (x >= 0.0F)
                {
                    drawList->AddLine(ImVec2(p0.x + x, p0.y), ImVec2(p0.x + x, p1.y), IM_COL32(40, 43, 52, 255));
                }
            }
            for (float y = std::fmod(m_canvasOffset.y, gridStep); y < avail.y; y += gridStep)
            {
                if (y >= 0.0F)
                {
                    drawList->AddLine(ImVec2(p0.x, p0.y + y), ImVec2(p1.x, p0.y + y), IM_COL32(40, 43, 52, 255));
                }
            }
        }
#endif
    }

    void CustomCanvasNodeEditorBackend::endCanvas()
    {
#ifdef AUTOINPUT_UI_INTERNAL_HAS_IMGUI
        // Reset active node drag if mouse button was released
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            m_activeDraggingNodeId = InvalidNodeId;
        }

        ImGui::PopID();
#endif
    }

    void CustomCanvasNodeEditorBackend::beginNode(NodeId nodeId)
    {
        NodeRecord node;
        node.nodeId = nodeId;

        auto it = m_nodePositions.find(nodeId);
        if (it != m_nodePositions.end())
        {
            node.canvasPos = it->second;
        }
        else
        {
            node.canvasPos = { .x = 0.0F, .y = 0.0F };
            m_nodePositions[nodeId] = node.canvasPos;
        }

        node.screenPos = canvasToScreen(node.canvasPos, m_canvasOffset, m_canvasZoom, m_canvasScreenOrigin);
        node.size = { .x = 160.0F * m_canvasZoom, .y = 70.0F * m_canvasZoom };
        m_currentNode = std::move(node);
        m_inNodeTitle = false;
        m_currentInputPin.reset();
        m_currentOutputPin.reset();
    }

    void CustomCanvasNodeEditorBackend::endNode()
    {
        if (!m_currentNode.has_value())
        {
            return;
        }

#ifdef AUTOINPUT_UI_INTERNAL_HAS_IMGUI
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        const auto& node = *m_currentNode;

        const std::size_t maxPinCount = std::max(node.inputPins.size(), node.outputPins.size());
        const float headerHeight = 28.0F * m_canvasZoom;
        const float pinSpacing = 24.0F * m_canvasZoom;
        const float contentHeight =
            std::max(40.0F * m_canvasZoom, static_cast<float>(maxPinCount) * pinSpacing + 10.0F);
        const float nodeWidth = 160.0F * m_canvasZoom;
        const float nodeHeight = headerHeight + contentHeight;

        const ImVec2 nodeMin(node.screenPos.x, node.screenPos.y);
        const ImVec2 nodeMax(node.screenPos.x + nodeWidth, node.screenPos.y + nodeHeight);
        const ImVec2 headerMax(node.screenPos.x + nodeWidth, node.screenPos.y + headerHeight);

        const ImGuiIO& io = ImGui::GetIO();
        const bool isNodeHovered = ImGui::IsMouseHoveringRect(nodeMin, nodeMax);

        // Selection handling
        if (isNodeHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            m_selectedNodeId = node.nodeId;
            m_selectedLinkId = InvalidLinkId;
            m_activeDraggingNodeId = node.nodeId;
        }

        // Dragging handling
        if (m_activeDraggingNodeId == node.nodeId && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0F))
        {
            const NodePosition dragDelta{ .x = io.MouseDelta.x, .y = io.MouseDelta.y };
            m_nodePositions[node.nodeId] = applyNodeDragDelta(m_nodePositions[node.nodeId], dragDelta, m_canvasZoom);
        }

        // Render node body shadow & background
        const bool isSelected = (m_selectedNodeId == node.nodeId);
        const float rounding = 6.0F * m_canvasZoom;

        // Node background
        drawList->AddRectFilled(nodeMin, nodeMax, IM_COL32(38, 42, 50, 245), rounding);

        // Header bar
        const ImU32 headerColor = isSelected ? IM_COL32(50, 80, 120, 255) : IM_COL32(46, 52, 64, 255);
        drawList->AddRectFilled(nodeMin, headerMax, headerColor, rounding, ImDrawFlags_RoundCornersTop);

        // Border / Selection outline
        const ImU32 borderColor = isSelected ? IM_COL32(255, 215, 0, 255) : IM_COL32(70, 76, 92, 255);
        const float borderWidth = isSelected ? 2.5F : 1.2F;
        drawList->AddRect(nodeMin, nodeMax, borderColor, rounding, 0, borderWidth);

        // Header divider line
        drawList->AddLine(ImVec2(nodeMin.x, headerMax.y), headerMax, IM_COL32(60, 68, 82, 255), 1.0F);

        // Render Title
        if (!node.title.empty())
        {
            const ImVec2 textPos(nodeMin.x + 8.0F * m_canvasZoom, nodeMin.y + 6.0F * m_canvasZoom);
            drawList->AddText(textPos, IM_COL32(240, 240, 245, 255), node.title.c_str());
        }

        // Render input pins
        const float pinRadius = 5.0F * m_canvasZoom;
        for (std::size_t i = 0; i < node.inputPins.size(); ++i)
        {
            const PinId pid = node.inputPins[i];
            const NodePosition pinPos = computePinPosition(node.screenPos, nodeWidth, PinDirection::Input, i,
                                                           node.inputPins.size(), headerHeight, pinSpacing);

            PinRecord pinRec;
            pinRec.pinId = pid;
            pinRec.nodeId = node.nodeId;
            pinRec.direction = PinDirection::Input;
            pinRec.screenPos = pinPos;
            m_pins[pid] = pinRec;

            const ImVec2 center(pinPos.x, pinPos.y);
            drawList->AddCircleFilled(center, pinRadius, IM_COL32(70, 160, 240, 255));
            drawList->AddCircle(center, pinRadius, IM_COL32(255, 255, 255, 200), 12, 1.2F);
        }

        // Render output pins
        for (std::size_t i = 0; i < node.outputPins.size(); ++i)
        {
            const PinId pid = node.outputPins[i];
            const NodePosition pinPos = computePinPosition(node.screenPos, nodeWidth, PinDirection::Output, i,
                                                           node.outputPins.size(), headerHeight, pinSpacing);

            PinRecord pinRec;
            pinRec.pinId = pid;
            pinRec.nodeId = node.nodeId;
            pinRec.direction = PinDirection::Output;
            pinRec.screenPos = pinPos;
            m_pins[pid] = pinRec;

            const ImVec2 center(pinPos.x, pinPos.y);
            drawList->AddCircleFilled(center, pinRadius, IM_COL32(240, 140, 60, 255));
            drawList->AddCircle(center, pinRadius, IM_COL32(255, 255, 255, 200), 12, 1.2F);
        }
#endif

        m_currentNode.reset();
    }

    void CustomCanvasNodeEditorBackend::beginNodeTitle()
    {
        m_inNodeTitle = true;
    }

    void CustomCanvasNodeEditorBackend::endNodeTitle()
    {
        m_inNodeTitle = false;
    }

    void CustomCanvasNodeEditorBackend::beginInputPin(PinId pinId)
    {
        m_currentInputPin = pinId;
        if (m_currentNode.has_value())
        {
            m_currentNode->inputPins.push_back(pinId);
        }
    }

    void CustomCanvasNodeEditorBackend::endInputPin()
    {
        m_currentInputPin.reset();
    }

    void CustomCanvasNodeEditorBackend::beginOutputPin(PinId pinId)
    {
        m_currentOutputPin = pinId;
        if (m_currentNode.has_value())
        {
            m_currentNode->outputPins.push_back(pinId);
        }
    }

    void CustomCanvasNodeEditorBackend::endOutputPin()
    {
        m_currentOutputPin.reset();
    }

    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
    void CustomCanvasNodeEditorBackend::drawLink(LinkId linkId, PinId startPinId, PinId endPinId)
    {
#ifdef AUTOINPUT_UI_INTERNAL_HAS_IMGUI
        NodePosition startPos{ .x = 0.0F, .y = 0.0F };
        NodePosition endPos{ .x = 0.0F, .y = 0.0F };

        const auto itStart = m_pins.find(startPinId);
        if (itStart != m_pins.end())
        {
            startPos = itStart->second.screenPos;
        }

        const auto itEnd = m_pins.find(endPinId);
        if (itEnd != m_pins.end())
        {
            endPos = itEnd->second.screenPos;
        }

        if (itStart == m_pins.end() || itEnd == m_pins.end())
        {
            return;
        }

        const auto [cp1, cp2] = computeLinkBezierControlPoints(startPos, endPos, 50.0F * m_canvasZoom);

        const bool isSelected = (m_selectedLinkId == linkId);
        const ImU32 linkColor = isSelected ? IM_COL32(255, 215, 0, 255) : IM_COL32(180, 185, 205, 220);
        const float thickness = (isSelected ? 3.0F : 2.0F) * m_canvasZoom;

        ImDrawList* drawList = ImGui::GetWindowDrawList();
        drawList->AddBezierCubic(ImVec2(startPos.x, startPos.y), ImVec2(cp1.x, cp1.y), ImVec2(cp2.x, cp2.y),
                                 ImVec2(endPos.x, endPos.y), linkColor, thickness);
#endif
    }

    std::optional<CreatedLinkEvent> CustomCanvasNodeEditorBackend::queryCreatedLink()
    {
        return std::nullopt;
    }

    std::optional<DeletedLinkEvent> CustomCanvasNodeEditorBackend::queryDeletedLink()
    {
        return std::nullopt;
    }

    std::vector<NodeId> CustomCanvasNodeEditorBackend::querySelectedNodes()
    {
        if (m_selectedNodeId != InvalidNodeId)
        {
            return { m_selectedNodeId };
        }
        return {};
    }

    std::vector<LinkId> CustomCanvasNodeEditorBackend::querySelectedLinks()
    {
        if (m_selectedLinkId != InvalidLinkId)
        {
            return { m_selectedLinkId };
        }
        return {};
    }

    void CustomCanvasNodeEditorBackend::setNodePosition(NodeId nodeId, const NodePosition& position)
    {
        m_nodePositions[nodeId] = position;
    }

    std::optional<NodePosition> CustomCanvasNodeEditorBackend::getNodePosition(NodeId nodeId) const
    {
        const auto it = m_nodePositions.find(nodeId);
        if (it != m_nodePositions.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

    std::unique_ptr<INodeEditorBackend> createCustomCanvasBackend()
    {
        return std::make_unique<CustomCanvasNodeEditorBackend>();
    }

} // namespace autoinput::ui::graph

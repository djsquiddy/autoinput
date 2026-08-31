/**
 * @file customCanvasBackendTest.cpp
 * @brief Unit tests for the custom Dear ImGui draw-list node canvas backend and math helpers.
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput_ui/graph/customCanvasBackend.h"
#include <gtest/gtest.h>

using namespace autoinput::ui::graph;

class CustomCanvasBackendTest : public ::testing::Test
{
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// =========================================================================
// Mathematical and Topological Pure Helper Tests
// =========================================================================

TEST_F(CustomCanvasBackendTest, CoordinateTransformCanvasToScreenAndScreenToCanvas)
{
    const NodePosition canvasPos{ 100.0F, 50.0F };
    const NodePosition offset{ 20.0F, -10.0F };
    const NodePosition origin{ 50.0F, 50.0F };
    const float zoom = 1.5F;

    // canvasToScreen: origin + offset + (canvasPos * zoom)
    // x = 50 + 20 + (100 * 1.5) = 220
    // y = 50 - 10 + (50 * 1.5) = 115
    const auto screenPos = canvasToScreen(canvasPos, offset, zoom, origin);
    EXPECT_FLOAT_EQ(screenPos.x, 220.0F);
    EXPECT_FLOAT_EQ(screenPos.y, 115.0F);

    // Roundtrip back to canvas space
    const auto roundtrip = screenToCanvas(screenPos, offset, zoom, origin);
    EXPECT_NEAR(roundtrip.x, canvasPos.x, 0.001F);
    EXPECT_NEAR(roundtrip.y, canvasPos.y, 0.001F);
}

TEST_F(CustomCanvasBackendTest, CoordinateTransformZeroAndDefaultZoom)
{
    const NodePosition canvasPos{ 300.0F, 400.0F };
    const NodePosition offset{ 0.0F, 0.0F };

    // Default zoom = 1.0f, default origin = (0, 0)
    const auto screenPos = canvasToScreen(canvasPos, offset);
    EXPECT_FLOAT_EQ(screenPos.x, 300.0F);
    EXPECT_FLOAT_EQ(screenPos.y, 400.0F);

    const auto canvasPosRound = screenToCanvas(screenPos, offset);
    EXPECT_FLOAT_EQ(canvasPosRound.x, 300.0F);
    EXPECT_FLOAT_EQ(canvasPosRound.y, 400.0F);

    // Degenerate zero zoom handling
    const auto safeCanvas = screenToCanvas(screenPos, offset, 0.0F);
    EXPECT_FLOAT_EQ(safeCanvas.x, 300.0F);
    EXPECT_FLOAT_EQ(safeCanvas.y, 400.0F);
}

TEST_F(CustomCanvasBackendTest, PointInNodeHitTesting)
{
    const NodePosition nodePos{ 50.0F, 50.0F };
    const NodePosition nodeSize{ 160.0F, 80.0F };

    // Inside point
    EXPECT_TRUE(isPointInNode({ 100.0F, 80.0F }, nodePos, nodeSize));

    // Corner / boundary points
    EXPECT_TRUE(isPointInNode({ 50.0F, 50.0F }, nodePos, nodeSize));
    EXPECT_TRUE(isPointInNode({ 210.0F, 130.0F }, nodePos, nodeSize));
    EXPECT_TRUE(isPointInNode({ 50.0F, 130.0F }, nodePos, nodeSize));
    EXPECT_TRUE(isPointInNode({ 210.0F, 50.0F }, nodePos, nodeSize));

    // Outside points
    EXPECT_FALSE(isPointInNode({ 49.9F, 50.0F }, nodePos, nodeSize));
    EXPECT_FALSE(isPointInNode({ 210.1F, 100.0F }, nodePos, nodeSize));
    EXPECT_FALSE(isPointInNode({ 100.0F, 49.9F }, nodePos, nodeSize));
    EXPECT_FALSE(isPointInNode({ 100.0F, 130.1F }, nodePos, nodeSize));
}

TEST_F(CustomCanvasBackendTest, PointInPinHitTesting)
{
    const NodePosition pinCenter{ 200.0F, 150.0F };
    const float radius = 8.0F;

    // Center point
    EXPECT_TRUE(isPointInPin(pinCenter, pinCenter, radius));

    // Inside circle
    EXPECT_TRUE(isPointInPin({ 204.0F, 153.0F }, pinCenter, radius));

    // Exact radius edge: dx=8, dy=0 -> 64 <= 64
    EXPECT_TRUE(isPointInPin({ 208.0F, 150.0F }, pinCenter, radius));

    // Outside circle
    EXPECT_FALSE(isPointInPin({ 209.0F, 150.0F }, pinCenter, radius));
    EXPECT_FALSE(isPointInPin({ 200.0F, 158.5F }, pinCenter, radius));
}

TEST_F(CustomCanvasBackendTest, ComputePinPositionAndOffset)
{
    const NodePosition nodePos{ 100.0F, 100.0F };
    const float nodeWidth = 160.0F;
    const float headerHeight = 28.0F;
    const float pinSpacing = 24.0F;

    // Input pin 0: left edge (x=0 relative to nodePos), y = headerHeight + 0.5 * pinSpacing = 28 + 12 = 40
    const auto inPin0 = computePinPosition(nodePos, nodeWidth, PinDirection::Input, 0, 2, headerHeight, pinSpacing);
    EXPECT_FLOAT_EQ(inPin0.x, 100.0F);
    EXPECT_FLOAT_EQ(inPin0.y, 140.0F);

    // Input pin 1: left edge, y = 28 + 1.5 * 24 = 64 -> nodePos.y + 64 = 164
    const auto inPin1 = computePinPosition(nodePos, nodeWidth, PinDirection::Input, 1, 2, headerHeight, pinSpacing);
    EXPECT_FLOAT_EQ(inPin1.x, 100.0F);
    EXPECT_FLOAT_EQ(inPin1.y, 164.0F);

    // Output pin 0: right edge (x=nodeWidth relative to nodePos) -> 100 + 160 = 260
    const auto outPin0 = computePinPosition(nodePos, nodeWidth, PinDirection::Output, 0, 1, headerHeight, pinSpacing);
    EXPECT_FLOAT_EQ(outPin0.x, 260.0F);
    EXPECT_FLOAT_EQ(outPin0.y, 140.0F);
}

TEST_F(CustomCanvasBackendTest, ComputeLinkBezierControlPoints)
{
    const NodePosition startPos{ 100.0F, 200.0F };
    const NodePosition endPos{ 300.0F, 250.0F };

    // Forward curve (dx = 200 > 0)
    const auto [cp1, cp2] = computeLinkBezierControlPoints(startPos, endPos, 50.0F);
    EXPECT_GT(cp1.x, startPos.x);
    EXPECT_LT(cp2.x, endPos.x);
    EXPECT_FLOAT_EQ(cp1.y, startPos.y);
    EXPECT_FLOAT_EQ(cp2.y, endPos.y);

    // Backward curve (dx = -150 < 0)
    const NodePosition backStartPos{ 300.0F, 200.0F };
    const NodePosition backEndPos{ 150.0F, 250.0F };
    const auto [backCp1, backCp2] = computeLinkBezierControlPoints(backStartPos, backEndPos, 50.0F);
    EXPECT_GT(backCp1.x, backStartPos.x);
    EXPECT_LT(backCp2.x, backEndPos.x);
}

TEST_F(CustomCanvasBackendTest, ApplyNodeDragDelta)
{
    const NodePosition currentPos{ 100.0F, 200.0F };
    const NodePosition delta{ 10.0F, -5.0F };

    // At 1.0x zoom
    const auto updated1 = applyNodeDragDelta(currentPos, delta, 1.0F);
    EXPECT_FLOAT_EQ(updated1.x, 110.0F);
    EXPECT_FLOAT_EQ(updated1.y, 195.0F);

    // At 2.0x zoom (screen delta translates to half canvas movement)
    const auto updated2 = applyNodeDragDelta(currentPos, delta, 2.0F);
    EXPECT_FLOAT_EQ(updated2.x, 105.0F);
    EXPECT_FLOAT_EQ(updated2.y, 197.5F);
}

// =========================================================================
// Backend Class Instance and Lifecycle Tests
// =========================================================================

TEST_F(CustomCanvasBackendTest, BackendCapabilitiesAndIdentification)
{
    auto backend = createCustomCanvasBackend();
    ASSERT_NE(backend, nullptr);

    EXPECT_EQ(backend->backendType(), NodeEditorBackendType::CustomCanvas);

    const auto& caps = backend->capabilities();
    EXPECT_TRUE(caps.isAvailable);
    EXPECT_TRUE(caps.supportsCanvas);
    EXPECT_TRUE(caps.supportsPositions);
    EXPECT_TRUE(caps.supportsSelectionQuery);
    EXPECT_TRUE(caps.supportsZoom);
    EXPECT_FALSE(caps.supportsLinkCreationQuery);
    EXPECT_FALSE(caps.supportsLinkDeletionQuery);
    EXPECT_FALSE(caps.supportsGroups);
    EXPECT_FALSE(caps.supportsComments);
    EXPECT_FALSE(caps.supportsMinimap);
    EXPECT_FALSE(caps.supportsMultiSelect);
    EXPECT_EQ(caps.backendName, "CustomCanvas");
    EXPECT_FALSE(caps.description.empty());
}

TEST_F(CustomCanvasBackendTest, NodePositionManagement)
{
    CustomCanvasNodeEditorBackend backend;
    backend.initialize();

    EXPECT_FALSE(backend.getNodePosition(1).has_value());

    backend.setNodePosition(1, { 150.0F, 250.0F });
    auto pos1 = backend.getNodePosition(1);
    ASSERT_TRUE(pos1.has_value());
    EXPECT_FLOAT_EQ(pos1->x, 150.0F);
    EXPECT_FLOAT_EQ(pos1->y, 250.0F);

    backend.setNodePosition(2, { -50.0F, 80.0F });
    auto pos2 = backend.getNodePosition(2);
    ASSERT_TRUE(pos2.has_value());
    EXPECT_FLOAT_EQ(pos2->x, -50.0F);
    EXPECT_FLOAT_EQ(pos2->y, 80.0F);

    backend.shutdown();
    EXPECT_FALSE(backend.getNodePosition(1).has_value());
}

TEST_F(CustomCanvasBackendTest, SelectionQueriesAndState)
{
    CustomCanvasNodeEditorBackend backend;
    backend.initialize();

    EXPECT_TRUE(backend.querySelectedNodes().empty());
    EXPECT_TRUE(backend.querySelectedLinks().empty());
    EXPECT_EQ(backend.selectedNodeId(), InvalidNodeId);
    EXPECT_EQ(backend.selectedLinkId(), InvalidLinkId);

    backend.setSelectedNodeId(42);
    EXPECT_EQ(backend.selectedNodeId(), 42);
    auto selectedNodes = backend.querySelectedNodes();
    ASSERT_EQ(selectedNodes.size(), 1U);
    EXPECT_EQ(selectedNodes[0], 42);

    backend.setSelectedLinkId(99);
    EXPECT_EQ(backend.selectedLinkId(), 99);
    auto selectedLinks = backend.querySelectedLinks();
    ASSERT_EQ(selectedLinks.size(), 1U);
    EXPECT_EQ(selectedLinks[0], 99);

    backend.clearSelection();
    EXPECT_TRUE(backend.querySelectedNodes().empty());
    EXPECT_TRUE(backend.querySelectedLinks().empty());
}

TEST_F(CustomCanvasBackendTest, CanvasOffsetAndZoomAccessors)
{
    CustomCanvasNodeEditorBackend backend;
    backend.initialize();

    EXPECT_FLOAT_EQ(backend.canvasZoom(), 1.0F);
    EXPECT_FLOAT_EQ(backend.canvasOffset().x, 0.0F);
    EXPECT_FLOAT_EQ(backend.canvasOffset().y, 0.0F);

    backend.setCanvasOffset({ 120.0F, -40.0F });
    EXPECT_FLOAT_EQ(backend.canvasOffset().x, 120.0F);
    EXPECT_FLOAT_EQ(backend.canvasOffset().y, -40.0F);

    backend.setCanvasZoom(1.8F);
    EXPECT_FLOAT_EQ(backend.canvasZoom(), 1.8F);

    // Zoom clamping
    backend.setCanvasZoom(10.0F);
    EXPECT_FLOAT_EQ(backend.canvasZoom(), 3.0F);
    backend.setCanvasZoom(0.05F);
    EXPECT_FLOAT_EQ(backend.canvasZoom(), 0.2F);
}

TEST_F(CustomCanvasBackendTest, SafeHeadlessRenderingLifecycle)
{
    CustomCanvasNodeEditorBackend backend;
    backend.initialize();

    // Call rendering lifecycle methods in headless environment (should be safe without GUI crash)
    backend.beginCanvas("TestCanvas");

    backend.beginNode(1);
    backend.beginNodeTitle();
    backend.endNodeTitle();
    backend.beginInputPin(10);
    backend.endInputPin();
    backend.beginOutputPin(11);
    backend.endOutputPin();
    backend.endNode();

    backend.beginNode(2);
    backend.beginInputPin(20);
    backend.endInputPin();
    backend.endNode();

    backend.drawLink(100, 11, 20);
    backend.endCanvas();

    EXPECT_FALSE(backend.queryCreatedLink().has_value());
    EXPECT_FALSE(backend.queryDeletedLink().has_value());

    backend.shutdown();
}

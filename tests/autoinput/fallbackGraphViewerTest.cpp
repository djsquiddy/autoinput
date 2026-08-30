/**
 * @file fallbackGraphViewerTest.cpp
 * @brief Unit tests for the dependency-free ImGui fallback graph viewer.
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>

#include "autoinput_ui/graph/fallbackGraphViewer.h"
#include "autoinput_ui/graph/graphModel.h"
#include "autoinput_ui/graph/graphValidator.h"

using namespace autoinput::ui::graph;

class FallbackGraphViewerTest : public ::testing::Test
{
protected:
    GraphDocument doc;
};

TEST_F(FallbackGraphViewerTest, StateDefaultInitializationAndTransitions)
{
    FallbackGraphViewerState state;
    EXPECT_EQ(state.selectedNodeId, InvalidNodeId);
    EXPECT_EQ(state.selectedLinkId, InvalidLinkId);
    EXPECT_TRUE(state.searchFilter.empty());
    EXPECT_FALSE(state.filterKind.has_value());
    EXPECT_EQ(state.viewMode, FallbackGraphViewMode::Split);
    EXPECT_TRUE(state.showValidationDetails);
    EXPECT_FALSE(state.showOnlyProblematicNodes);
    EXPECT_FALSE(state.hasSelection());

    state.selectNode(42);
    EXPECT_TRUE(state.hasSelection());
    EXPECT_TRUE(state.isNodeSelected(42));
    EXPECT_FALSE(state.isNodeSelected(10));
    EXPECT_EQ(state.selectedNodeId, 42);
    EXPECT_EQ(state.selectedLinkId, InvalidLinkId);

    state.selectLink(100);
    EXPECT_TRUE(state.hasSelection());
    EXPECT_TRUE(state.isLinkSelected(100));
    EXPECT_FALSE(state.isLinkSelected(200));
    EXPECT_EQ(state.selectedLinkId, 100);

    state.clearSelection();
    EXPECT_FALSE(state.hasSelection());
    EXPECT_FALSE(state.isNodeSelected(42));
    EXPECT_FALSE(state.isLinkSelected(100));
}

TEST_F(FallbackGraphViewerTest, ViewModeToString)
{
    EXPECT_EQ(fallbackGraphViewModeToString(FallbackGraphViewMode::Split), "Split");
    EXPECT_EQ(fallbackGraphViewModeToString(FallbackGraphViewMode::ListOnly), "List");
    EXPECT_EQ(fallbackGraphViewModeToString(FallbackGraphViewMode::Canvas), "Canvas");
    EXPECT_EQ(fallbackGraphViewModeToString(static_cast<FallbackGraphViewMode>(99)), "Unknown");
}

TEST_F(FallbackGraphViewerTest, FormatNodeHeader)
{
    GraphNode node1;
    node1.id = 1;
    node1.kind = NodeKind::Start;
    node1.title = "Sequence Start";
    EXPECT_EQ(formatNodeHeader(node1), "[Start] Sequence Start");

    GraphNode node2;
    node2.id = 5;
    node2.kind = NodeKind::Wait;
    node2.title = "";
    EXPECT_EQ(formatNodeHeader(node2), "[Wait] Node #5");
}

TEST_F(FallbackGraphViewerTest, FormatPinSummary)
{
    GraphPin pinIn;
    pinIn.id = 1;
    pinIn.name = "in";
    pinIn.direction = PinDirection::Input;
    EXPECT_EQ(formatPinSummary(pinIn), "in (Input)");

    GraphPin pinOut;
    pinOut.id = 2;
    pinOut.name = "out";
    pinOut.direction = PinDirection::Output;
    EXPECT_EQ(formatPinSummary(pinOut), "out (Output)");

    GraphPin unnamedPin;
    unnamedPin.id = 10;
    unnamedPin.name = "";
    unnamedPin.direction = PinDirection::Input;
    EXPECT_EQ(formatPinSummary(unnamedPin), "Pin #10 (Input)");
}

TEST_F(FallbackGraphViewerTest, FormatLinkSummary)
{
    auto& n1 = doc.createNode(NodeKind::Start, "StartNode", { 0.0F, 0.0F });
    auto* p1 = doc.createPin(n1.id, PinDirection::Output, "out");

    auto& n2 = doc.createNode(NodeKind::End, "EndNode", { 200.0F, 0.0F });
    auto* p2 = doc.createPin(n2.id, PinDirection::Input, "in");

    ASSERT_NE(p1, nullptr);
    ASSERT_NE(p2, nullptr);
    auto* link = doc.createLink(p1->id, p2->id);
    ASSERT_NE(link, nullptr);

    std::string summary = formatLinkSummary(doc, *link);
    EXPECT_NE(summary.find("Link #1"), std::string::npos);
    EXPECT_NE(summary.find("StartNode:out"), std::string::npos);
    EXPECT_NE(summary.find("EndNode:in"), std::string::npos);

    // Dangling link summary check
    GraphLink orphanLink{ .id = 99, .fromPinId = 999, .toPinId = 888 };
    std::string orphanSummary = formatLinkSummary(doc, orphanLink);
    EXPECT_NE(orphanSummary.find("Unknown Node:Unknown Pin"), std::string::npos);
}

TEST_F(FallbackGraphViewerTest, ExtractNodeAndLinkValidationIssues)
{
    ValidationResult res;
    res.issues.push_back(ValidationIssue{
        .severity = ValidationSeverity::Error, .message = "Node error message", .nodeId = 10, .linkId = std::nullopt });
    res.issues.push_back(ValidationIssue{ .severity = ValidationSeverity::Warning,
                                          .message = "Node warning message",
                                          .nodeId = 10,
                                          .linkId = std::nullopt });
    res.issues.push_back(ValidationIssue{
        .severity = ValidationSeverity::Error, .message = "Other node error", .nodeId = 20, .linkId = std::nullopt });
    res.issues.push_back(ValidationIssue{ .severity = ValidationSeverity::Error,
                                          .message = "Link error message",
                                          .nodeId = std::nullopt,
                                          .linkId = 100 });

    auto node10Issues = getNodeValidationIssues(res, 10);
    EXPECT_EQ(node10Issues.size(), 2U);
    EXPECT_EQ(node10Issues[0].message, "Node error message");
    EXPECT_EQ(node10Issues[1].message, "Node warning message");

    auto node20Issues = getNodeValidationIssues(res, 20);
    EXPECT_EQ(node20Issues.size(), 1U);
    EXPECT_EQ(node20Issues[0].message, "Other node error");

    auto node30Issues = getNodeValidationIssues(res, 30);
    EXPECT_TRUE(node30Issues.empty());

    auto link100Issues = getLinkValidationIssues(res, 100);
    EXPECT_EQ(link100Issues.size(), 1U);
    EXPECT_EQ(link100Issues[0].message, "Link error message");

    auto link200Issues = getLinkValidationIssues(res, 200);
    EXPECT_TRUE(link200Issues.empty());
}

TEST_F(FallbackGraphViewerTest, NodeIncomingAndOutgoingLinks)
{
    auto& n1 = doc.createNode(NodeKind::Start);
    auto* p1_out = doc.createPin(n1.id, PinDirection::Output, "out");

    auto& n2 = doc.createNode(NodeKind::RecordedEvent);
    auto* p2_in = doc.createPin(n2.id, PinDirection::Input, "in");
    auto* p2_out = doc.createPin(n2.id, PinDirection::Output, "out");

    auto& n3 = doc.createNode(NodeKind::End);
    auto* p3_in = doc.createPin(n3.id, PinDirection::Input, "in");

    ASSERT_NE(p1_out, nullptr);
    ASSERT_NE(p2_in, nullptr);
    ASSERT_NE(p2_out, nullptr);
    ASSERT_NE(p3_in, nullptr);

    auto* l1 = doc.createLink(p1_out->id, p2_in->id);
    auto* l2 = doc.createLink(p2_out->id, p3_in->id);
    ASSERT_NE(l1, nullptr);
    ASSERT_NE(l2, nullptr);

    auto in1 = getNodeIncomingLinks(doc, n1.id);
    auto out1 = getNodeOutgoingLinks(doc, n1.id);
    EXPECT_TRUE(in1.empty());
    ASSERT_EQ(out1.size(), 1U);
    EXPECT_EQ(out1[0], l1->id);

    auto in2 = getNodeIncomingLinks(doc, n2.id);
    auto out2 = getNodeOutgoingLinks(doc, n2.id);
    ASSERT_EQ(in2.size(), 1U);
    EXPECT_EQ(in2[0], l1->id);
    ASSERT_EQ(out2.size(), 1U);
    EXPECT_EQ(out2[0], l2->id);

    auto in3 = getNodeIncomingLinks(doc, n3.id);
    auto out3 = getNodeOutgoingLinks(doc, n3.id);
    ASSERT_EQ(in3.size(), 1U);
    EXPECT_EQ(in3[0], l2->id);
    EXPECT_TRUE(out3.empty());

    // Non-existent node
    EXPECT_TRUE(getNodeIncomingLinks(doc, 999).empty());
    EXPECT_TRUE(getNodeOutgoingLinks(doc, 999).empty());
}

TEST_F(FallbackGraphViewerTest, ComputeGraphBoundingBox)
{
    // Empty graph
    auto [emptyMin, emptyMax] = computeGraphBoundingBox(doc);
    EXPECT_FLOAT_EQ(emptyMin.x, 0.0F);
    EXPECT_FLOAT_EQ(emptyMin.y, 0.0F);
    EXPECT_FLOAT_EQ(emptyMax.x, 0.0F);
    EXPECT_FLOAT_EQ(emptyMax.y, 0.0F);

    // Multiple nodes
    doc.createNode(NodeKind::Start, "Start", { 10.0F, 20.0F });
    doc.createNode(NodeKind::RecordedEvent, "Event", { -50.0F, 100.0F });
    doc.createNode(NodeKind::End, "End", { 300.0F, -10.0F });

    auto [boxMin, boxMax] = computeGraphBoundingBox(doc);
    EXPECT_FLOAT_EQ(boxMin.x, -50.0F);
    EXPECT_FLOAT_EQ(boxMin.y, -10.0F);
    EXPECT_FLOAT_EQ(boxMax.x, 300.0F);
    EXPECT_FLOAT_EQ(boxMax.y, 100.0F);
}

TEST_F(FallbackGraphViewerTest, FilterGraphNodes)
{
    auto& n1 = doc.createNode(NodeKind::Start, "Start Node", { 0.0F, 0.0F });
    n1.subtitle = "Entry point";

    auto& n2 = doc.createNode(NodeKind::RecordedEvent, "Key Press A", { 100.0F, 0.0F });
    n2.subtitle = "Key event details";

    auto& n3 = doc.createNode(NodeKind::Wait, "Pause", { 200.0F, 0.0F });
    n3.subtitle = "Wait 500ms";

    auto& n4 = doc.createNode(NodeKind::End, "End Node", { 300.0F, 0.0F });
    n4.subtitle = "Termination";

    // Empty filter returns all
    auto allNodes = filterGraphNodes(doc);
    EXPECT_EQ(allNodes.size(), 4U);

    // Substring in title (case insensitive)
    auto keyFilter = filterGraphNodes(doc, "key");
    ASSERT_EQ(keyFilter.size(), 1U);
    EXPECT_EQ(keyFilter[0], n2.id);

    // Substring in subtitle
    auto waitFilter = filterGraphNodes(doc, "500ms");
    ASSERT_EQ(waitFilter.size(), 1U);
    EXPECT_EQ(waitFilter[0], n3.id);

    // Filter by NodeKind
    auto kindFilter = filterGraphNodes(doc, "", NodeKind::End);
    ASSERT_EQ(kindFilter.size(), 1U);
    EXPECT_EQ(kindFilter[0], n4.id);

    // Filter by ID
    auto idFilter = filterGraphNodes(doc, std::to_string(n1.id));
    EXPECT_FALSE(idFilter.empty());
    EXPECT_EQ(idFilter[0], n1.id);

    // Problematic nodes filter
    ValidationResult res;
    res.issues.push_back(
        ValidationIssue{ .severity = ValidationSeverity::Error, .message = "Broken", .nodeId = n2.id });

    auto issueFilter = filterGraphNodes(doc, "", std::nullopt, &res, true);
    ASSERT_EQ(issueFilter.size(), 1U);
    EXPECT_EQ(issueFilter[0], n2.id);
}

TEST_F(FallbackGraphViewerTest, RenderLifecycleHeadlessSafety)
{
    doc.createNode(NodeKind::Start);
    doc.createNode(NodeKind::End);

    FallbackGraphViewerState state;
    ValidationResult res = validateGraph(doc);

    // Verify executing the rendering entry point does not throw or crash in headless test environment
    EXPECT_NO_THROW(renderFallbackGraphViewer(doc, res, state, "TestViewer"));
    EXPECT_NO_THROW(renderFallbackGraphViewer(doc, state));
}

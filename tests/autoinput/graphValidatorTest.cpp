/**
 * @file graphValidatorTest.cpp
 * @brief Unit tests for the dependency-free UI graph validation utilities.
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput_ui/graph/graphValidator.h"
#include "autoinput_ui/graph/graphModel.h"
#include <gtest/gtest.h>

using namespace autoinput::ui::graph;

class GraphValidatorTest : public ::testing::Test
{
protected:
    GraphDocument m_doc;
};

TEST_F(GraphValidatorTest, SeverityStringConversion)
{
    EXPECT_EQ(validationSeverityToString(ValidationSeverity::Info), "Info");
    EXPECT_EQ(validationSeverityToString(ValidationSeverity::Warning), "Warning");
    EXPECT_EQ(validationSeverityToString(ValidationSeverity::Error), "Error");
}

TEST_F(GraphValidatorTest, ValidationResultHelpers)
{
    ValidationResult result;
    EXPECT_TRUE(result.isValid());
    EXPECT_FALSE(result.hasErrors());
    EXPECT_FALSE(result.hasWarnings());
    EXPECT_EQ(result.errorCount(), 0);
    EXPECT_EQ(result.warningCount(), 0);
    EXPECT_EQ(result.infoCount(), 0);

    result.issues.push_back(ValidationIssue{ .severity = ValidationSeverity::Info,
                                             .message = "Informational note",
                                             .nodeId = std::nullopt,
                                             .linkId = std::nullopt });
    EXPECT_TRUE(result.isValid());
    EXPECT_FALSE(result.hasErrors());
    EXPECT_EQ(result.infoCount(), 1);

    result.issues.push_back(ValidationIssue{
        .severity = ValidationSeverity::Warning, .message = "Warning note", .nodeId = 10, .linkId = std::nullopt });
    EXPECT_TRUE(result.isValid());
    EXPECT_FALSE(result.hasErrors());
    EXPECT_TRUE(result.hasWarnings());
    EXPECT_EQ(result.warningCount(), 1);

    result.issues.push_back(ValidationIssue{
        .severity = ValidationSeverity::Error, .message = "Critical error", .nodeId = std::nullopt, .linkId = 20 });
    EXPECT_FALSE(result.isValid());
    EXPECT_TRUE(result.hasErrors());
    EXPECT_EQ(result.errorCount(), 1);
}

TEST_F(GraphValidatorTest, ValidLinearSequenceGraph)
{
    // Build: Start -> Event -> Wait -> End
    auto& startNode = m_doc.createNode(NodeKind::Start, "Start");
    auto* startOut = m_doc.createPin(startNode.id, PinDirection::Output, "Out");

    auto& eventNode = m_doc.createNode(NodeKind::RecordedEvent, "Key Press");
    auto* eventIn = m_doc.createPin(eventNode.id, PinDirection::Input, "In");
    auto* eventOut = m_doc.createPin(eventNode.id, PinDirection::Output, "Out");

    auto& waitNode = m_doc.createNode(NodeKind::Wait, "Wait 500ms");
    auto* waitIn = m_doc.createPin(waitNode.id, PinDirection::Input, "In");
    auto* waitOut = m_doc.createPin(waitNode.id, PinDirection::Output, "Out");

    auto& endNode = m_doc.createNode(NodeKind::End, "End");
    auto* endIn = m_doc.createPin(endNode.id, PinDirection::Input, "In");

    ASSERT_NE(startOut, nullptr);
    ASSERT_NE(eventIn, nullptr);
    ASSERT_NE(eventOut, nullptr);
    ASSERT_NE(waitIn, nullptr);
    ASSERT_NE(waitOut, nullptr);
    ASSERT_NE(endIn, nullptr);

    m_doc.createLink(startOut->id, eventIn->id);
    m_doc.createLink(eventOut->id, waitIn->id);
    m_doc.createLink(waitOut->id, endIn->id);

    const auto result = validateGraph(m_doc, ValidationOptions::sequenceGraph());
    EXPECT_TRUE(result.isValid());
    EXPECT_FALSE(result.hasErrors());
    EXPECT_FALSE(result.hasWarnings());
    EXPECT_EQ(result.issues.size(), 0);
}

TEST_F(GraphValidatorTest, MissingRequiredStartNode)
{
    auto& endNode = m_doc.createNode(NodeKind::End, "End");
    auto* endIn = m_doc.createPin(endNode.id, PinDirection::Input, "In");
    ASSERT_NE(endIn, nullptr);

    const auto issues = validateStartNodes(m_doc, true, false);
    ASSERT_EQ(issues.size(), 1);
    EXPECT_EQ(issues[0].severity, ValidationSeverity::Error);
    EXPECT_FALSE(issues[0].nodeId.has_value());

    const auto result = validateGraph(m_doc, ValidationOptions::sequenceGraph());
    EXPECT_FALSE(result.isValid());
    EXPECT_TRUE(result.hasErrors());
}

TEST_F(GraphValidatorTest, MultipleStartNodes)
{
    auto& start1 = m_doc.createNode(NodeKind::Start, "Start 1");
    auto& start2 = m_doc.createNode(NodeKind::Start, "Start 2");

    const auto issues = validateStartNodes(m_doc, true, false);
    ASSERT_EQ(issues.size(), 1);
    EXPECT_EQ(issues[0].severity, ValidationSeverity::Error);
    EXPECT_EQ(issues[0].nodeId, start2.id);

    // If multiple start nodes are permitted (e.g. in configGraph)
    const auto allowedIssues = validateStartNodes(m_doc, true, true);
    EXPECT_TRUE(allowedIssues.empty());
}

TEST_F(GraphValidatorTest, MissingRequiredEndNode)
{
    auto& startNode = m_doc.createNode(NodeKind::Start, "Start");
    auto* startOut = m_doc.createPin(startNode.id, PinDirection::Output, "Out");
    ASSERT_NE(startOut, nullptr);

    const auto issues = validateEndNodes(m_doc, true);
    ASSERT_EQ(issues.size(), 1);
    EXPECT_EQ(issues[0].severity, ValidationSeverity::Error);

    // If end node is optional
    const auto optionalIssues = validateEndNodes(m_doc, false);
    EXPECT_TRUE(optionalIssues.empty());
}

TEST_F(GraphValidatorTest, DisconnectedNodesWarning)
{
    auto& startNode = m_doc.createNode(NodeKind::Start, "Start");
    auto* startOut = m_doc.createPin(startNode.id, PinDirection::Output, "Out");

    auto& endNode = m_doc.createNode(NodeKind::End, "End");
    auto* endIn = m_doc.createPin(endNode.id, PinDirection::Input, "In");

    m_doc.createLink(startOut->id, endIn->id);

    // Add a floating unattached wait node
    auto& floatingWait = m_doc.createNode(NodeKind::Wait, "Floating Wait");
    m_doc.createPin(floatingWait.id, PinDirection::Input, "In");

    const auto issues = validateDisconnectedNodes(m_doc, ValidationSeverity::Warning);
    ASSERT_EQ(issues.size(), 1);
    EXPECT_EQ(issues[0].severity, ValidationSeverity::Warning);
    EXPECT_EQ(issues[0].nodeId, floatingWait.id);

    // Verify comment nodes are not flagged as disconnected
    m_doc.createNode(NodeKind::Comment, "Notes: do not optimize");
    const auto issuesWithComment = validateDisconnectedNodes(m_doc, ValidationSeverity::Warning);
    EXPECT_EQ(issuesWithComment.size(), 1);
}

TEST_F(GraphValidatorTest, InvalidLinkPinReferences)
{
    auto& startNode = m_doc.createNode(NodeKind::Start, "Start");
    auto* startOut = m_doc.createPin(startNode.id, PinDirection::Output, "Out");
    ASSERT_NE(startOut, nullptr);

    // Manually push a link referencing non-existent pin ID
    m_doc.links().push_back(GraphLink{
        .id = 100,
        .fromPinId = startOut->id,
        .toPinId = 999999 // Non-existent pin
    });

    const auto issues = validateLinkReferences(m_doc);
    ASSERT_EQ(issues.size(), 1);
    EXPECT_EQ(issues[0].severity, ValidationSeverity::Error);
    EXPECT_EQ(issues[0].linkId, 100);
}

TEST_F(GraphValidatorTest, InvalidLinkDirections)
{
    auto& nodeA = m_doc.createNode(NodeKind::Command, "Node A");
    auto* outA = m_doc.createPin(nodeA.id, PinDirection::Output, "Out A");
    auto* inA = m_doc.createPin(nodeA.id, PinDirection::Input, "In A");

    auto& nodeB = m_doc.createNode(NodeKind::Command, "Node B");
    auto* outB = m_doc.createPin(nodeB.id, PinDirection::Output, "Out B");
    auto* inB = m_doc.createPin(nodeB.id, PinDirection::Input, "In B");

    // Output to Output
    auto* outToOutLink = m_doc.createLink(outA->id, outB->id);
    ASSERT_NE(outToOutLink, nullptr);

    // Input to Input
    auto* inToInLink = m_doc.createLink(inA->id, inB->id);
    ASSERT_NE(inToInLink, nullptr);

    // Input to Output
    auto* inToOutLink = m_doc.createLink(inA->id, outB->id);
    ASSERT_NE(inToOutLink, nullptr);

    const auto issues = validateLinkDirections(m_doc, false);
    EXPECT_EQ(issues.size(), 3);
    for (const auto& issue : issues)
    {
        EXPECT_EQ(issue.severity, ValidationSeverity::Error);
        EXPECT_TRUE(issue.linkId.has_value());
    }
}

TEST_F(GraphValidatorTest, SelfLinkValidation)
{
    auto& node = m_doc.createNode(NodeKind::Wait, "Self Wait");
    auto* inPin = m_doc.createPin(node.id, PinDirection::Input, "In");
    auto* outPin = m_doc.createPin(node.id, PinDirection::Output, "Out");

    auto* link = m_doc.createLink(outPin->id, inPin->id);
    ASSERT_NE(link, nullptr);

    const auto disallowedIssues = validateLinkDirections(m_doc, false);
    ASSERT_EQ(disallowedIssues.size(), 1);
    EXPECT_EQ(disallowedIssues[0].severity, ValidationSeverity::Error);
    EXPECT_EQ(disallowedIssues[0].nodeId, node.id);

    const auto allowedIssues = validateLinkDirections(m_doc, true);
    EXPECT_TRUE(allowedIssues.empty());
}

TEST_F(GraphValidatorTest, CycleDetectionMultiNode)
{
    // Start -> A -> B -> C -> A (cycle between A, B, C)
    auto& startNode = m_doc.createNode(NodeKind::Start, "Start");
    auto* startOut = m_doc.createPin(startNode.id, PinDirection::Output, "Out");

    auto& nodeA = m_doc.createNode(NodeKind::Command, "Step A");
    auto* inA = m_doc.createPin(nodeA.id, PinDirection::Input, "In A");
    auto* outA = m_doc.createPin(nodeA.id, PinDirection::Output, "Out A");

    auto& nodeB = m_doc.createNode(NodeKind::Command, "Step B");
    auto* inB = m_doc.createPin(nodeB.id, PinDirection::Input, "In B");
    auto* outB = m_doc.createPin(nodeB.id, PinDirection::Output, "Out B");

    auto& nodeC = m_doc.createNode(NodeKind::Command, "Step C");
    auto* inC = m_doc.createPin(nodeC.id, PinDirection::Input, "In C");
    auto* outC = m_doc.createPin(nodeC.id, PinDirection::Output, "Out C");

    m_doc.createLink(startOut->id, inA->id);
    m_doc.createLink(outA->id, inB->id);
    m_doc.createLink(outB->id, inC->id);
    m_doc.createLink(outC->id, inA->id); // Back-edge forming cycle

    const auto cycleIssues = validateAcyclic(m_doc);
    ASSERT_FALSE(cycleIssues.empty());
    EXPECT_EQ(cycleIssues[0].severity, ValidationSeverity::Error);

    const auto result = validateGraph(m_doc, ValidationOptions::sequenceGraph());
    EXPECT_FALSE(result.isValid());
    EXPECT_TRUE(result.hasErrors());
}

TEST_F(GraphValidatorTest, ConfigurableProfilesComparison)
{
    // Read-only config graph with multiple roots and no start/end nodes
    auto& filterNode = m_doc.createNode(NodeKind::ApplicationFilter, "Filter Window");
    auto* filterOut = m_doc.createPin(filterNode.id, PinDirection::Output, "Out");

    auto& groupNode = m_doc.createNode(NodeKind::ExclusiveGroup, "Exclusive 1");
    auto* groupIn = m_doc.createPin(groupNode.id, PinDirection::Input, "In");

    m_doc.createLink(filterOut->id, groupIn->id);

    // Floating comment node
    m_doc.createNode(NodeKind::Comment, "Config overview");

    // Sequence graph options: fails because Start/End are missing
    const auto seqResult = validateGraph(m_doc, ValidationOptions::sequenceGraph());
    EXPECT_FALSE(seqResult.isValid());
    EXPECT_TRUE(seqResult.hasErrors());

    // Config graph options: valid because Start/End are optional and acyclic/disconnected are relaxed
    const auto cfgResult = validateGraph(m_doc, ValidationOptions::configGraph());
    EXPECT_TRUE(cfgResult.isValid());
    EXPECT_FALSE(cfgResult.hasErrors());
    EXPECT_FALSE(cfgResult.hasWarnings());
}

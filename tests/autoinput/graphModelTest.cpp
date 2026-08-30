/**
 * @file graphModelTest.cpp
 * @brief Unit tests for the dependency-free UI graph document model.
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput_ui/graph/graphModel.h"
#include <gtest/gtest.h>

using namespace autoinput::ui::graph;

class GraphModelTest : public ::testing::Test
{
protected:
    GraphDocument m_doc;
};

TEST_F(GraphModelTest, InitialStateIsEmpty)
{
    EXPECT_TRUE(m_doc.empty());
    EXPECT_EQ(m_doc.nodeCount(), 0);
    EXPECT_EQ(m_doc.pinCount(), 0);
    EXPECT_EQ(m_doc.linkCount(), 0);
    EXPECT_TRUE(m_doc.nodes().empty());
    EXPECT_TRUE(m_doc.pins().empty());
    EXPECT_TRUE(m_doc.links().empty());
}

TEST_F(GraphModelTest, NodeKindStringConversions)
{
    EXPECT_EQ(nodeKindToString(NodeKind::Start), "Start");
    EXPECT_EQ(nodeKindToString(NodeKind::End), "End");
    EXPECT_EQ(nodeKindToString(NodeKind::RecordedEvent), "Recorded event");
    EXPECT_EQ(nodeKindToString(NodeKind::Wait), "Wait");
    EXPECT_EQ(nodeKindToString(NodeKind::Command), "Command");
    EXPECT_EQ(nodeKindToString(NodeKind::Control), "Control");
    EXPECT_EQ(nodeKindToString(NodeKind::Input), "Input");
    EXPECT_EQ(nodeKindToString(NodeKind::Sequence), "Sequence");
    EXPECT_EQ(nodeKindToString(NodeKind::ExclusiveGroup), "Exclusive group");
    EXPECT_EQ(nodeKindToString(NodeKind::ApplicationFilter), "Application filter");
    EXPECT_EQ(nodeKindToString(NodeKind::BlacklistEntry), "Blacklist entry");
    EXPECT_EQ(nodeKindToString(NodeKind::Comment), "Comment");
    EXPECT_EQ(nodeKindToString(NodeKind::Unknown), "Unknown");
}

TEST_F(GraphModelTest, PinDirectionStringConversions)
{
    EXPECT_EQ(pinDirectionToString(PinDirection::Input), "Input");
    EXPECT_EQ(pinDirectionToString(PinDirection::Output), "Output");
}

TEST_F(GraphModelTest, CreateNodesAndVerifyMetadata)
{
    auto& startNode = m_doc.createNode(NodeKind::Start, "Start Node", { .x = 10.0F, .y = 20.0F });
    EXPECT_NE(startNode.id, InvalidNodeId);
    EXPECT_EQ(startNode.kind, NodeKind::Start);
    EXPECT_EQ(startNode.title, "Start Node");
    EXPECT_FLOAT_EQ(startNode.position.x, 10.0F);
    EXPECT_FLOAT_EQ(startNode.position.y, 20.0F);
    EXPECT_FALSE(startNode.sourceIndex.has_value());

    startNode.setDetails("Trigger on hotkey");
    EXPECT_EQ(startNode.details(), "Trigger on hotkey");
    EXPECT_EQ(startNode.subtitle, "Trigger on hotkey");

    startNode.sourceIndex = 42;
    EXPECT_TRUE(startNode.sourceIndex.has_value());
    EXPECT_EQ(*startNode.sourceIndex, 42);

    auto meta = startNode.metadata();
    EXPECT_EQ(meta.title, "Start Node");
    EXPECT_EQ(meta.subtitle, "Trigger on hotkey");
    EXPECT_EQ(meta.sourceIndex, std::optional<std::size_t>(42));

    EXPECT_EQ(m_doc.nodeCount(), 1);
    EXPECT_FALSE(m_doc.empty());
}

TEST_F(GraphModelTest, CreatePinsOnNode)
{
    auto& node = m_doc.createNode(NodeKind::Command, "Run Cmd", { .x = 50.0F, .y = 100.0F });
    const NodeId nodeId = node.id;

    auto* inPin = m_doc.createPin(nodeId, PinDirection::Input, "Exec In");
    ASSERT_NE(inPin, nullptr);
    EXPECT_NE(inPin->id, InvalidPinId);
    EXPECT_EQ(inPin->nodeId, nodeId);
    EXPECT_EQ(inPin->direction, PinDirection::Input);
    EXPECT_EQ(inPin->name, "Exec In");

    auto* outPin = m_doc.createPin(nodeId, PinDirection::Output, "Exec Out");
    ASSERT_NE(outPin, nullptr);
    EXPECT_NE(outPin->id, InvalidPinId);
    EXPECT_NE(outPin->id, inPin->id);
    EXPECT_EQ(outPin->nodeId, nodeId);
    EXPECT_EQ(outPin->direction, PinDirection::Output);
    EXPECT_EQ(outPin->name, "Exec Out");

    EXPECT_EQ(m_doc.pinCount(), 2);

    // Verify parent node pin tracking
    const auto* foundNode = m_doc.findNode(nodeId);
    ASSERT_NE(foundNode, nullptr);
    ASSERT_EQ(foundNode->pinIds.size(), 2);
    EXPECT_EQ(foundNode->pinIds[0], inPin->id);
    EXPECT_EQ(foundNode->pinIds[1], outPin->id);

    // Verify pin ownership helper
    EXPECT_TRUE(m_doc.isPinOnNode(inPin->id, nodeId));
    EXPECT_TRUE(m_doc.pinBelongsToNode(outPin->id, nodeId));
    EXPECT_FALSE(m_doc.pinBelongsToNode(inPin->id, 999999));
    EXPECT_FALSE(m_doc.pinBelongsToNode(999999, nodeId));
}

TEST_F(GraphModelTest, CreatePinOnNonExistentNodeFails)
{
    auto* pin = m_doc.createPin(999, PinDirection::Input, "Orphan");
    EXPECT_EQ(pin, nullptr);
    EXPECT_EQ(m_doc.pinCount(), 0);
}

TEST_F(GraphModelTest, CreateLinksBetweenPins)
{
    auto& nodeA = m_doc.createNode(NodeKind::Start, "Start");
    auto& nodeB = m_doc.createNode(NodeKind::Wait, "Wait");

    auto* outPinA = m_doc.createPin(nodeA.id, PinDirection::Output, "Out");
    auto* inPinB = m_doc.createPin(nodeB.id, PinDirection::Input, "In");
    ASSERT_NE(outPinA, nullptr);
    ASSERT_NE(inPinB, nullptr);

    auto* link = m_doc.createLink(outPinA->id, inPinB->id);
    ASSERT_NE(link, nullptr);
    EXPECT_NE(link->id, InvalidLinkId);
    EXPECT_EQ(link->fromPinId, outPinA->id);
    EXPECT_EQ(link->toPinId, inPinB->id);

    EXPECT_EQ(m_doc.linkCount(), 1);

    auto* foundLink = m_doc.findLink(link->id);
    ASSERT_NE(foundLink, nullptr);
    EXPECT_EQ(foundLink->id, link->id);

    auto* foundBetween = m_doc.findLinkBetween(outPinA->id, inPinB->id);
    ASSERT_NE(foundBetween, nullptr);
    EXPECT_EQ(foundBetween->id, link->id);
}

TEST_F(GraphModelTest, CreateLinkFailureCases)
{
    auto& node = m_doc.createNode(NodeKind::Command, "Cmd");
    auto* pin = m_doc.createPin(node.id, PinDirection::Output, "Out");
    ASSERT_NE(pin, nullptr);

    // Self link
    EXPECT_EQ(m_doc.createLink(pin->id, pin->id), nullptr);

    // Invalid / zero pin IDs
    EXPECT_EQ(m_doc.createLink(InvalidPinId, pin->id), nullptr);
    EXPECT_EQ(m_doc.createLink(pin->id, InvalidPinId), nullptr);

    // Non-existent pins
    EXPECT_EQ(m_doc.createLink(pin->id, 9999), nullptr);
    EXPECT_EQ(m_doc.createLink(8888, pin->id), nullptr);

    EXPECT_EQ(m_doc.linkCount(), 0);
}

TEST_F(GraphModelTest, RemoveNodeRemovesAttachedPinsAndLinks)
{
    // Build: Node1 -> Node2 -> Node3
    auto& node1 = m_doc.createNode(NodeKind::Start, "Start");
    auto& node2 = m_doc.createNode(NodeKind::Input, "Input");
    auto& node3 = m_doc.createNode(NodeKind::End, "End");

    auto* p1Out = m_doc.createPin(node1.id, PinDirection::Output, "out");
    auto* p2In = m_doc.createPin(node2.id, PinDirection::Input, "in");
    auto* p2Out = m_doc.createPin(node2.id, PinDirection::Output, "out");
    auto* p3In = m_doc.createPin(node3.id, PinDirection::Input, "in");

    ASSERT_NE(p1Out, nullptr);
    ASSERT_NE(p2In, nullptr);
    ASSERT_NE(p2Out, nullptr);
    ASSERT_NE(p3In, nullptr);

    auto* link1 = m_doc.createLink(p1Out->id, p2In->id);
    auto* link2 = m_doc.createLink(p2Out->id, p3In->id);
    ASSERT_NE(link1, nullptr);
    ASSERT_NE(link2, nullptr);

    const LinkId link1Id = link1->id;
    const LinkId link2Id = link2->id;
    const PinId p2InId = p2In->id;
    const PinId p2OutId = p2Out->id;
    const NodeId node2Id = node2.id;

    EXPECT_EQ(m_doc.nodeCount(), 3);
    EXPECT_EQ(m_doc.pinCount(), 4);
    EXPECT_EQ(m_doc.linkCount(), 2);

    // Remove middle node (node2)
    EXPECT_TRUE(m_doc.removeNode(node2Id));

    // Verify node2 is gone
    EXPECT_EQ(m_doc.findNode(node2Id), nullptr);
    EXPECT_EQ(m_doc.nodeCount(), 2);

    // Verify node2's pins are gone
    EXPECT_EQ(m_doc.findPin(p2InId), nullptr);
    EXPECT_EQ(m_doc.findPin(p2OutId), nullptr);
    EXPECT_EQ(m_doc.pinCount(), 2);

    // Verify all links attached to node2's pins are gone
    EXPECT_EQ(m_doc.findLink(link1Id), nullptr);
    EXPECT_EQ(m_doc.findLink(link2Id), nullptr);
    EXPECT_EQ(m_doc.linkCount(), 0);

    // Verify remaining nodes and pins are intact
    EXPECT_NE(m_doc.findNode(node1.id), nullptr);
    EXPECT_NE(m_doc.findNode(node3.id), nullptr);
    EXPECT_NE(m_doc.findPin(p1Out->id), nullptr);
    EXPECT_NE(m_doc.findPin(p3In->id), nullptr);
}

TEST_F(GraphModelTest, RemovePinRemovesConnectedLinksAndUpdatesParentNode)
{
    auto& nodeA = m_doc.createNode(NodeKind::Sequence, "Seq");
    auto& nodeB = m_doc.createNode(NodeKind::Control, "Ctrl");

    auto* pinA = m_doc.createPin(nodeA.id, PinDirection::Output, "out");
    auto* pinB = m_doc.createPin(nodeB.id, PinDirection::Input, "in");
    ASSERT_NE(pinA, nullptr);
    ASSERT_NE(pinB, nullptr);

    auto* link = m_doc.createLink(pinA->id, pinB->id);
    ASSERT_NE(link, nullptr);
    const LinkId linkId = link->id;
    const PinId pinAId = pinA->id;

    EXPECT_EQ(m_doc.pinCount(), 2);
    EXPECT_EQ(m_doc.linkCount(), 1);

    EXPECT_TRUE(m_doc.removePin(pinAId));

    EXPECT_EQ(m_doc.findPin(pinAId), nullptr);
    EXPECT_EQ(m_doc.findLink(linkId), nullptr);
    EXPECT_EQ(m_doc.pinCount(), 1);
    EXPECT_EQ(m_doc.linkCount(), 0);

    // Check parent node's pin list was updated
    const auto* foundNodeA = m_doc.findNode(nodeA.id);
    ASSERT_NE(foundNodeA, nullptr);
    EXPECT_TRUE(foundNodeA->pinIds.empty());
}

TEST_F(GraphModelTest, RemoveLinkIndividually)
{
    auto& nodeA = m_doc.createNode(NodeKind::ExclusiveGroup, "Group");
    auto& nodeB = m_doc.createNode(NodeKind::ApplicationFilter, "Filter");

    auto* pinA = m_doc.createPin(nodeA.id, PinDirection::Output, "out");
    auto* pinB = m_doc.createPin(nodeB.id, PinDirection::Input, "in");
    ASSERT_NE(pinA, nullptr);
    ASSERT_NE(pinB, nullptr);

    auto* link = m_doc.createLink(pinA->id, pinB->id);
    ASSERT_NE(link, nullptr);
    const LinkId linkId = link->id;

    EXPECT_EQ(m_doc.linkCount(), 1);
    EXPECT_TRUE(m_doc.removeLink(linkId));
    EXPECT_EQ(m_doc.linkCount(), 0);
    EXPECT_EQ(m_doc.findLink(linkId), nullptr);
    EXPECT_FALSE(m_doc.removeLink(linkId)); // Already removed
}

TEST_F(GraphModelTest, LookupFailureCases)
{
    EXPECT_EQ(m_doc.findNode(InvalidNodeId), nullptr);
    EXPECT_EQ(m_doc.findNode(9999), nullptr);

    EXPECT_EQ(m_doc.findPin(InvalidPinId), nullptr);
    EXPECT_EQ(m_doc.findPin(8888), nullptr);

    EXPECT_EQ(m_doc.findLink(InvalidLinkId), nullptr);
    EXPECT_EQ(m_doc.findLink(7777), nullptr);

    EXPECT_EQ(m_doc.findLinkBetween(1, 2), nullptr);
    EXPECT_EQ(m_doc.findLinkBetween(InvalidPinId, InvalidPinId), nullptr);

    EXPECT_FALSE(m_doc.removeNode(InvalidNodeId));
    EXPECT_FALSE(m_doc.removeNode(9999));
    EXPECT_FALSE(m_doc.removePin(InvalidPinId));
    EXPECT_FALSE(m_doc.removePin(8888));
    EXPECT_FALSE(m_doc.removeLink(InvalidLinkId));
    EXPECT_FALSE(m_doc.removeLink(7777));
}

TEST_F(GraphModelTest, StableIdsDoNotDependOnVectorIndex)
{
    // Create 5 nodes
    auto& n1 = m_doc.createNode(NodeKind::Start, "Node 1");
    auto& n2 = m_doc.createNode(NodeKind::RecordedEvent, "Node 2");
    auto& n3 = m_doc.createNode(NodeKind::Wait, "Node 3");
    auto& n4 = m_doc.createNode(NodeKind::Command, "Node 4");
    auto& n5 = m_doc.createNode(NodeKind::End, "Node 5");

    const NodeId id1 = n1.id;
    const NodeId id2 = n2.id;
    const NodeId id3 = n3.id;
    const NodeId id4 = n4.id;
    const NodeId id5 = n5.id;

    // Verify IDs are distinct and positive
    EXPECT_NE(id1, id2);
    EXPECT_NE(id2, id3);
    EXPECT_NE(id3, id4);
    EXPECT_NE(id4, id5);

    // Delete node 2 and node 4 (vector elements shift)
    EXPECT_TRUE(m_doc.removeNode(id2));
    EXPECT_TRUE(m_doc.removeNode(id4));

    EXPECT_EQ(m_doc.nodeCount(), 3);

    // Lookups by original IDs should find exactly the right nodes, unaffected by position shifts
    const auto* found1 = m_doc.findNode(id1);
    ASSERT_NE(found1, nullptr);
    EXPECT_EQ(found1->id, id1);
    EXPECT_EQ(found1->title, "Node 1");

    EXPECT_EQ(m_doc.findNode(id2), nullptr);

    const auto* found3 = m_doc.findNode(id3);
    ASSERT_NE(found3, nullptr);
    EXPECT_EQ(found3->id, id3);
    EXPECT_EQ(found3->title, "Node 3");

    EXPECT_EQ(m_doc.findNode(id4), nullptr);

    const auto* found5 = m_doc.findNode(id5);
    ASSERT_NE(found5, nullptr);
    EXPECT_EQ(found5->id, id5);
    EXPECT_EQ(found5->title, "Node 5");

    // Add a new node - ID must not collide with existing or reused IDs
    auto& n6 = m_doc.createNode(NodeKind::Comment, "Node 6");
    EXPECT_NE(n6.id, id1);
    EXPECT_NE(n6.id, id3);
    EXPECT_NE(n6.id, id5);
    EXPECT_NE(m_doc.findNode(n6.id), nullptr);
}

TEST_F(GraphModelTest, ExplicitIdCreationAndCollisionHandling)
{
    // Create node with explicit ID
    auto* customNode = m_doc.createNodeWithId(100, NodeKind::BlacklistEntry, "Blacklist", { .x = 1.0F, .y = 2.0F });
    ASSERT_NE(customNode, nullptr);
    EXPECT_EQ(customNode->id, 100);
    EXPECT_EQ(customNode->kind, NodeKind::BlacklistEntry);

    // Duplicate ID should fail
    EXPECT_EQ(m_doc.createNodeWithId(100, NodeKind::Comment, "Duplicate"), nullptr);

    // Invalid ID 0 should fail
    EXPECT_EQ(m_doc.createNodeWithId(InvalidNodeId, NodeKind::Comment, "Zero"), nullptr);

    // Explicit pin creation
    auto* customPin = m_doc.createPinWithId(500, 100, PinDirection::Input, "customPin");
    ASSERT_NE(customPin, nullptr);
    EXPECT_EQ(customPin->id, 500);
    EXPECT_EQ(customPin->nodeId, 100);

    // Duplicate pin ID fails
    EXPECT_EQ(m_doc.createPinWithId(500, 100, PinDirection::Output, "dup"), nullptr);

    // Explicit pin with invalid node fails
    EXPECT_EQ(m_doc.createPinWithId(501, 9999, PinDirection::Output, "orphan"), nullptr);

    // Another node & pin for link
    auto* node2 = m_doc.createNodeWithId(200, NodeKind::Comment, "Comment");
    ASSERT_NE(node2, nullptr);
    auto* pin2 = m_doc.createPinWithId(600, 200, PinDirection::Output, "pin2");
    ASSERT_NE(pin2, nullptr);

    // Explicit link creation
    auto* customLink = m_doc.createLinkWithId(800, 600, 500);
    ASSERT_NE(customLink, nullptr);
    EXPECT_EQ(customLink->id, 800);
    EXPECT_EQ(customLink->fromPinId, 600);
    EXPECT_EQ(customLink->toPinId, 500);

    // Duplicate link ID fails
    EXPECT_EQ(m_doc.createLinkWithId(800, 600, 500), nullptr);

    // Next auto-allocated IDs should not collide
    auto& autoNode = m_doc.createNode(NodeKind::Start, "Auto");
    EXPECT_GT(autoNode.id, 200);

    auto* autoPin = m_doc.createPin(autoNode.id, PinDirection::Output, "AutoPin");
    ASSERT_NE(autoPin, nullptr);
    EXPECT_GT(autoPin->id, 600);

    auto* autoLink = m_doc.createLink(autoPin->id, 500);
    ASSERT_NE(autoLink, nullptr);
    EXPECT_GT(autoLink->id, 800);
}

TEST_F(GraphModelTest, ClearEmptiesEverythingAndResetsCounters)
{
    auto& node = m_doc.createNode(NodeKind::Start, "Start");
    auto* pin = m_doc.createPin(node.id, PinDirection::Output, "out");
    ASSERT_NE(pin, nullptr);

    auto& node2 = m_doc.createNode(NodeKind::End, "End");
    auto* pin2 = m_doc.createPin(node2.id, PinDirection::Input, "in");
    ASSERT_NE(pin2, nullptr);

    ASSERT_NE(m_doc.createLink(pin->id, pin2->id), nullptr);

    EXPECT_FALSE(m_doc.empty());
    m_doc.clear();

    EXPECT_TRUE(m_doc.empty());
    EXPECT_EQ(m_doc.nodeCount(), 0);
    EXPECT_EQ(m_doc.pinCount(), 0);
    EXPECT_EQ(m_doc.linkCount(), 0);
}

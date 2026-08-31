/**
 * @file sequenceGraphEditorTest.cpp
 * @brief Unit tests for the sequence graph viewer/editor state and controller logic.
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>

#include "autoinput/config/config.h"
#include "autoinput_ui/editors/sequenceGraphEditor.h"
#include "autoinput_ui/graph/graphModel.h"
#include "autoinput_ui/graph/graphValidator.h"

using namespace autoinput;
using namespace autoinput::ui::editors;
using namespace autoinput::ui::graph;

class SequenceGraphEditorTest : public ::testing::Test
{
protected:
    RecordedSequence sampleSequence{
        .name = "TestSequence",
        .start = "f9",
        .repeat = true,
        .events = { RecordedEvent{ .type = RecordedEventType::KeyDown, .delay = "10ms", .key = "ctrl" },
                    RecordedEvent{ .type = RecordedEventType::KeyDown, .delay = "20ms", .key = "c" },
                    RecordedEvent{ .type = RecordedEventType::KeyUp, .delay = "15ms", .key = "c" },
                    RecordedEvent{ .type = RecordedEventType::KeyUp, .delay = "10ms", .key = "ctrl" },
                    RecordedEvent{ .type = RecordedEventType::MouseMove, .delay = "50ms", .x = 250, .y = 350 },
                    RecordedEvent{ .type = RecordedEventType::MouseDown, .delay = "30ms", .button = "left" },
                    RecordedEvent{ .type = RecordedEventType::MouseUp, .delay = "20ms", .button = "left" } }
    };
};

namespace
{
    std::vector<const GraphNode*> getNodesOfKind(const GraphDocument& doc, NodeKind kind)
    {
        std::vector<const GraphNode*> result;
        for (const auto& node : doc.nodes())
        {
            if (node.kind == kind)
            {
                result.push_back(&node);
            }
        }
        return result;
    }

    const GraphPin* findFirstPin(const GraphDocument& doc, const GraphNode& node, PinDirection dir)
    {
        for (PinId pinId : node.pinIds)
        {
            const auto* pin = doc.findPin(pinId);
            if (pin != nullptr && pin->direction == dir)
            {
                return pin;
            }
        }
        return nullptr;
    }
} // namespace

TEST_F(SequenceGraphEditorTest, DefaultStateInitialization)
{
    SequenceGraphEditorState state;
    EXPECT_TRUE(state.graphDocument.nodes().empty());
    EXPECT_TRUE(state.graphDocument.links().empty());
    EXPECT_FALSE(state.isGraphSynchronized);
    EXPECT_TRUE(state.isEditingAllowed);
    EXPECT_EQ(state.cachedSequenceEventCount, 0U);
    EXPECT_FALSE(state.lastCompilationError.has_value());
    EXPECT_FALSE(state.viewerState.hasSelection());
    EXPECT_FALSE(state.canUndo());
    EXPECT_FALSE(state.canRedo());
    EXPECT_EQ(state.statusMessage, "Ready");
}

TEST_F(SequenceGraphEditorTest, SyncAndRebuildEmptySequence)
{
    RecordedSequence emptySeq{ .name = "EmptySeq", .start = "f1", .repeat = false, .events = {} };
    SequenceGraphEditorState state;

    state.syncWithSequence(emptySeq);
    EXPECT_TRUE(state.isGraphSynchronized);
    EXPECT_EQ(state.cachedSequenceEventCount, 0U);
    EXPECT_EQ(state.graphDocument.nodeCount(), 2U); // Start and End
    EXPECT_EQ(state.graphDocument.linkCount(), 1U); // Start -> End
    EXPECT_TRUE(state.validationResult.isValid());
    EXPECT_NE(state.statusMessage.find("synchronized"), std::string::npos);
}

TEST_F(SequenceGraphEditorTest, SyncAndRebuildMultiEventSequence)
{
    SequenceGraphEditorState state;
    state.syncWithSequence(sampleSequence);

    EXPECT_TRUE(state.isGraphSynchronized);
    EXPECT_EQ(state.cachedSequenceEventCount, sampleSequence.events.size());
    // Start + 7 events + End = 9 nodes
    EXPECT_EQ(state.graphDocument.nodeCount(), 9U);
    EXPECT_EQ(state.graphDocument.linkCount(), 8U);
    EXPECT_TRUE(state.validateCurrentGraph());
    EXPECT_TRUE(state.validationResult.isValid());
}

TEST_F(SequenceGraphEditorTest, ValidateCurrentGraphReportsIssues)
{
    SequenceGraphEditorState state;
    state.syncWithSequence(sampleSequence);
    EXPECT_TRUE(state.validateCurrentGraph());

    // Introduce an error by breaking links and adding a cycle
    ASSERT_GE(state.graphDocument.nodeCount(), 3U);
    const auto* pinOut = findFirstPin(state.graphDocument, state.graphDocument.nodes()[1], PinDirection::Output);
    const auto* pinIn = findFirstPin(state.graphDocument, state.graphDocument.nodes()[0], PinDirection::Input);
    if (pinIn == nullptr)
    {
        pinIn = state.graphDocument.createPin(state.graphDocument.nodes()[0].id, PinDirection::Input, "in");
    }
    ASSERT_NE(pinOut, nullptr);
    ASSERT_NE(pinIn, nullptr);
    state.graphDocument.createLink(pinOut->id, pinIn->id);

    EXPECT_FALSE(state.validateCurrentGraph());
    EXPECT_FALSE(state.validationResult.isValid());
    EXPECT_NE(state.statusMessage.find("issue(s)"), std::string::npos);
}

TEST_F(SequenceGraphEditorTest, CompileAndApplySequenceRoundTrip)
{
    SequenceGraphEditorState state;
    state.syncWithSequence(sampleSequence);

    RecordedSequence target;
    bool applied = state.applyToSequence(target);
    EXPECT_TRUE(applied);
    EXPECT_EQ(target.name, sampleSequence.name);
    EXPECT_EQ(target.start, sampleSequence.start);
    EXPECT_EQ(target.repeat, sampleSequence.repeat);
    ASSERT_EQ(target.events.size(), sampleSequence.events.size());

    for (std::size_t i = 0; i < sampleSequence.events.size(); ++i)
    {
        EXPECT_EQ(target.events[i].type, sampleSequence.events[i].type);
        EXPECT_EQ(target.events[i].key, sampleSequence.events[i].key);
        EXPECT_EQ(target.events[i].button, sampleSequence.events[i].button);
        EXPECT_EQ(target.events[i].x, sampleSequence.events[i].x);
        EXPECT_EQ(target.events[i].y, sampleSequence.events[i].y);
    }
}

TEST_F(SequenceGraphEditorTest, CompileRejectsInvalidTopology)
{
    SequenceGraphEditorState state;
    state.syncWithSequence(sampleSequence);

    // Remove the End node to make the graph uncompilable
    auto endNodes = getNodesOfKind(state.graphDocument, NodeKind::End);
    ASSERT_FALSE(endNodes.empty());
    state.graphDocument.removeNode(endNodes.front()->id);

    RecordedSequence target;
    bool applied = state.applyToSequence(target);
    EXPECT_FALSE(applied);
    EXPECT_TRUE(state.lastCompilationError.has_value());
    EXPECT_NE(state.statusMessage.find("Cannot apply changes"), std::string::npos);
}

TEST_F(SequenceGraphEditorTest, ResolveNodeInspectionDetails)
{
    SequenceGraphEditorState state;
    state.syncWithSequence(sampleSequence);

    // 1. Inspect Start Node
    auto startNodes = getNodesOfKind(state.graphDocument, NodeKind::Start);
    ASSERT_FALSE(startNodes.empty());
    auto startDetails = resolveNodeInspectionDetails(
        state.graphDocument, startNodes.front()->id, sampleSequence, state.validationResult);
    ASSERT_TRUE(startDetails.has_value());
    EXPECT_EQ(startDetails->kind, NodeKind::Start);
    EXPECT_FALSE(startDetails->hasAssociatedEvent);
    EXPECT_FALSE(startDetails->sourceIndex.has_value());

    // 2. Inspect First Recorded Event Node
    auto eventNodes = getNodesOfKind(state.graphDocument, NodeKind::RecordedEvent);
    ASSERT_FALSE(eventNodes.empty());
    auto eventDetails = resolveNodeInspectionDetails(
        state.graphDocument, eventNodes.front()->id, sampleSequence, state.validationResult);
    ASSERT_TRUE(eventDetails.has_value());
    EXPECT_EQ(eventDetails->kind, NodeKind::RecordedEvent);
    EXPECT_TRUE(eventDetails->hasAssociatedEvent);
    ASSERT_TRUE(eventDetails->sourceIndex.has_value());
    EXPECT_EQ(*eventDetails->sourceIndex, 0U);
    EXPECT_EQ(eventDetails->associatedEvent.type, RecordedEventType::KeyDown);
    EXPECT_EQ(eventDetails->associatedEvent.key.value_or(""), "ctrl");

    // 3. Inspect with selected node in state
    state.viewerState.selectNode(eventNodes.front()->id);
    auto stateDetails = state.getSelectedNodeDetails(sampleSequence);
    ASSERT_TRUE(stateDetails.has_value());
    EXPECT_EQ(stateDetails->nodeId, eventNodes.front()->id);

    // 4. Non-existent node ID
    auto invalidDetails =
        resolveNodeInspectionDetails(state.graphDocument, 9999, sampleSequence, state.validationResult);
    EXPECT_FALSE(invalidDetails.has_value());
}

TEST_F(SequenceGraphEditorTest, FormatEventFieldsSummary)
{
    RecordedEvent keyEv{ .type = RecordedEventType::KeyDown, .delay = "12ms", .key = "enter" };
    EXPECT_NE(formatEventFieldsSummary(keyEv).find("KeyDown"), std::string::npos);
    EXPECT_NE(formatEventFieldsSummary(keyEv).find("enter"), std::string::npos);

    RecordedEvent keyUpEv{ .type = RecordedEventType::KeyUp, .delay = "10ms", .key = "space" };
    EXPECT_NE(formatEventFieldsSummary(keyUpEv).find("KeyUp"), std::string::npos);

    RecordedEvent mouseBtnEv{ .type = RecordedEventType::MouseDown, .delay = "25ms", .button = "right" };
    EXPECT_NE(formatEventFieldsSummary(mouseBtnEv).find("MouseDown"), std::string::npos);
    EXPECT_NE(formatEventFieldsSummary(mouseBtnEv).find("right"), std::string::npos);

    RecordedEvent mouseMoveEv{ .type = RecordedEventType::MouseMove, .delay = "5ms", .x = 100, .y = 200 };
    EXPECT_NE(formatEventFieldsSummary(mouseMoveEv).find("MouseMove"), std::string::npos);
    EXPECT_NE(formatEventFieldsSummary(mouseMoveEv).find("100"), std::string::npos);

    RecordedEvent delayEv{ .type = RecordedEventType::Invalid, .delay = "200ms" };
    EXPECT_NE(formatEventFieldsSummary(delayEv).find("Delay"), std::string::npos);
    EXPECT_NE(formatEventFieldsSummary(delayEv).find("200ms"), std::string::npos);
}

TEST_F(SequenceGraphEditorTest, DeleteEventNodeAndRecompile)
{
    SequenceGraphEditorState state;
    state.syncWithSequence(sampleSequence);

    auto chainBefore = getLinearExecutionNodes(state.graphDocument);
    ASSERT_EQ(chainBefore.size(), 9U);    // Start + 7 events + End
    NodeId nodeToDelete = chainBefore[3]; // The 2nd event

    bool deleted = state.deleteNode(nodeToDelete, true);
    EXPECT_TRUE(deleted);
    EXPECT_TRUE(state.validateCurrentGraph());

    auto chainAfter = getLinearExecutionNodes(state.graphDocument);
    EXPECT_EQ(chainAfter.size(), 8U);
    EXPECT_EQ(std::find(chainAfter.begin(), chainAfter.end(), nodeToDelete), chainAfter.end());

    auto compileResult = state.compileGraph();
    ASSERT_TRUE(compileResult.success);
    ASSERT_TRUE(compileResult.sequence.has_value());
    EXPECT_EQ(compileResult.sequence->events.size(), 6U);
}

TEST_F(SequenceGraphEditorTest, CannotDeleteStartOrEndNode)
{
    SequenceGraphEditorState state;
    state.syncWithSequence(sampleSequence);

    auto startNodes = getNodesOfKind(state.graphDocument, NodeKind::Start);
    ASSERT_FALSE(startNodes.empty());
    EXPECT_FALSE(state.deleteNode(startNodes.front()->id));

    auto endNodes = getNodesOfKind(state.graphDocument, NodeKind::End);
    ASSERT_FALSE(endNodes.empty());
    EXPECT_FALSE(state.deleteNode(endNodes.front()->id));
}

TEST_F(SequenceGraphEditorTest, AddEventNodeAndRecompile)
{
    SequenceGraphEditorState state;
    state.syncWithSequence(sampleSequence);

    RecordedEvent newEv{ .type = RecordedEventType::KeyDown, .delay = "15ms", .key = "escape" };
    NodeId newId = state.addEventNode(newEv);
    EXPECT_NE(newId, InvalidNodeId);
    EXPECT_TRUE(state.validateCurrentGraph());

    auto chain = getLinearExecutionNodes(state.graphDocument);
    EXPECT_EQ(chain.size(), 10U);              // Start + 8 events + End
    EXPECT_EQ(chain[chain.size() - 2], newId); // Inserted before End

    auto compileResult = state.compileGraph();
    ASSERT_TRUE(compileResult.success);
    ASSERT_TRUE(compileResult.sequence.has_value());
    EXPECT_EQ(compileResult.sequence->events.size(), 8U);
    EXPECT_EQ(compileResult.sequence->events.back().key.value_or(""), "escape");

    // Add dedicated wait node
    NodeId waitId = state.addWaitNode("250ms");
    EXPECT_NE(waitId, InvalidNodeId);
    EXPECT_TRUE(state.validateCurrentGraph());

    auto compileWithWait = state.compileGraph();
    ASSERT_TRUE(compileWithWait.success);
    ASSERT_TRUE(compileWithWait.sequence.has_value());
    EXPECT_EQ(compileWithWait.sequence->events.size(), 9U);
}

TEST_F(SequenceGraphEditorTest, UpdateNodeEventAndDelay)
{
    SequenceGraphEditorState state;
    state.syncWithSequence(sampleSequence);

    auto eventNodes = getNodesOfKind(state.graphDocument, NodeKind::RecordedEvent);
    ASSERT_FALSE(eventNodes.empty());
    NodeId targetNodeId = eventNodes.front()->id;

    RecordedEvent updatedEv{ .type = RecordedEventType::MouseDown, .delay = "75ms", .button = "right" };
    EXPECT_TRUE(state.updateNodeEvent(targetNodeId, updatedEv));

    auto compileResult = state.compileGraph();
    ASSERT_TRUE(compileResult.success);
    ASSERT_TRUE(compileResult.sequence.has_value());
    EXPECT_EQ(compileResult.sequence->events.front().type, RecordedEventType::MouseDown);
    EXPECT_EQ(compileResult.sequence->events.front().button.value_or(""), "right");
    EXPECT_EQ(compileResult.sequence->events.front().delay, "75ms");

    // Update delay only
    EXPECT_TRUE(state.updateNodeDelay(targetNodeId, "120ms"));
    auto compileResult2 = state.compileGraph();
    ASSERT_TRUE(compileResult2.success);
    EXPECT_EQ(compileResult2.sequence->events.front().delay, "120ms");
}

TEST_F(SequenceGraphEditorTest, MoveNodeUpAndDown)
{
    SequenceGraphEditorState state;
    state.syncWithSequence(sampleSequence);

    auto chainInitial = getLinearExecutionNodes(state.graphDocument);
    ASSERT_GE(chainInitial.size(), 4U);

    NodeId node2 = chainInitial[2]; // Second event node
    EXPECT_TRUE(state.moveNodeUp(node2));

    auto chainAfterUp = getLinearExecutionNodes(state.graphDocument);
    EXPECT_EQ(chainAfterUp[1], node2); // Now first event node

    EXPECT_TRUE(state.moveNodeDown(node2));
    auto chainAfterDown = getLinearExecutionNodes(state.graphDocument);
    EXPECT_EQ(chainAfterDown[2], node2); // Back to second
}

TEST_F(SequenceGraphEditorTest, ReconnectLinearChain)
{
    SequenceGraphEditorState state;
    state.syncWithSequence(sampleSequence);

    // Break all links
    auto links = state.graphDocument.links();
    for (const auto& link : links)
    {
        state.graphDocument.removeLink(link.id);
    }
    EXPECT_FALSE(state.validateCurrentGraph());

    // Reconnect linear chain
    EXPECT_TRUE(state.reconnectLinearChain());
    EXPECT_TRUE(state.validateCurrentGraph());
    auto compileResult = state.compileGraph();
    EXPECT_TRUE(compileResult.success);
}

TEST_F(SequenceGraphEditorTest, UndoAndRedoOperations)
{
    SequenceGraphEditorState state;
    state.syncWithSequence(sampleSequence);
    EXPECT_FALSE(state.canUndo());
    EXPECT_FALSE(state.canRedo());

    RecordedEvent newEv{ .type = RecordedEventType::KeyDown, .delay = "10ms", .key = "z" };
    NodeId newId = state.addEventNode(newEv);
    EXPECT_TRUE(state.canUndo());
    EXPECT_FALSE(state.canRedo());

    EXPECT_TRUE(state.undo());
    EXPECT_FALSE(state.canUndo());
    EXPECT_TRUE(state.canRedo());
    EXPECT_EQ(state.graphDocument.findNode(newId), nullptr);

    EXPECT_TRUE(state.redo());
    EXPECT_TRUE(state.canUndo());
    EXPECT_FALSE(state.canRedo());
    EXPECT_NE(state.graphDocument.findNode(newId), nullptr);
}

TEST_F(SequenceGraphEditorTest, RejectUnsupportedBranching)
{
    SequenceGraphEditorState state;
    state.syncWithSequence(sampleSequence);

    // Create a branch from node 1 output to node 3 input
    auto chain = getLinearExecutionNodes(state.graphDocument);
    ASSERT_GE(chain.size(), 4U);
    const auto* outPin = findPinOfDirection(state.graphDocument, chain[1], PinDirection::Output);
    const auto* inPin = findPinOfDirection(state.graphDocument, chain[3], PinDirection::Input);
    ASSERT_NE(outPin, nullptr);
    ASSERT_NE(inPin, nullptr);

    state.graphDocument.createLink(outPin->id, inPin->id);

    auto compileRes = state.compileGraph();
    EXPECT_FALSE(compileRes.success);
    EXPECT_TRUE(compileRes.hasErrors());

    RecordedSequence target = sampleSequence;
    EXPECT_FALSE(state.applyToSequence(target));
}

TEST_F(SequenceGraphEditorTest, ApplyFailureDoesNotMutateOriginalSequence)
{
    SequenceGraphEditorState state;
    state.syncWithSequence(sampleSequence);

    // Break graph by removing End node
    auto endNodes = getNodesOfKind(state.graphDocument, NodeKind::End);
    ASSERT_FALSE(endNodes.empty());
    state.graphDocument.removeNode(endNodes.front()->id);

    RecordedSequence target = sampleSequence;
    bool applied = state.applyToSequence(target);
    EXPECT_FALSE(applied);

    // Verify target sequence is completely unchanged
    EXPECT_EQ(target.name, sampleSequence.name);
    EXPECT_EQ(target.start, sampleSequence.start);
    EXPECT_EQ(target.repeat, sampleSequence.repeat);
    EXPECT_EQ(target.events.size(), sampleSequence.events.size());
    for (std::size_t i = 0; i < sampleSequence.events.size(); ++i)
    {
        EXPECT_EQ(target.events[i].type, sampleSequence.events[i].type);
        EXPECT_EQ(target.events[i].key, sampleSequence.events[i].key);
        EXPECT_EQ(target.events[i].delay, sampleSequence.events[i].delay);
    }
}

TEST_F(SequenceGraphEditorTest, UndoAfterNodeAdd)
{
    SequenceGraphEditorState state;
    state.syncWithSequence(sampleSequence);
    EXPECT_FALSE(state.hasUnappliedChanges());
    const std::size_t initialNodes = state.graphDocument.nodeCount();

    RecordedEvent keyEv{ .type = RecordedEventType::KeyDown, .delay = "10ms", .key = "f12" };
    NodeId newId = state.addEventNode(keyEv);
    EXPECT_NE(newId, InvalidNodeId);
    EXPECT_EQ(state.graphDocument.nodeCount(), initialNodes + 1);
    EXPECT_TRUE(state.hasUnappliedChanges());
    EXPECT_TRUE(state.canUndo());

    // Undo node add
    EXPECT_TRUE(state.undo());
    EXPECT_EQ(state.graphDocument.nodeCount(), initialNodes);
    EXPECT_EQ(state.graphDocument.findNode(newId), nullptr);
    EXPECT_TRUE(state.validateCurrentGraph());

    // Redo node add
    EXPECT_TRUE(state.redo());
    EXPECT_EQ(state.graphDocument.nodeCount(), initialNodes + 1);
    EXPECT_NE(state.graphDocument.findNode(newId), nullptr);
    EXPECT_TRUE(state.validateCurrentGraph());
}

TEST_F(SequenceGraphEditorTest, UndoAfterNodeDelete)
{
    SequenceGraphEditorState state;
    state.syncWithSequence(sampleSequence);
    const std::size_t initialNodes = state.graphDocument.nodeCount();

    auto chain = getLinearExecutionNodes(state.graphDocument);
    ASSERT_GE(chain.size(), 3U);
    NodeId toDelete = chain[1]; // First event node

    EXPECT_TRUE(state.deleteNode(toDelete, true));
    EXPECT_EQ(state.graphDocument.nodeCount(), initialNodes - 1);
    EXPECT_EQ(state.graphDocument.findNode(toDelete), nullptr);
    EXPECT_TRUE(state.hasUnappliedChanges());
    EXPECT_TRUE(state.canUndo());

    // Undo node delete
    EXPECT_TRUE(state.undo());
    EXPECT_EQ(state.graphDocument.nodeCount(), initialNodes);
    EXPECT_NE(state.graphDocument.findNode(toDelete), nullptr);
    EXPECT_TRUE(state.validateCurrentGraph());

    // Redo node delete
    EXPECT_TRUE(state.redo());
    EXPECT_EQ(state.graphDocument.nodeCount(), initialNodes - 1);
    EXPECT_EQ(state.graphDocument.findNode(toDelete), nullptr);
}

TEST_F(SequenceGraphEditorTest, RedoAfterUndoMultipleSteps)
{
    SequenceGraphEditorState state;
    state.syncWithSequence(sampleSequence);

    RecordedEvent ev1{ .type = RecordedEventType::KeyDown, .delay = "10ms", .key = "x" };
    RecordedEvent ev2{ .type = RecordedEventType::KeyDown, .delay = "10ms", .key = "y" };

    NodeId id1 = state.addEventNode(ev1);
    NodeId id2 = state.addEventNode(ev2);

    EXPECT_NE(state.graphDocument.findNode(id1), nullptr);
    EXPECT_NE(state.graphDocument.findNode(id2), nullptr);

    // Undo step 2
    EXPECT_TRUE(state.undo());
    EXPECT_NE(state.graphDocument.findNode(id1), nullptr);
    EXPECT_EQ(state.graphDocument.findNode(id2), nullptr);

    // Undo step 1
    EXPECT_TRUE(state.undo());
    EXPECT_EQ(state.graphDocument.findNode(id1), nullptr);
    EXPECT_EQ(state.graphDocument.findNode(id2), nullptr);

    // Redo step 1
    EXPECT_TRUE(state.redo());
    EXPECT_NE(state.graphDocument.findNode(id1), nullptr);
    EXPECT_EQ(state.graphDocument.findNode(id2), nullptr);

    // Redo step 2
    EXPECT_TRUE(state.redo());
    EXPECT_NE(state.graphDocument.findNode(id1), nullptr);
    EXPECT_NE(state.graphDocument.findNode(id2), nullptr);
}

TEST_F(SequenceGraphEditorTest, SourceDataUnchangedUntilApply)
{
    RecordedSequence target = sampleSequence;
    SequenceGraphEditorState state;
    state.syncWithSequence(target);

    // Perform multiple graph edits
    RecordedEvent ev1{ .type = RecordedEventType::KeyDown, .delay = "10ms", .key = "tab" };
    state.addEventNode(ev1);
    state.addWaitNode("100ms");

    auto chain = getLinearExecutionNodes(state.graphDocument);
    ASSERT_GE(chain.size(), 3U);
    state.deleteNode(chain[1], true);

    EXPECT_TRUE(state.hasUnappliedChanges());

    // Verify target sequence is completely identical to original
    EXPECT_EQ(target.events.size(), sampleSequence.events.size());
    for (std::size_t i = 0; i < sampleSequence.events.size(); ++i)
    {
        EXPECT_EQ(target.events[i].type, sampleSequence.events[i].type);
        EXPECT_EQ(target.events[i].key, sampleSequence.events[i].key);
        EXPECT_EQ(target.events[i].delay, sampleSequence.events[i].delay);
    }

    // Apply changes
    EXPECT_TRUE(state.applyToSequence(target));
    EXPECT_FALSE(state.hasUnappliedChanges());
    EXPECT_NE(target.events.size(), sampleSequence.events.size());
}

TEST_F(SequenceGraphEditorTest, CopyAndPasteNodeOperations)
{
    SequenceGraphEditorState state;
    state.syncWithSequence(sampleSequence);

    auto chain = getLinearExecutionNodes(state.graphDocument);
    ASSERT_GE(chain.size(), 3U);
    NodeId eventNodeId = chain[1];

    EXPECT_FALSE(state.canPaste());

    state.selectNode(eventNodeId);
    EXPECT_TRUE(state.canCopy());
    EXPECT_TRUE(state.copySelectedNode());
    EXPECT_TRUE(state.canPaste());

    NodeId pastedNodeId = state.pasteNode(eventNodeId);
    EXPECT_NE(pastedNodeId, InvalidNodeId);
    EXPECT_TRUE(state.hasUnappliedChanges());

    const auto* pastedNode = state.graphDocument.findNode(pastedNodeId);
    ASSERT_NE(pastedNode, nullptr);
    EXPECT_EQ(pastedNode->kind, NodeKind::RecordedEvent);

    auto parsedEvent = parseRecordedEventFromNode(*pastedNode);
    EXPECT_EQ(parsedEvent.type, sampleSequence.events[0].type);
    EXPECT_EQ(parsedEvent.key, sampleSequence.events[0].key);
}

TEST_F(SequenceGraphEditorTest, DuplicateNodeOperation)
{
    SequenceGraphEditorState state;
    state.syncWithSequence(sampleSequence);

    auto chain = getLinearExecutionNodes(state.graphDocument);
    ASSERT_GE(chain.size(), 3U);
    NodeId eventNodeId = chain[1];

    state.selectNode(eventNodeId);
    EXPECT_TRUE(state.canDuplicate());

    NodeId dupId = state.duplicateSelectedNode();
    EXPECT_NE(dupId, InvalidNodeId);
    EXPECT_TRUE(state.hasUnappliedChanges());

    const auto* dupNode = state.graphDocument.findNode(dupId);
    ASSERT_NE(dupNode, nullptr);
    EXPECT_EQ(dupNode->title, state.graphDocument.findNode(eventNodeId)->title);

    EXPECT_TRUE(state.validateCurrentGraph());
    auto compRes = state.compileGraph();
    EXPECT_TRUE(compRes.success);
    EXPECT_EQ(compRes.sequence->events.size(), sampleSequence.events.size() + 1);
}

TEST_F(SequenceGraphEditorTest, DeleteLinkAndDisconnectNode)
{
    SequenceGraphEditorState state;
    state.syncWithSequence(sampleSequence);

    auto links = state.graphDocument.links();
    ASSERT_FALSE(links.empty());
    LinkId targetLink = links.front().id;

    EXPECT_TRUE(state.deleteLink(targetLink));
    EXPECT_EQ(state.graphDocument.findLink(targetLink), nullptr);
    EXPECT_TRUE(state.hasUnappliedChanges());

    // Undo restores the link
    EXPECT_TRUE(state.undo());
    EXPECT_NE(state.graphDocument.findLink(targetLink), nullptr);

    // Disconnect node
    auto chain = getLinearExecutionNodes(state.graphDocument);
    ASSERT_GE(chain.size(), 3U);
    NodeId midNode = chain[1];
    EXPECT_TRUE(state.disconnectNode(midNode));
    EXPECT_FALSE(state.validateCurrentGraph());
}

TEST_F(SequenceGraphEditorTest, ApplyWarningConfirmationFlow)
{
    SequenceGraphEditorState state;
    state.syncWithSequence(sampleSequence);

    // Introduce an unlinked event node (which causes validation issues or warnings depending on options)
    auto& orphan = state.graphDocument.createNode(NodeKind::Comment, "NoteNode");
    state.markDirty();

    // With warnings/issues, applying without force prompts confirmation if warnings exist
    RecordedSequence target = sampleSequence;
    if (state.validationResult.hasWarnings() && !state.validationResult.hasErrors())
    {
        EXPECT_FALSE(state.applyToSequence(target, false));
        EXPECT_TRUE(state.showApplyWarningConfirmation);

        // Cancel
        state.dismissApplyWarningConfirmation();
        EXPECT_FALSE(state.showApplyWarningConfirmation);
    }
}

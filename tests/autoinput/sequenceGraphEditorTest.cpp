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
    EXPECT_FALSE(state.isEditingAllowed);
    EXPECT_EQ(state.cachedSequenceEventCount, 0U);
    EXPECT_FALSE(state.lastCompilationError.has_value());
    EXPECT_FALSE(state.viewerState.hasSelection());
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
    EXPECT_NE(state.statusMessage.find("Compilation failed"), std::string::npos);
}

TEST_F(SequenceGraphEditorTest, ResolveNodeInspectionDetails)
{
    SequenceGraphEditorState state;
    state.syncWithSequence(sampleSequence);

    // 1. Inspect Start Node
    auto startNodes = getNodesOfKind(state.graphDocument, NodeKind::Start);
    ASSERT_FALSE(startNodes.empty());
    auto startDetails = resolveNodeInspectionDetails(state.graphDocument, startNodes.front()->id, sampleSequence,
                                                     state.validationResult);
    ASSERT_TRUE(startDetails.has_value());
    EXPECT_EQ(startDetails->kind, NodeKind::Start);
    EXPECT_FALSE(startDetails->hasAssociatedEvent);
    EXPECT_FALSE(startDetails->sourceIndex.has_value());

    // 2. Inspect First Recorded Event Node
    auto eventNodes = getNodesOfKind(state.graphDocument, NodeKind::RecordedEvent);
    ASSERT_FALSE(eventNodes.empty());
    auto eventDetails = resolveNodeInspectionDetails(state.graphDocument, eventNodes.front()->id, sampleSequence,
                                                     state.validationResult);
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

    RecordedEvent delayEv{ .type = RecordedEventType::Invalid, .delay = "500ms" };
    EXPECT_NE(formatEventFieldsSummary(delayEv).find("Delay"), std::string::npos);
    EXPECT_NE(formatEventFieldsSummary(delayEv).find("500ms"), std::string::npos);
}

TEST_F(SequenceGraphEditorTest, SeparateWaitNodesToggle)
{
    SequenceGraphEditorState state;
    state.adapterOptions.separateWaitNodes = false;
    state.rebuildFromSequence(sampleSequence);
    std::size_t standardCount = state.graphDocument.nodeCount();

    state.adapterOptions.separateWaitNodes = true;
    state.rebuildFromSequence(sampleSequence);
    std::size_t separatedCount = state.graphDocument.nodeCount();

    // With separate wait nodes enabled on non-zero delays, node count must increase
    EXPECT_GT(separatedCount, standardCount);

    // Validation and compilation back must still succeed
    EXPECT_TRUE(state.validateCurrentGraph());
    RecordedSequence compiled;
    EXPECT_TRUE(state.applyToSequence(compiled));
    EXPECT_EQ(compiled.events.size(), sampleSequence.events.size());
}

TEST_F(SequenceGraphEditorTest, RenderHeadlessLifecycleSafety)
{
    SequenceGraphEditorState state;
    EXPECT_FALSE(renderSequenceGraphEditor(sampleSequence, state, "HeadlessTest"));
}

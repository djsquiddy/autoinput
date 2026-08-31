/**
 * @file recorderGraphAdapterTest.cpp
 * @brief Unit tests for recorder-to-graph integration, partial event handling, and workflow state.
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput_ui/graph/recorderGraphAdapter.h"
#include "autoinput/config/config.h"
#include "autoinput_ui/graph/graphModel.h"
#include "autoinput_ui/graph/graphValidator.h"
#include "autoinput_ui/graph/sequenceGraphAdapter.h"

#include <string>
#include <vector>
#include <gtest/gtest.h>

using namespace autoinput;
using namespace autoinput::ui::graph;
using namespace autoinput::ui::editors;

class RecorderGraphAdapterTest : public ::testing::Test
{
protected:
    RecordedSequence createSampleRecordedSequence()
    {
        RecordedSequence seq;
        seq.name = "RecordedMacro";
        seq.start = "f2";
        seq.repeat = true;
        seq.events = { RecordedEvent{ .type = RecordedEventType::KeyDown, .delay = "15ms", .key = "shift" },
                       RecordedEvent{ .type = RecordedEventType::KeyDown, .delay = "30ms", .key = "a" },
                       RecordedEvent{ .type = RecordedEventType::KeyUp, .delay = "20ms", .key = "a" },
                       RecordedEvent{ .type = RecordedEventType::KeyUp, .delay = "10ms", .key = "shift" },
                       RecordedEvent{ .type = RecordedEventType::MouseMove, .delay = "40ms", .x = 100, .y = 200 },
                       RecordedEvent{ .type = RecordedEventType::MouseDown, .delay = "25ms", .button = "left" },
                       RecordedEvent{ .type = RecordedEventType::MouseUp, .delay = "15ms", .button = "left" } };
        return seq;
    }
};

// =========================================================================
// Graph Generation Tests
// =========================================================================

TEST_F(RecorderGraphAdapterTest, GenerateGraphFromEmptyRecordedSequence)
{
    RecordedSequence emptySeq{ .name = "EmptyRecorderSeq", .start = "f8", .repeat = false, .events = {} };
    const auto result = generateGraphFromRecordedSequence(emptySeq);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.warnings.empty());
    EXPECT_TRUE(result.validationResult.isValid());
    EXPECT_EQ(result.graphDocument.nodeCount(), 2U); // Start, End
    EXPECT_EQ(result.graphDocument.linkCount(), 1U); // Start -> End

    const auto* startNode = result.graphDocument.findNode(1);
    ASSERT_NE(startNode, nullptr);
    EXPECT_EQ(startNode->kind, NodeKind::Start);
    EXPECT_EQ(startNode->details(), "Trigger: f8");

    const auto* endNode = result.graphDocument.findNode(2);
    ASSERT_NE(endNode, nullptr);
    EXPECT_EQ(endNode->kind, NodeKind::End);
    EXPECT_EQ(endNode->details(), "Repeat: Disabled");
}

TEST_F(RecorderGraphAdapterTest, GenerateGraphFromSampleRecordedSequencePreservingTimingAndOrder)
{
    const auto seq = createSampleRecordedSequence();
    const auto result = generateGraphFromRecordedSequence(seq);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.warnings.empty());
    EXPECT_TRUE(result.validationResult.isValid());
    // Start + 7 events + End = 9 nodes
    EXPECT_EQ(result.graphDocument.nodeCount(), 9U);
    EXPECT_EQ(result.graphDocument.linkCount(), 8U);

    // Verify ordering and source indices
    for (std::size_t i = 0; i < seq.events.size(); ++i)
    {
        const auto* node = result.graphDocument.findNode(static_cast<NodeId>(i + 2));
        ASSERT_NE(node, nullptr);
        EXPECT_EQ(node->kind, NodeKind::RecordedEvent);
        ASSERT_TRUE(node->sourceIndex.has_value());
        EXPECT_EQ(*node->sourceIndex, i);
    }
}

TEST_F(RecorderGraphAdapterTest, GenerateGraphWithSeparatedWaitNodes)
{
    const auto seq = createSampleRecordedSequence();
    SequenceGraphOptions opts;
    opts.separateWaitNodes = true;

    const auto result = generateGraphFromRecordedSequence(seq, opts);
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.validationResult.isValid());
    // Start + (7 wait nodes + 7 event nodes) + End = 16 nodes
    EXPECT_EQ(result.graphDocument.nodeCount(), 16U);
    EXPECT_EQ(result.graphDocument.linkCount(), 15U);
}

// =========================================================================
// Partial and Invalid Event Graceful Handling Tests
// =========================================================================

TEST_F(RecorderGraphAdapterTest, HandlesInvalidAndPartialRecordedEventsGracefully)
{
    RecordedSequence partialSeq;
    partialSeq.name = "PartialSeq";
    partialSeq.start = "f2";
    partialSeq.events = { RecordedEvent{ .type = RecordedEventType::Invalid, .delay = "10ms" },
                          RecordedEvent{ .type = RecordedEventType::KeyDown, .delay = "", .key = std::nullopt },
                          RecordedEvent{ .type = RecordedEventType::MouseDown, .delay = "0ms", .button = std::nullopt },
                          RecordedEvent{
                              .type = RecordedEventType::MouseMove, .delay = "5ms", .x = std::nullopt, .y = -50 } };

    const auto result = generateGraphFromRecordedSequence(partialSeq);

    // Must still generate a valid graph topology without crashing
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.validationResult.isValid());
    EXPECT_EQ(result.graphDocument.nodeCount(), 6U); // Start + 4 events + End
    EXPECT_EQ(result.graphDocument.linkCount(), 5U);

    // Warnings should capture each partial or invalid condition
    EXPECT_EQ(result.warnings.size(), 4U);

    // Node 1: Invalid Event
    const auto* ev0 = result.graphDocument.findNode(2);
    ASSERT_NE(ev0, nullptr);
    EXPECT_EQ(ev0->title, "Invalid Event");

    // Node 2: Key Down with empty key
    const auto* ev1 = result.graphDocument.findNode(3);
    ASSERT_NE(ev1, nullptr);
    EXPECT_EQ(ev1->title, "Key Down");
    EXPECT_NE(ev1->details().find("Key: <none>"), std::string::npos);

    // Node 3: Mouse Down with empty button
    const auto* ev2 = result.graphDocument.findNode(4);
    ASSERT_NE(ev2, nullptr);
    EXPECT_EQ(ev2->title, "Mouse Down");
    EXPECT_NE(ev2->details().find("Button: <none>"), std::string::npos);

    // Node 4: Mouse Move with partial coordinates
    const auto* ev3 = result.graphDocument.findNode(5);
    ASSERT_NE(ev3, nullptr);
    EXPECT_EQ(ev3->title, "Mouse Move");
    EXPECT_NE(ev3->details().find("Position: (0, -50)"), std::string::npos);
}

TEST_F(RecorderGraphAdapterTest, SanitizeRecordedSequenceFillsDefaults)
{
    RecordedSequence rawSeq;
    rawSeq.events = { RecordedEvent{ .type = RecordedEventType::KeyDown, .delay = "", .key = "b" },
                      RecordedEvent{
                          .type = RecordedEventType::MouseMove, .delay = "", .x = std::nullopt, .y = std::nullopt } };

    const auto sanitized = sanitizeRecordedSequence(rawSeq);
    EXPECT_EQ(sanitized.events.size(), 2U);
    EXPECT_EQ(sanitized.events[0].delay, "0ms");
    EXPECT_EQ(sanitized.events[1].delay, "0ms");
    ASSERT_TRUE(sanitized.events[1].x.has_value());
    ASSERT_TRUE(sanitized.events[1].y.has_value());
    EXPECT_EQ(*sanitized.events[1].x, 0);
    EXPECT_EQ(*sanitized.events[1].y, 0);
}

// =========================================================================
// RecorderGraphWorkflow Lifecycle and State Tests
// =========================================================================

TEST_F(RecorderGraphAdapterTest, WorkflowLifecycleTransitions)
{
    RecorderGraphWorkflow workflow;
    EXPECT_FALSE(workflow.isRecording());
    EXPECT_FALSE(workflow.isPaused());
    EXPECT_FALSE(workflow.hasRecordedSequence());
    EXPECT_EQ(workflow.eventCount(), 0U);

    // 1. Start recording
    workflow.onRecordingStarted("LiveCapture", "f4", "f5");
    EXPECT_TRUE(workflow.isRecording());
    EXPECT_FALSE(workflow.isPaused());
    EXPECT_TRUE(workflow.hasRecordedSequence());
    EXPECT_EQ(workflow.eventCount(), 0U);
    EXPECT_EQ(workflow.getGraphDocument().nodeCount(), 2U); // Start, End
    EXPECT_TRUE(workflow.validateGraph());

    // 2. Incoming update stream
    RecordedSequence streamSeq;
    streamSeq.name = "LiveCapture";
    streamSeq.start = "f4";
    streamSeq.events.push_back(RecordedEvent{ .type = RecordedEventType::KeyDown, .delay = "10ms", .key = "w" });
    streamSeq.events.push_back(RecordedEvent{ .type = RecordedEventType::KeyUp, .delay = "20ms", .key = "w" });

    workflow.onRecordingUpdated(true, false, 2, streamSeq);
    EXPECT_TRUE(workflow.isRecording());
    EXPECT_EQ(workflow.eventCount(), 2U);
    EXPECT_EQ(workflow.getGraphDocument().nodeCount(), 4U); // Start + 2 events + End
    EXPECT_TRUE(workflow.validateGraph());

    // 3. Stop recording
    streamSeq.events.push_back(
        RecordedEvent{ .type = RecordedEventType::MouseDown, .delay = "15ms", .button = "left" });
    workflow.onRecordingStopped(streamSeq);
    EXPECT_FALSE(workflow.isRecording());
    EXPECT_FALSE(workflow.isPaused());
    EXPECT_EQ(workflow.eventCount(), 3U);
    EXPECT_EQ(workflow.getGraphDocument().nodeCount(), 5U); // Start + 3 events + End
    EXPECT_TRUE(workflow.validateGraph());

    // 4. Discard recording
    workflow.onRecordingDiscarded();
    EXPECT_FALSE(workflow.isRecording());
    EXPECT_FALSE(workflow.hasRecordedSequence());
    EXPECT_EQ(workflow.eventCount(), 0U);
    EXPECT_EQ(workflow.getGraphDocument().nodeCount(), 0U);
}

// =========================================================================
// Graph Edits and Config Export Tests
// =========================================================================

TEST_F(RecorderGraphAdapterTest, ApplyGraphEditsAndSaveToConfig)
{
    RecorderGraphWorkflow workflow;
    auto seq = createSampleRecordedSequence();
    workflow.onRecordingStopped(seq);

    ASSERT_TRUE(workflow.hasRecordedSequence());
    EXPECT_EQ(workflow.getRecordedSequence()->events.size(), 7U);

    // Apply edits via GraphEditorState (e.g. add a wait node)
    workflow.getGraphEditorState().addWaitNode("250ms");
    EXPECT_TRUE(workflow.validateGraph());

    // Compile edits back to sequence
    EXPECT_TRUE(workflow.applyGraphEdits());
    ASSERT_TRUE(workflow.hasRecordedSequence());
    // Original 7 events + 1 newly compiled wait event = 8 events
    EXPECT_EQ(workflow.getRecordedSequence()->events.size(), 8U);

    // Save to ConfigData
    ConfigData config;
    EXPECT_TRUE(workflow.saveToConfig(config, "CustomMacroName"));
    ASSERT_EQ(config.sequences.size(), 1U);
    EXPECT_EQ(config.sequences.front().name, "CustomMacroName");
    EXPECT_EQ(config.sequences.front().events.size(), 8U);

    // Overwrite / merge existing
    EXPECT_TRUE(workflow.saveToConfig(config, "CustomMacroName"));
    EXPECT_EQ(config.sequences.size(), 1U);
}

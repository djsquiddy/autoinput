/**
 * @file sequenceGraphAdapterTest.cpp
 * @brief Unit tests for converting RecordedSequence into GraphDocument models.
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput_ui/graph/sequenceGraphAdapter.h"
#include "autoinput_ui/graph/graphModel.h"
#include "autoinput_ui/graph/graphValidator.h"
#include <gtest/gtest.h>

using namespace autoinput::ui::graph;

class SequenceGraphAdapterTest : public ::testing::Test
{
protected:
    autoinput::RecordedSequence m_sequence;
};

TEST_F(SequenceGraphAdapterTest, ConvertEmptySequence)
{
    m_sequence.name = "Empty Sequence";
    m_sequence.start = "f6";
    m_sequence.repeat = false;
    m_sequence.events.clear();

    const auto doc = sequenceToGraphDocument(m_sequence);

    // Empty sequence should have 1 Start node and 1 End node
    EXPECT_EQ(doc.nodes().size(), 2);
    EXPECT_EQ(doc.links().size(), 1);

    const auto& startNode = doc.nodes()[0];
    EXPECT_EQ(startNode.kind, NodeKind::Start);
    EXPECT_EQ(startNode.title, "Start");
    EXPECT_EQ(startNode.details(), "Trigger: f6");
    EXPECT_EQ(startNode.pinIds.size(), 1);
    const auto* startPin = doc.findPin(startNode.pinIds[0]);
    ASSERT_NE(startPin, nullptr);
    EXPECT_EQ(startPin->direction, PinDirection::Output);

    const auto& endNode = doc.nodes()[1];
    EXPECT_EQ(endNode.kind, NodeKind::End);
    EXPECT_EQ(endNode.title, "End");
    EXPECT_EQ(endNode.details(), "Repeat: Disabled");
    EXPECT_EQ(endNode.pinIds.size(), 1);
    const auto* endPin = doc.findPin(endNode.pinIds[0]);
    ASSERT_NE(endPin, nullptr);
    EXPECT_EQ(endPin->direction, PinDirection::Input);

    // Link connects Start.Out -> End.In
    const auto& link = doc.links()[0];
    EXPECT_EQ(link.fromPinId, startPin->id);
    EXPECT_EQ(link.toPinId, endPin->id);

    // Graph validation check
    const auto validation = validateGraph(doc, ValidationOptions::sequenceGraph());
    EXPECT_TRUE(validation.isValid());
    EXPECT_FALSE(validation.hasErrors());
    EXPECT_FALSE(validation.hasWarnings());
}

TEST_F(SequenceGraphAdapterTest, ConvertSequenceWithKeyboardEvents)
{
    m_sequence.name = "Key Sequence";
    m_sequence.start = "ctrl+shift+k";
    m_sequence.repeat = true;

    autoinput::RecordedEvent e1;
    e1.type = autoinput::RecordedEventType::KeyDown;
    e1.key = "a";
    e1.delay = "0ms";
    m_sequence.events.push_back(e1);

    autoinput::RecordedEvent e2;
    e2.type = autoinput::RecordedEventType::KeyUp;
    e2.key = "a";
    e2.delay = "50ms";
    m_sequence.events.push_back(e2);

    const auto doc = sequenceToGraphDocument(m_sequence);

    // Nodes: Start -> KeyDown -> KeyUp -> End
    ASSERT_EQ(doc.nodes().size(), 4);
    ASSERT_EQ(doc.links().size(), 3);

    // Validate Start Node
    EXPECT_EQ(doc.nodes()[0].kind, NodeKind::Start);
    EXPECT_EQ(doc.nodes()[0].details(), "Trigger: ctrl+shift+k");

    // Validate KeyDown Node
    const auto& keyNode1 = doc.nodes()[1];
    EXPECT_EQ(keyNode1.kind, NodeKind::RecordedEvent);
    EXPECT_EQ(keyNode1.title, "Key Down");
    EXPECT_EQ(keyNode1.details(), "Key: a");
    ASSERT_TRUE(keyNode1.sourceIndex.has_value());
    EXPECT_EQ(*keyNode1.sourceIndex, 0);

    // Validate KeyUp Node (with delay retained in subtitle)
    const auto& keyNode2 = doc.nodes()[2];
    EXPECT_EQ(keyNode2.kind, NodeKind::RecordedEvent);
    EXPECT_EQ(keyNode2.title, "Key Up");
    EXPECT_EQ(keyNode2.details(), "Key: a (delay: 50ms)");
    ASSERT_TRUE(keyNode2.sourceIndex.has_value());
    EXPECT_EQ(*keyNode2.sourceIndex, 1);

    // Validate End Node
    EXPECT_EQ(doc.nodes()[3].kind, NodeKind::End);
    EXPECT_EQ(doc.nodes()[3].details(), "Repeat: Enabled");

    // Graph validation
    const auto validation = validateGraph(doc, ValidationOptions::sequenceGraph());
    EXPECT_TRUE(validation.isValid());
}

TEST_F(SequenceGraphAdapterTest, ConvertSequenceWithMouseEvents)
{
    m_sequence.name = "Mouse Sequence";
    m_sequence.start = "f7";

    autoinput::RecordedEvent e1;
    e1.type = autoinput::RecordedEventType::MouseMove;
    e1.x = 250;
    e1.y = 400;
    e1.delay = "0ms";
    m_sequence.events.push_back(e1);

    autoinput::RecordedEvent e2;
    e2.type = autoinput::RecordedEventType::MouseDown;
    e2.button = "left";
    e2.delay = "100ms";
    m_sequence.events.push_back(e2);

    autoinput::RecordedEvent e3;
    e3.type = autoinput::RecordedEventType::MouseUp;
    e3.button = "left";
    e3.delay = "20ms";
    m_sequence.events.push_back(e3);

    const auto doc = convertSequenceToGraph(m_sequence);

    // Nodes: Start -> MouseMove -> MouseDown -> MouseUp -> End
    ASSERT_EQ(doc.nodes().size(), 5);
    ASSERT_EQ(doc.links().size(), 4);

    EXPECT_EQ(doc.nodes()[1].kind, NodeKind::RecordedEvent);
    EXPECT_EQ(doc.nodes()[1].title, "Mouse Move");
    EXPECT_EQ(doc.nodes()[1].details(), "Position: (250, 400)");
    EXPECT_EQ(doc.nodes()[1].sourceIndex, 0);

    EXPECT_EQ(doc.nodes()[2].kind, NodeKind::RecordedEvent);
    EXPECT_EQ(doc.nodes()[2].title, "Mouse Down");
    EXPECT_EQ(doc.nodes()[2].details(), "Button: left (delay: 100ms)");
    EXPECT_EQ(doc.nodes()[2].sourceIndex, 1);

    EXPECT_EQ(doc.nodes()[3].kind, NodeKind::RecordedEvent);
    EXPECT_EQ(doc.nodes()[3].title, "Mouse Up");
    EXPECT_EQ(doc.nodes()[3].details(), "Button: left (delay: 20ms)");
    EXPECT_EQ(doc.nodes()[3].sourceIndex, 2);

    const auto validation = validateGraph(doc, ValidationOptions::sequenceGraph());
    EXPECT_TRUE(validation.isValid());
}

TEST_F(SequenceGraphAdapterTest, SourceIndexPreservation)
{
    m_sequence.name = "Indexing Test";

    for (int i = 0; i < 5; ++i)
    {
        autoinput::RecordedEvent ev;
        ev.type = autoinput::RecordedEventType::KeyDown;
        ev.key = std::string(1, static_cast<char>('1' + i));
        ev.delay = "0ms";
        m_sequence.events.push_back(ev);
    }

    const auto doc = sequenceToGraphDocument(m_sequence);
    ASSERT_EQ(doc.nodes().size(), 7); // Start + 5 events + End

    for (std::size_t i = 0; i < 5; ++i)
    {
        const auto& node = doc.nodes()[i + 1];
        ASSERT_TRUE(node.sourceIndex.has_value());
        EXPECT_EQ(*node.sourceIndex, i);
        EXPECT_EQ(node.details(), "Key: " + std::string(1, static_cast<char>('1' + i)));
    }
}

TEST_F(SequenceGraphAdapterTest, SeparateWaitNodesOption)
{
    m_sequence.name = "Timing Sequence";

    autoinput::RecordedEvent e1;
    e1.type = autoinput::RecordedEventType::KeyDown;
    e1.key = "space";
    e1.delay = "0ms"; // zero delay -> no separate wait node
    m_sequence.events.push_back(e1);

    autoinput::RecordedEvent e2;
    e2.type = autoinput::RecordedEventType::KeyUp;
    e2.key = "space";
    e2.delay = "250ms"; // non-zero delay -> separate wait node
    m_sequence.events.push_back(e2);

    SequenceGraphOptions options;
    options.separateWaitNodes = true;

    const auto doc = sequenceToGraphDocument(m_sequence, options);

    // Nodes: Start -> Event0 (KeyDown) -> Wait (250ms) -> Event1 (KeyUp) -> End
    ASSERT_EQ(doc.nodes().size(), 5);
    ASSERT_EQ(doc.links().size(), 4);

    EXPECT_EQ(doc.nodes()[0].kind, NodeKind::Start);

    EXPECT_EQ(doc.nodes()[1].kind, NodeKind::RecordedEvent);
    EXPECT_EQ(doc.nodes()[1].title, "Key Down");
    EXPECT_EQ(doc.nodes()[1].details(), "Key: space");
    EXPECT_EQ(doc.nodes()[1].sourceIndex, 0);

    EXPECT_EQ(doc.nodes()[2].kind, NodeKind::Wait);
    EXPECT_EQ(doc.nodes()[2].title, "Wait");
    EXPECT_EQ(doc.nodes()[2].details(), "Delay: 250ms");
    EXPECT_EQ(doc.nodes()[2].sourceIndex, 1);

    EXPECT_EQ(doc.nodes()[3].kind, NodeKind::RecordedEvent);
    EXPECT_EQ(doc.nodes()[3].title, "Key Up");
    EXPECT_EQ(doc.nodes()[3].details(), "Key: space");
    EXPECT_EQ(doc.nodes()[3].sourceIndex, 1);

    EXPECT_EQ(doc.nodes()[4].kind, NodeKind::End);

    const auto validation = validateGraph(doc, ValidationOptions::sequenceGraph());
    EXPECT_TRUE(validation.isValid());
}

TEST_F(SequenceGraphAdapterTest, DeterministicPositions)
{
    m_sequence.name = "Position Test";

    autoinput::RecordedEvent e1;
    e1.type = autoinput::RecordedEventType::KeyDown;
    e1.key = "x";
    m_sequence.events.push_back(e1);

    SequenceGraphOptions options;
    options.startX = 10.0F;
    options.startY = 20.0F;
    options.stepX = 150.0F;
    options.stepY = 30.0F;

    const auto doc = sequenceToGraphDocument(m_sequence, options);
    ASSERT_EQ(doc.nodes().size(), 3); // Start, Event, End

    // Node 0 (Start)
    EXPECT_FLOAT_EQ(doc.nodes()[0].position.x, 10.0F);
    EXPECT_FLOAT_EQ(doc.nodes()[0].position.y, 20.0F);

    // Node 1 (Event)
    EXPECT_FLOAT_EQ(doc.nodes()[1].position.x, 160.0F);
    EXPECT_FLOAT_EQ(doc.nodes()[1].position.y, 50.0F);

    // Node 2 (End)
    EXPECT_FLOAT_EQ(doc.nodes()[2].position.x, 310.0F);
    EXPECT_FLOAT_EQ(doc.nodes()[2].position.y, 80.0F);
}

TEST_F(SequenceGraphAdapterTest, FormattingHelpersAndInvalidEvent)
{
    EXPECT_FALSE(isNonZeroDelay(""));
    EXPECT_FALSE(isNonZeroDelay("0ms"));
    EXPECT_FALSE(isNonZeroDelay("0s"));
    EXPECT_FALSE(isNonZeroDelay("0"));
    EXPECT_TRUE(isNonZeroDelay("10ms"));
    EXPECT_TRUE(isNonZeroDelay("1.5s"));

    autoinput::RecordedEvent invalidEvent;
    invalidEvent.type = autoinput::RecordedEventType::Invalid;
    invalidEvent.delay = "100ms";

    EXPECT_EQ(formatRecordedEventTitle(invalidEvent), "Invalid Event");
    EXPECT_EQ(formatRecordedEventSubtitle(invalidEvent, true), "Unknown event (delay: 100ms)");
    EXPECT_EQ(formatRecordedEventSubtitle(invalidEvent, false), "Unknown event");

    // Test missing key / button
    autoinput::RecordedEvent emptyKey;
    emptyKey.type = autoinput::RecordedEventType::KeyDown;
    EXPECT_EQ(formatRecordedEventSubtitle(emptyKey), "Key: <none>");

    autoinput::RecordedEvent emptyButton;
    emptyButton.type = autoinput::RecordedEventType::MouseDown;
    EXPECT_EQ(formatRecordedEventSubtitle(emptyButton), "Button: <none>");
}

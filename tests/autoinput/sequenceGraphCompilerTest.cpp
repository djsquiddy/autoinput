/**
 * @file sequenceGraphCompilerTest.cpp
 * @brief Unit tests for sequence graph compiler (GraphDocument to RecordedSequence).
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput_ui/graph/sequenceGraphCompiler.h"
#include "autoinput_ui/graph/graphModel.h"
#include "autoinput_ui/graph/sequenceGraphAdapter.h"
#include <gtest/gtest.h>

using namespace autoinput::ui::graph;

class SequenceGraphCompilerTest : public ::testing::Test
{
protected:
    autoinput::RecordedSequence m_sequence;
};

TEST_F(SequenceGraphCompilerTest, CompileValidLinearGraph)
{
    GraphDocument doc;
    auto& start = doc.createNode(NodeKind::Start, "Start");
    start.setDetails("Trigger: f6");
    auto* startOut = doc.createPin(start.id, PinDirection::Output, "Out");

    auto& ev1 = doc.createNode(NodeKind::RecordedEvent, "Key Down");
    ev1.setDetails("Key: a");
    auto* ev1In = doc.createPin(ev1.id, PinDirection::Input, "In");
    auto* ev1Out = doc.createPin(ev1.id, PinDirection::Output, "Out");

    auto& ev2 = doc.createNode(NodeKind::RecordedEvent, "Key Up");
    ev2.setDetails("Key: a (delay: 75ms)");
    auto* ev2In = doc.createPin(ev2.id, PinDirection::Input, "In");
    auto* ev2Out = doc.createPin(ev2.id, PinDirection::Output, "Out");

    auto& end = doc.createNode(NodeKind::End, "End");
    end.setDetails("Repeat: Enabled");
    auto* endIn = doc.createPin(end.id, PinDirection::Input, "In");

    ASSERT_NE(startOut, nullptr);
    ASSERT_NE(ev1In, nullptr);
    ASSERT_NE(ev1Out, nullptr);
    ASSERT_NE(ev2In, nullptr);
    ASSERT_NE(ev2Out, nullptr);
    ASSERT_NE(endIn, nullptr);

    doc.createLink(startOut->id, ev1In->id);
    doc.createLink(ev1Out->id, ev2In->id);
    doc.createLink(ev2Out->id, endIn->id);

    const auto result = compileGraphToSequence(doc);
    EXPECT_TRUE(result.isSuccess());
    EXPECT_TRUE(static_cast<bool>(result));
    EXPECT_FALSE(result.hasErrors());
    EXPECT_EQ(result.errorCount(), 0);
    ASSERT_TRUE(result.sequence.has_value());

    const auto& compiled = *result.sequence;
    EXPECT_EQ(compiled.start, "f6");
    EXPECT_TRUE(compiled.repeat);
    ASSERT_EQ(compiled.events.size(), 2);

    EXPECT_EQ(compiled.events[0].type, autoinput::RecordedEventType::KeyDown);
    EXPECT_EQ(compiled.events[0].key.value_or(""), "a");
    EXPECT_EQ(compiled.events[0].delay, "0ms");

    EXPECT_EQ(compiled.events[1].type, autoinput::RecordedEventType::KeyUp);
    EXPECT_EQ(compiled.events[1].key.value_or(""), "a");
    EXPECT_EQ(compiled.events[1].delay, "75ms");
}

TEST_F(SequenceGraphCompilerTest, RoundTripEmptySequence)
{
    m_sequence.name = "Empty Sequence";
    m_sequence.start = "f8";
    m_sequence.repeat = false;
    m_sequence.events.clear();

    const auto doc = sequenceToGraphDocument(m_sequence);
    const auto result = compileGraphToSequence(doc, m_sequence);

    ASSERT_TRUE(result.isSuccess());
    ASSERT_TRUE(result.sequence.has_value());

    const auto& compiled = *result.sequence;
    EXPECT_EQ(compiled.name, m_sequence.name);
    EXPECT_EQ(compiled.start, m_sequence.start);
    EXPECT_EQ(compiled.repeat, m_sequence.repeat);
    EXPECT_TRUE(compiled.events.empty());
}

TEST_F(SequenceGraphCompilerTest, RoundTripSequenceWithKeyboardAndMouseEvents)
{
    m_sequence.name = "Complex Sequence";
    m_sequence.start = "ctrl+alt+s";
    m_sequence.repeat = true;

    autoinput::RecordedEvent e1;
    e1.type = autoinput::RecordedEventType::KeyDown;
    e1.key = "w";
    e1.delay = "0ms";
    m_sequence.events.push_back(e1);

    autoinput::RecordedEvent e2;
    e2.type = autoinput::RecordedEventType::MouseMove;
    e2.x = 640;
    e2.y = 480;
    e2.delay = "50ms";
    m_sequence.events.push_back(e2);

    autoinput::RecordedEvent e3;
    e3.type = autoinput::RecordedEventType::MouseDown;
    e3.button = "left";
    e3.delay = "100ms";
    m_sequence.events.push_back(e3);

    autoinput::RecordedEvent e4;
    e4.type = autoinput::RecordedEventType::MouseUp;
    e4.button = "left";
    e4.delay = "20ms";
    m_sequence.events.push_back(e4);

    autoinput::RecordedEvent e5;
    e5.type = autoinput::RecordedEventType::KeyUp;
    e5.key = "w";
    e5.delay = "10ms";
    m_sequence.events.push_back(e5);

    // 1. Round-trip without separated wait nodes
    {
        const auto doc = sequenceToGraphDocument(m_sequence);
        const auto result = compileGraphToSequence(doc, m_sequence);

        ASSERT_TRUE(result.isSuccess());
        ASSERT_TRUE(result.sequence.has_value());

        const auto& compiled = *result.sequence;
        EXPECT_EQ(compiled.name, m_sequence.name);
        EXPECT_EQ(compiled.start, m_sequence.start);
        EXPECT_EQ(compiled.repeat, m_sequence.repeat);
        ASSERT_EQ(compiled.events.size(), m_sequence.events.size());

        for (std::size_t i = 0; i < m_sequence.events.size(); ++i)
        {
            EXPECT_EQ(compiled.events[i].type, m_sequence.events[i].type);
            EXPECT_EQ(compiled.events[i].key, m_sequence.events[i].key);
            EXPECT_EQ(compiled.events[i].button, m_sequence.events[i].button);
            EXPECT_EQ(compiled.events[i].x, m_sequence.events[i].x);
            EXPECT_EQ(compiled.events[i].y, m_sequence.events[i].y);
            EXPECT_EQ(compiled.events[i].delay, m_sequence.events[i].delay);
        }
    }

    // 2. Round-trip with separated wait nodes
    {
        SequenceGraphOptions opt;
        opt.separateWaitNodes = true;
        const auto doc = sequenceToGraphDocument(m_sequence, opt);
        const auto result = compileGraphToSequence(doc, m_sequence);

        ASSERT_TRUE(result.isSuccess());
        ASSERT_TRUE(result.sequence.has_value());

        const auto& compiled = *result.sequence;
        EXPECT_EQ(compiled.name, m_sequence.name);
        EXPECT_EQ(compiled.start, m_sequence.start);
        EXPECT_EQ(compiled.repeat, m_sequence.repeat);
        ASSERT_EQ(compiled.events.size(), m_sequence.events.size());

        for (std::size_t i = 0; i < m_sequence.events.size(); ++i)
        {
            EXPECT_EQ(compiled.events[i].type, m_sequence.events[i].type);
            EXPECT_EQ(compiled.events[i].key, m_sequence.events[i].key);
            EXPECT_EQ(compiled.events[i].button, m_sequence.events[i].button);
            EXPECT_EQ(compiled.events[i].x, m_sequence.events[i].x);
            EXPECT_EQ(compiled.events[i].y, m_sequence.events[i].y);
            EXPECT_EQ(compiled.events[i].delay, m_sequence.events[i].delay);
        }
    }
}

TEST_F(SequenceGraphCompilerTest, ExecutionOrderDeterminedByLinksNotNodeOrder)
{
    GraphDocument doc;
    // Create nodes out of execution order
    auto& start = doc.createNode(NodeKind::Start, "Start");
    auto& evSecond = doc.createNode(NodeKind::RecordedEvent, "Key Up");
    evSecond.setDetails("Key: b");
    auto& evFirst = doc.createNode(NodeKind::RecordedEvent, "Key Down");
    evFirst.setDetails("Key: a");
    auto& end = doc.createNode(NodeKind::End, "End");

    auto* startOut = doc.createPin(start.id, PinDirection::Output, "Out");
    auto* evFirstIn = doc.createPin(evFirst.id, PinDirection::Input, "In");
    auto* evFirstOut = doc.createPin(evFirst.id, PinDirection::Output, "Out");
    auto* evSecondIn = doc.createPin(evSecond.id, PinDirection::Input, "In");
    auto* evSecondOut = doc.createPin(evSecond.id, PinDirection::Output, "Out");
    auto* endIn = doc.createPin(end.id, PinDirection::Input, "In");

    // Link in topological order: Start -> evFirst -> evSecond -> End
    doc.createLink(startOut->id, evFirstIn->id);
    doc.createLink(evFirstOut->id, evSecondIn->id);
    doc.createLink(evSecondOut->id, endIn->id);

    const auto result = compileGraphToSequence(doc);
    ASSERT_TRUE(result.isSuccess());
    ASSERT_TRUE(result.sequence.has_value());
    ASSERT_EQ(result.sequence->events.size(), 2);

    EXPECT_EQ(result.sequence->events[0].type, autoinput::RecordedEventType::KeyDown);
    EXPECT_EQ(result.sequence->events[0].key.value_or(""), "a");

    EXPECT_EQ(result.sequence->events[1].type, autoinput::RecordedEventType::KeyUp);
    EXPECT_EQ(result.sequence->events[1].key.value_or(""), "b");
}

TEST_F(SequenceGraphCompilerTest, RejectMissingStartNode)
{
    GraphDocument doc;
    auto& ev = doc.createNode(NodeKind::RecordedEvent, "Key Down");
    auto& end = doc.createNode(NodeKind::End, "End");

    auto* evOut = doc.createPin(ev.id, PinDirection::Output, "Out");
    auto* endIn = doc.createPin(end.id, PinDirection::Input, "In");
    doc.createLink(evOut->id, endIn->id);

    const auto result = compileGraphToSequence(doc);
    EXPECT_FALSE(result.isSuccess());
    EXPECT_FALSE(static_cast<bool>(result));
    EXPECT_TRUE(result.hasErrors());
    EXPECT_EQ(result.sequence, std::nullopt);
}

TEST_F(SequenceGraphCompilerTest, RejectMultipleStartNodes)
{
    GraphDocument doc;
    auto& start1 = doc.createNode(NodeKind::Start, "Start 1");
    auto& start2 = doc.createNode(NodeKind::Start, "Start 2");
    auto& end = doc.createNode(NodeKind::End, "End");

    auto* start1Out = doc.createPin(start1.id, PinDirection::Output, "Out");
    auto* start2Out = doc.createPin(start2.id, PinDirection::Output, "Out");
    auto* endIn = doc.createPin(end.id, PinDirection::Input, "In");

    doc.createLink(start1Out->id, endIn->id);
    (void)start2Out;

    const auto result = compileGraphToSequence(doc);
    EXPECT_FALSE(result.isSuccess());
    EXPECT_TRUE(result.hasErrors());
}

TEST_F(SequenceGraphCompilerTest, RejectMissingEndNode)
{
    GraphDocument doc;
    auto& start = doc.createNode(NodeKind::Start, "Start");
    auto& ev = doc.createNode(NodeKind::RecordedEvent, "Key Down");

    auto* startOut = doc.createPin(start.id, PinDirection::Output, "Out");
    auto* evIn = doc.createPin(ev.id, PinDirection::Input, "In");
    doc.createLink(startOut->id, evIn->id);

    const auto result = compileGraphToSequence(doc);
    EXPECT_FALSE(result.isSuccess());
    EXPECT_TRUE(result.hasErrors());
}

TEST_F(SequenceGraphCompilerTest, RejectUnreachableEndNode)
{
    GraphDocument doc;
    auto& start = doc.createNode(NodeKind::Start, "Start");
    auto& ev = doc.createNode(NodeKind::RecordedEvent, "Key Down");
    auto& end = doc.createNode(NodeKind::End, "End");

    auto* startOut = doc.createPin(start.id, PinDirection::Output, "Out");
    auto* evIn = doc.createPin(ev.id, PinDirection::Input, "In");
    auto* endIn = doc.createPin(end.id, PinDirection::Input, "In");

    // Start -> ev, but end is not connected
    doc.createLink(startOut->id, evIn->id);
    (void)endIn;

    const auto result = compileGraphToSequence(doc);
    EXPECT_FALSE(result.isSuccess());
    EXPECT_TRUE(result.hasErrors());
}

TEST_F(SequenceGraphCompilerTest, RejectUnsupportedBranching)
{
    GraphDocument doc;
    auto& start = doc.createNode(NodeKind::Start, "Start");
    auto& ev1 = doc.createNode(NodeKind::RecordedEvent, "Key Down 1");
    auto& ev2 = doc.createNode(NodeKind::RecordedEvent, "Key Down 2");
    auto& end = doc.createNode(NodeKind::End, "End");

    auto* startOut = doc.createPin(start.id, PinDirection::Output, "Out");
    auto* ev1In = doc.createPin(ev1.id, PinDirection::Input, "In");
    auto* ev1Out = doc.createPin(ev1.id, PinDirection::Output, "Out");
    auto* ev2In = doc.createPin(ev2.id, PinDirection::Input, "In");
    auto* ev2Out = doc.createPin(ev2.id, PinDirection::Output, "Out");
    auto* endIn = doc.createPin(end.id, PinDirection::Input, "In");

    // Start forks into ev1 and ev2
    doc.createLink(startOut->id, ev1In->id);
    doc.createLink(startOut->id, ev2In->id);
    doc.createLink(ev1Out->id, endIn->id);
    doc.createLink(ev2Out->id, endIn->id);

    const auto result = compileGraphToSequence(doc);
    EXPECT_FALSE(result.isSuccess());
    EXPECT_TRUE(result.hasErrors());
}

TEST_F(SequenceGraphCompilerTest, RejectCycle)
{
    GraphDocument doc;
    auto& start = doc.createNode(NodeKind::Start, "Start");
    auto& ev1 = doc.createNode(NodeKind::RecordedEvent, "Event 1");
    auto& ev2 = doc.createNode(NodeKind::RecordedEvent, "Event 2");
    auto& end = doc.createNode(NodeKind::End, "End");

    auto* startOut = doc.createPin(start.id, PinDirection::Output, "Out");
    auto* ev1In = doc.createPin(ev1.id, PinDirection::Input, "In");
    auto* ev1Out = doc.createPin(ev1.id, PinDirection::Output, "Out");
    auto* ev2In = doc.createPin(ev2.id, PinDirection::Input, "In");
    auto* ev2Out = doc.createPin(ev2.id, PinDirection::Output, "Out");
    auto* endIn = doc.createPin(end.id, PinDirection::Input, "In");

    doc.createLink(startOut->id, ev1In->id);
    doc.createLink(ev1Out->id, ev2In->id);
    // Cycle: ev2 -> ev1
    doc.createLink(ev2Out->id, ev1In->id);
    doc.createLink(ev2Out->id, endIn->id);

    const auto result = compileGraphToSequence(doc);
    EXPECT_FALSE(result.isSuccess());
    EXPECT_TRUE(result.hasErrors());
}

TEST_F(SequenceGraphCompilerTest, RejectInvalidLinkDirection)
{
    GraphDocument doc;
    auto& start = doc.createNode(NodeKind::Start, "Start");
    auto& end = doc.createNode(NodeKind::End, "End");

    auto* startIn = doc.createPin(start.id, PinDirection::Input, "In");
    auto* endIn = doc.createPin(end.id, PinDirection::Input, "In");

    // Invalid input-to-input link
    doc.createLink(startIn->id, endIn->id);

    const auto result = compileGraphToSequence(doc);
    EXPECT_FALSE(result.isSuccess());
    EXPECT_TRUE(result.hasErrors());
}

TEST_F(SequenceGraphCompilerTest, RejectDisconnectedExecutionNode)
{
    GraphDocument doc;
    auto& start = doc.createNode(NodeKind::Start, "Start");
    auto& evConnected = doc.createNode(NodeKind::RecordedEvent, "Key Down");
    evConnected.setDetails("Key: a");
    auto& evDisconnected = doc.createNode(NodeKind::RecordedEvent, "Floating Event");
    evDisconnected.setDetails("Key: b");
    auto& end = doc.createNode(NodeKind::End, "End");

    auto* startOut = doc.createPin(start.id, PinDirection::Output, "Out");
    auto* evConnIn = doc.createPin(evConnected.id, PinDirection::Input, "In");
    auto* evConnOut = doc.createPin(evConnected.id, PinDirection::Output, "Out");
    auto* endIn = doc.createPin(end.id, PinDirection::Input, "In");

    doc.createLink(startOut->id, evConnIn->id);
    doc.createLink(evConnOut->id, endIn->id);

    const auto result = compileGraphToSequence(doc);
    EXPECT_FALSE(result.isSuccess());
    EXPECT_TRUE(result.hasErrors());
}

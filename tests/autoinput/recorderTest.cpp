/**
 * @file recorderTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include "autoinput/input/sequenceRecorder.h"
#include "autoinput/support/types.h"
#include <filesystem>
#include <fstream>
#include <thread>

namespace autoinput
{
    class RecorderTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            testConfigPath = std::filesystem::temp_directory_path() / "test_macro.toml";
            if (std::filesystem::exists(testConfigPath))
            {
                std::filesystem::remove(testConfigPath);
            }
        }

        void TearDown() override
        {
            if (std::filesystem::exists(testConfigPath))
            {
                std::filesystem::remove(testConfigPath);
            }
        }

        std::filesystem::path testConfigPath;
    };

    TEST_F(RecorderTest, StateMachine)
    {
        SequenceRecorder recorder(SequenceConfig{ .recordMouseMoves = false, .name = "test-macro", .startKey = "f8", .endKey = "f9", .playStartKey = "f6", .mouseSampleDelay = "25ms" });
        
        // Verify the recorder initializes in the Waiting state
        EXPECT_EQ(recorder.getState(), RecorderState::Waiting);

        // Pressing non-start key does nothing
        recorder.onKeyEvent(Key::fromString("a"), true);
        // Verify pressing a non-start key does not change state from Waiting
        EXPECT_EQ(recorder.getState(), RecorderState::Waiting);
        // Verify no events are recorded before recording starts
        EXPECT_TRUE(recorder.getSequence().events.empty());

        // Pressing start key
        recorder.onKeyEvent(Key::fromString("f8"), true);
        // Verify pressing the start key transitions the state to Recording
        EXPECT_EQ(recorder.getState(), RecorderState::Recording);

        // Record some events
        recorder.onKeyEvent(Key::fromString("a"), true);
        recorder.onKeyEvent(Key::fromString("a"), false);
        
        // Verify two events (key down and key up) were recorded
        EXPECT_EQ(recorder.getSequence().events.size(), 2);
        // Verify the first recorded event is a KeyDown event
        EXPECT_EQ(recorder.getSequence().events[0].type, RecordedEventType::KeyDown);
        // Verify the recorded key for the first event is 'a'
        EXPECT_EQ(recorder.getSequence().events[0].key, "a");
        // Verify the second recorded event is a KeyUp event
        EXPECT_EQ(recorder.getSequence().events[1].type, RecordedEventType::KeyUp);
        // Verify the recorded key for the second event is 'a'
        EXPECT_EQ(recorder.getSequence().events[1].key, "a");

        // Pressing end key
        recorder.onKeyEvent(Key::fromString("f9"), true);
        // Verify pressing the end key transitions the state to Finished
        EXPECT_EQ(recorder.getState(), RecorderState::Finished);

        // Events after end are not recorded
        recorder.onKeyEvent(Key::fromString("b"), true);
        // Verify events occurring after the end key is pressed are not added
        EXPECT_EQ(recorder.getSequence().events.size(), 2);
    }

    TEST_F(RecorderTest, ExcludeControlKeys)
    {
        SequenceRecorder recorder(SequenceConfig{ .recordMouseMoves = false, .name = "test-macro", .startKey = "f8", .endKey = "f9", .playStartKey = "f6", .mouseSampleDelay = "25ms" });
        
        recorder.onKeyEvent(Key::fromString("f8"), true); // Start
        recorder.onKeyEvent(Key::fromString("a"), true);
        recorder.onKeyEvent(Key::fromString("f9"), true); // End
        
        const auto& events = recorder.getSequence().events;
        // Ensure only the payload key event was recorded
        ASSERT_EQ(events.size(), 1);
        // Verify the recorded payload key is 'a'
        EXPECT_EQ(events[0].key, "a");
        
        for (const auto& event : events)
        {
            // Verify the start key 'f8' was excluded from recorded events
            EXPECT_NE(event.key, "f8");
            // Verify the end key 'f9' was excluded from recorded events
            EXPECT_NE(event.key, "f9");
        }
    }

    TEST_F(RecorderTest, RecordMouseEvents)
    {
        SequenceRecorder recorder(SequenceConfig{ .recordMouseMoves = true, .name = "test-macro", .startKey = "f8", .endKey = "f9", .playStartKey = "f6", .mouseSampleDelay = "25ms" });
        
        recorder.onKeyEvent(Key::fromString("f8"), true); // Start
        
        recorder.onMouseEvent(Mouse(MouseButton::Left), true, 100, 200);
        recorder.onMouseEvent(Mouse(MouseButton::Left), false, 100, 200);
        
        // Move mouse
        recorder.onMouseMove(110, 210);
        
        recorder.onKeyEvent(Key::fromString("f9"), true); // End
        
        const auto& events = recorder.getSequence().events;
        // Ensure at least three mouse events (down, up, and move) were recorded
        ASSERT_GE(events.size(), 3);
        // Verify the first event is a MouseDown event
        EXPECT_EQ(events[0].type, RecordedEventType::MouseDown);
        // Verify the mouse button recorded is 'left'
        EXPECT_EQ(events[0].button, "left");
        // Verify the mouse X coordinate is 100
        EXPECT_EQ(events[0].x, 100);
        // Verify the mouse Y coordinate is 200
        EXPECT_EQ(events[0].y, 200);
        
        // Verify the second event is a MouseUp event
        EXPECT_EQ(events[1].type, RecordedEventType::MouseUp);
        
        bool foundMove = false;
        for (const auto& event : events)
        {
            if (event.type == RecordedEventType::MouseMove)
            {
                foundMove = true;
                // Verify the recorded mouse move X coordinate is 110
                EXPECT_EQ(event.x, 110);
                // Verify the recorded mouse move Y coordinate is 210
                EXPECT_EQ(event.y, 210);
                break;
            }
        }
        // Verify that a mouse move event was found in the recorded sequence
        EXPECT_TRUE(foundMove);
    }

    TEST_F(RecorderTest, SaveConfig)
    {
        SequenceRecorder recorder(SequenceConfig{ .recordMouseMoves = false, .name = "test-macro", .startKey = "f8", .endKey = "f9", .playStartKey = "f6", .mouseSampleDelay = "25ms" });
        
        recorder.onKeyEvent(Key::fromString("f8"), true);
        recorder.onKeyEvent(Key::fromString("space"), true);
        recorder.onKeyEvent(Key::fromString("space"), false);
        recorder.onKeyEvent(Key::fromString("f9"), true);
        
        // Ensure saving the recorded sequence to a new config file succeeds
        ASSERT_TRUE(recorder.save(testConfigPath, false));
        // Ensure the config file was actually created on disk
        ASSERT_TRUE(std::filesystem::exists(testConfigPath));
        
        // Load and verify
        auto loaded = loadConfigData(testConfigPath);
        // Ensure loading the saved config file succeeds
        ASSERT_TRUE(loaded.has_value());
        // Ensure exactly one sequence is present in the loaded config
        ASSERT_EQ(loaded->sequences.size(), 1);
        // Verify the sequence name matches the configured macro name
        EXPECT_EQ(loaded->sequences[0].name, "test-macro");
        // Verify the play start key matches 'f6'
        EXPECT_EQ(loaded->sequences[0].start, "f6");
        // Ensure both recorded events are present in the saved sequence
        ASSERT_EQ(loaded->sequences[0].events.size(), 2);
        // Verify the first saved event type is KeyDown
        EXPECT_EQ(loaded->sequences[0].events[0].type, RecordedEventType::KeyDown);
        // Verify the first saved event key is 'space'
        EXPECT_EQ(loaded->sequences[0].events[0].key, "space");
    }

    TEST_F(RecorderTest, OverwriteBehavior)
    {
        SequenceRecorder recorder(SequenceConfig{ .recordMouseMoves = false, .name = "test-macro", .startKey = "f8", .endKey = "f9", .playStartKey = "f6", .mouseSampleDelay = "25ms" });
        
        // Create file
        {
            std::ofstream f(testConfigPath);
            f << "already exists";
        }
        
        // Save without force should fail
        // Verify saving without force fails when target config file already exists
        EXPECT_FALSE(recorder.save(testConfigPath, false));
        
        // Save with force should succeed
        // Verify saving with force succeeds even when target file exists
        EXPECT_TRUE(recorder.save(testConfigPath, true));
    }
    
    TEST_F(RecorderTest, SyntheticEventsIgnored)
    {
        SequenceRecorder recorder(SequenceConfig{ .recordMouseMoves = true, .name = "test-macro", .startKey = "f8", .endKey = "f9", .playStartKey = "f6", .mouseSampleDelay = "25ms" });
        
        recorder.onKeyEvent(Key::fromString("f8"), true); // Start
        
        recorder.onKeyEvent(Key::fromString("a"), true, true); // Synthetic
        recorder.onMouseEvent(Mouse(MouseButton::Left), true, 100, 200, true); // Synthetic
        recorder.onMouseMove(300, 400, true); // Synthetic
        
        recorder.onKeyEvent(Key::fromString("f9"), true); // End
        
        // Verify synthetic events are ignored and not recorded into sequence
        EXPECT_TRUE(recorder.getSequence().events.empty());
    }
}

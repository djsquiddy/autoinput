/**
 * @file recorderTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include "autoinput/sequenceRecorder.h"
#include "autoinput/types.h"
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
        SequenceRecorder recorder("test-macro", "f8", "f9", "f6", false, "25ms");
        
        EXPECT_EQ(recorder.getState(), RecorderState::Waiting);

        // Pressing non-start key does nothing
        recorder.onKeyEvent(Key::fromString("a"), true);
        EXPECT_EQ(recorder.getState(), RecorderState::Waiting);
        EXPECT_TRUE(recorder.getSequence().events.empty());

        // Pressing start key
        recorder.onKeyEvent(Key::fromString("f8"), true);
        EXPECT_EQ(recorder.getState(), RecorderState::Recording);

        // Record some events
        recorder.onKeyEvent(Key::fromString("a"), true);
        recorder.onKeyEvent(Key::fromString("a"), false);
        
        EXPECT_EQ(recorder.getSequence().events.size(), 2);
        EXPECT_EQ(recorder.getSequence().events[0].type, RecordedEventType::KeyDown);
        EXPECT_EQ(recorder.getSequence().events[0].key, "a");
        EXPECT_EQ(recorder.getSequence().events[1].type, RecordedEventType::KeyUp);
        EXPECT_EQ(recorder.getSequence().events[1].key, "a");

        // Pressing end key
        recorder.onKeyEvent(Key::fromString("f9"), true);
        EXPECT_EQ(recorder.getState(), RecorderState::Finished);

        // Events after end are not recorded
        recorder.onKeyEvent(Key::fromString("b"), true);
        EXPECT_EQ(recorder.getSequence().events.size(), 2);
    }

    TEST_F(RecorderTest, ExcludeControlKeys)
    {
        SequenceRecorder recorder("test-macro", "f8", "f9", "f6", false, "25ms");
        
        recorder.onKeyEvent(Key::fromString("f8"), true); // Start
        recorder.onKeyEvent(Key::fromString("a"), true);
        recorder.onKeyEvent(Key::fromString("f9"), true); // End
        
        const auto& events = recorder.getSequence().events;
        ASSERT_EQ(events.size(), 1);
        EXPECT_EQ(events[0].key, "a");
        
        for (const auto& event : events)
        {
            EXPECT_NE(event.key, "f8");
            EXPECT_NE(event.key, "f9");
        }
    }

    TEST_F(RecorderTest, RecordMouseEvents)
    {
        SequenceRecorder recorder("test-macro", "f8", "f9", "f6", true, "25ms");
        
        recorder.onKeyEvent(Key::fromString("f8"), true); // Start
        
        recorder.onMouseEvent(Mouse(MouseButton::Left), true, 100, 200);
        recorder.onMouseEvent(Mouse(MouseButton::Left), false, 100, 200);
        
        // Move mouse
        recorder.onMouseMove(110, 210);
        
        recorder.onKeyEvent(Key::fromString("f9"), true); // End
        
        const auto& events = recorder.getSequence().events;
        ASSERT_GE(events.size(), 3);
        EXPECT_EQ(events[0].type, RecordedEventType::MouseDown);
        EXPECT_EQ(events[0].button, "left");
        EXPECT_EQ(events[0].x, 100);
        EXPECT_EQ(events[0].y, 200);
        
        EXPECT_EQ(events[1].type, RecordedEventType::MouseUp);
        
        bool foundMove = false;
        for (const auto& event : events)
        {
            if (event.type == RecordedEventType::MouseMove)
            {
                foundMove = true;
                EXPECT_EQ(event.x, 110);
                EXPECT_EQ(event.y, 210);
                break;
            }
        }
        EXPECT_TRUE(foundMove);
    }

    TEST_F(RecorderTest, SaveConfig)
    {
        SequenceRecorder recorder("test-macro", "f8", "f9", "f6", false, "25ms");
        
        recorder.onKeyEvent(Key::fromString("f8"), true);
        recorder.onKeyEvent(Key::fromString("space"), true);
        recorder.onKeyEvent(Key::fromString("space"), false);
        recorder.onKeyEvent(Key::fromString("f9"), true);
        
        ASSERT_TRUE(recorder.save(testConfigPath, false));
        ASSERT_TRUE(std::filesystem::exists(testConfigPath));
        
        // Load and verify
        auto loaded = loadConfigData(testConfigPath);
        ASSERT_TRUE(loaded.has_value());
        ASSERT_EQ(loaded->sequences.size(), 1);
        EXPECT_EQ(loaded->sequences[0].name, "test-macro");
        EXPECT_EQ(loaded->sequences[0].start, "f6");
        ASSERT_EQ(loaded->sequences[0].events.size(), 2);
        EXPECT_EQ(loaded->sequences[0].events[0].type, RecordedEventType::KeyDown);
        EXPECT_EQ(loaded->sequences[0].events[0].key, "space");
    }

    TEST_F(RecorderTest, OverwriteBehavior)
    {
        SequenceRecorder recorder("test-macro", "f8", "f9", "f6", false, "25ms");
        
        // Create file
        {
            std::ofstream f(testConfigPath);
            f << "already exists";
        }
        
        // Save without force should fail
        EXPECT_FALSE(recorder.save(testConfigPath, false));
        
        // Save with force should succeed
        EXPECT_TRUE(recorder.save(testConfigPath, true));
    }
    
    TEST_F(RecorderTest, SyntheticEventsIgnored)
    {
        SequenceRecorder recorder("test-macro", "f8", "f9", "f6", true, "25ms");
        
        recorder.onKeyEvent(Key::fromString("f8"), true); // Start
        
        recorder.onKeyEvent(Key::fromString("a"), true, true); // Synthetic
        recorder.onMouseEvent(Mouse(MouseButton::Left), true, 100, 200, true); // Synthetic
        recorder.onMouseMove(300, 400, true); // Synthetic
        
        recorder.onKeyEvent(Key::fromString("f9"), true); // End
        
        EXPECT_TRUE(recorder.getSequence().events.empty());
    }
}

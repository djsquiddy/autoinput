/**
 * @file sequenceTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/config/config.h"
#include "autoinput/config/configValidator.h"
#include "autoinput/input/sequence.h"
#include "autoinput/platform/backend.h"
#include "autoinput/app/autoinput.h"
#include "testUtils.h"
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>

namespace autoinput
{
    class SequenceTest : public ::testing::Test
    {
    protected:
        test::TemporaryDirectory m_tempDir{ "autoinput_sequence_test" };
    };

    TEST_F(SequenceTest, ParseSequenceConfig)
    {
        const std::string toml = R"toml(
end = "f3"

[[sequence]]
name = "my-macro"
start = "f6"
repeat = false

events = [
  { type = "mouse_move", x = 1000, y = 500, delay = "0ms" },
  { type = "mouse_down", button = "left", x = 1000, y = 500, delay = "120ms" },
  { type = "mouse_up", button = "left", x = 1000, y = 500, delay = "80ms" },
  { type = "key_down", key = "space", delay = "400ms" },
  { type = "key_up", key = "space", delay = "40ms" }
]
)toml";
        const auto path = m_tempDir / "macro.toml";
        std::ofstream file{ path };
        file << toml;
        file.close();

        auto configData = loadConfigData(path);
        // Ensure sequence configuration TOML file was loaded successfully
        ASSERT_TRUE(configData.has_value());
        // Verify the end key was parsed as 'f3'
        EXPECT_EQ(configData->endKey, "f3");
        // Ensure exactly one sequence was parsed from the configuration
        ASSERT_EQ(configData->sequences.size(), 1);

        const auto& seq = configData->sequences[0];
        // Verify sequence name is 'my-macro'
        EXPECT_EQ(seq.name, "my-macro");
        // Verify sequence start key is 'f6'
        EXPECT_EQ(seq.start, "f6");
        // Verify repeat flag is false
        EXPECT_FALSE(seq.repeat);
        // Ensure all five recorded events were parsed
        ASSERT_EQ(seq.events.size(), 5);

        // Verify the first event type is MouseMove
        EXPECT_EQ(seq.events[0].type, RecordedEventType::MouseMove);
        // Verify the mouse X coordinate is 1000
        EXPECT_EQ(seq.events[0].x, 1000);
        // Verify the mouse Y coordinate is 500
        EXPECT_EQ(seq.events[0].y, 500);

        // Verify the second event type is MouseDown
        EXPECT_EQ(seq.events[1].type, RecordedEventType::MouseDown);
        // Verify the mouse button is 'left'
        EXPECT_EQ(seq.events[1].button, "left");
        // Verify the event delay is '120ms'
        EXPECT_EQ(seq.events[1].delay, "120ms");

        // Verify the fourth event type is KeyDown
        EXPECT_EQ(seq.events[3].type, RecordedEventType::KeyDown);
        // Verify the key for fourth event is 'space'
        EXPECT_EQ(seq.events[3].key, "space");
    }

    TEST_F(SequenceTest, ValidateValidSequence)
    {
        RecordedSequence seq;
        seq.name = "test";
        seq.start = "f6";
        
        RecordedEvent e1;
        e1.type = RecordedEventType::KeyDown;
        e1.key = "a";
        e1.delay = "10ms";
        seq.events.push_back(e1);

        ConfigData data;
        data.sequences.push_back(seq);
        data.endKey = "f3";

        auto errors = validateConfigData(data);
        // Verify that a complete and valid sequence produces no validation errors
        EXPECT_TRUE(errors.empty());
    }

    TEST_F(SequenceTest, ValidateInvalidSequence)
    {
        RecordedSequence seq;
        seq.name = "test";
        // Missing start key
        
        RecordedEvent e1;
        e1.type = RecordedEventType::KeyDown;
        // Missing key
        seq.events.push_back(e1);

        ConfigData data;
        data.sequences.push_back(seq);

        auto errors = validateConfigData(data);
        // Ensure validation errors are produced for an invalid sequence
        ASSERT_FALSE(errors.empty());
        
        bool foundStartError = false;
        bool foundKeyError = false;
        for (const auto& err : errors)
        {
            if (err.message.find("start key is required") != std::string::npos) foundStartError = true;
            if (err.message.find("Key is required for key event") != std::string::npos) foundKeyError = true;
        }
        // Verify that a missing start key error was reported
        EXPECT_TRUE(foundStartError);
        // Verify that a missing key name error for key event was reported
        EXPECT_TRUE(foundKeyError);
    }

    class MockBackend : public FakeBackend
    {
    public:
        struct EventCall {
            RecordedEventType type;
            std::string value;
            int32_t x = 0;
            int32_t y = 0;
        };
        std::vector<EventCall> calls;
        std::mutex mutex;

        void keyDown(const Key& key) override { 
            std::lock_guard lock(mutex);
            calls.push_back({RecordedEventType::KeyDown, key.toString()}); 
        }
        void keyUp(const Key& key) override { 
            std::lock_guard lock(mutex);
            calls.push_back({RecordedEventType::KeyUp, key.toString()}); 
        }
        void mouseDown(const Mouse& mouse) override { 
            std::lock_guard lock(mutex);
            calls.push_back({RecordedEventType::MouseDown, mouse.toString()}); 
        }
        void mouseUp(const Mouse& mouse) override { 
            std::lock_guard lock(mutex);
            calls.push_back({RecordedEventType::MouseUp, mouse.toString()}); 
        }
        void moveMouseTo(int32_t x, int32_t y) override { 
            std::lock_guard lock(mutex);
            calls.push_back({RecordedEventType::MouseMove, "", x, y}); 
        }
    };

    TEST_F(SequenceTest, PlaybackDispatch)
    {
        RecordedSequence seq;
        seq.name = "macro";
        seq.start = "f6";
        
        RecordedEvent e1;
        e1.type = RecordedEventType::KeyDown;
        e1.key = "a";
        e1.delay = "0ms";
        seq.events.push_back(e1);

        RecordedEvent e2;
        e2.type = RecordedEventType::MouseMove;
        e2.x = 100;
        e2.y = 200;
        e2.delay = "0ms";
        seq.events.push_back(e2);

        MockBackend backend;
        SequenceHandler handler(seq, &backend);
        
        handler.press();
        
        // Wait for thread to finish playback - more robustly
        int retries = 0;
        bool finished = false;
        while (retries < 200)
        {
            {
                std::lock_guard lock(backend.mutex);
                if (backend.calls.size() >= 2) {
                    finished = true;
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            retries++;
        }
        
        // Verify that sequence playback completed within the timeout period
        EXPECT_TRUE(finished) << "Playback timed out";
        
        {
            std::lock_guard lock(backend.mutex);
            // Ensure both sequence events were dispatched to the backend
            ASSERT_EQ(backend.calls.size(), 2);
            // Verify the first dispatched event is KeyDown
            EXPECT_EQ(backend.calls[0].type, RecordedEventType::KeyDown);
            // Verify the key dispatched for the first event is 'a'
            EXPECT_EQ(backend.calls[0].value, "a");
            // Verify the second dispatched event is MouseMove
            EXPECT_EQ(backend.calls[1].type, RecordedEventType::MouseMove);
            // Verify the mouse X coordinate dispatched is 100
            EXPECT_EQ(backend.calls[1].x, 100);
            // Verify the mouse Y coordinate dispatched is 200
            EXPECT_EQ(backend.calls[1].y, 200);
        }

        handler.release();
    }
}

/**
 * @file sequenceTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/config.h"
#include "autoinput/configValidator.h"
#include "autoinput/sequence.h"
#include "autoinput/backend.h"
#include "autoinput/autoinput.h"
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
        ASSERT_TRUE(configData.has_value());
        EXPECT_EQ(configData->endKey, "f3");
        ASSERT_EQ(configData->sequences.size(), 1);

        const auto& seq = configData->sequences[0];
        EXPECT_EQ(seq.name, "my-macro");
        EXPECT_EQ(seq.start, "f6");
        EXPECT_FALSE(seq.repeat);
        ASSERT_EQ(seq.events.size(), 5);

        EXPECT_EQ(seq.events[0].type, RecordedEventType::MouseMove);
        EXPECT_EQ(seq.events[0].x, 1000);
        EXPECT_EQ(seq.events[0].y, 500);

        EXPECT_EQ(seq.events[1].type, RecordedEventType::MouseDown);
        EXPECT_EQ(seq.events[1].button, "left");
        EXPECT_EQ(seq.events[1].delay, "120ms");

        EXPECT_EQ(seq.events[3].type, RecordedEventType::KeyDown);
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
        ASSERT_FALSE(errors.empty());
        
        bool foundStartError = false;
        bool foundKeyError = false;
        for (const auto& err : errors)
        {
            if (err.message.find("start key is required") != std::string::npos) foundStartError = true;
            if (err.message.find("Key is required for key event") != std::string::npos) foundKeyError = true;
        }
        EXPECT_TRUE(foundStartError);
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

        void keyDown(const Key& key) override { calls.push_back({RecordedEventType::KeyDown, key.toString()}); }
        void keyUp(const Key& key) override { calls.push_back({RecordedEventType::KeyUp, key.toString()}); }
        void mouseDown(const Mouse& mouse) override { calls.push_back({RecordedEventType::MouseDown, mouse.toString()}); }
        void mouseUp(const Mouse& mouse) override { calls.push_back({RecordedEventType::MouseUp, mouse.toString()}); }
        void moveMouseTo(int32_t x, int32_t y) override { calls.push_back({RecordedEventType::MouseMove, "", x, y}); }
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
        
        // Wait for thread to finish playback
        int retries = 0;
        while (handler.getActive() && retries < 100)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            retries++;
        }
        
        ASSERT_EQ(backend.calls.size(), 2);
        EXPECT_EQ(backend.calls[0].type, RecordedEventType::KeyDown);
        EXPECT_EQ(backend.calls[0].value, "a");
        EXPECT_EQ(backend.calls[1].type, RecordedEventType::MouseMove);
        EXPECT_EQ(backend.calls[1].x, 100);
        EXPECT_EQ(backend.calls[1].y, 200);

        handler.release();
    }
}

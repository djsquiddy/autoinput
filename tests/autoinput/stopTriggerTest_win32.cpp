/**
 * @file stopTriggerTest_win32.cpp
 */
#include "autoinput/app/autoinput.h"
#include "autoinput/platform/win32/internalData_win32.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#ifdef _WIN32
#include <windows.h>

namespace autoinput
{
    class StopTriggerReproTest : public ::testing::Test
    {
    protected:
        std::unique_ptr<Program> m_testProgram;
        void SetUp() override
        {
            m_testProgram = std::make_unique<Program>();
            m_testProgram->setBackend(std::make_unique<FakeBackend>());
            g_program = m_testProgram.get();
        }

        void TearDown() override
        {
            if (g_program)
            {
                g_program->end();
            }
            g_program = nullptr;
            m_testProgram.reset();
        }
    };

    TEST_F(StopTriggerReproTest, StopKeyIgnoredWhenBlacklistedRepro)
    {
        g_program->arguments().blacklist = {"notepad"};
        g_program->arguments().endKey = "end";
        // We need to manually add the end key to m_keyInfo because we are not calling init() which parses config files
        // Or we can call init() if we have a valid config, but it's easier to just push it.
        // Actually, init() also sets up the backend.
        
        // Let's call init() with minimal arguments
        ASSERT_TRUE(g_program->init());

        // The default init() might not set "end" if it's not in defaults.
        // Let's check defaults.h or just manually add it.
        // Looking at Program::init(), it clears m_keyInfo and then populates it from arguments.
        
        g_program->setTestActiveApp("Notepad.exe");
        ASSERT_TRUE(g_program->isApplicationBlacklisted());

        // Simulate "End" key press (VK_END = 0x23)
        KBDLLHOOKSTRUCT kbdStruct{};
        kbdStruct.vkCode = VK_END;
        
        WindowsKeyboardData winData{};
        winData.wParam = WM_KEYDOWN;
        winData.kbdStruct = &kbdStruct;

        KeyboardData data;
        data.internal = winData;
        
        // In the fixed state, this should return true and stop automation
        EXPECT_TRUE(g_program->processKeyEvent(KeyboardInput(data)));
    }

    TEST_F(StopTriggerReproTest, SpecialKeysWorkAsStopTriggers)
    {
        struct TestCase {
            std::string keyName;
            WORD vkCode;
        };

        std::vector<TestCase> testCases = {
            {"end", VK_END},
            {"home", VK_HOME},
            {"insert", VK_INSERT},
            {"delete", VK_DELETE},
            {"pageup", VK_PRIOR},
            {"pagedown", VK_NEXT}
        };

        for (const auto& tc : testCases)
        {
            auto testProgram = std::make_unique<Program>();
            testProgram->setBackend(std::make_unique<FakeBackend>());
            g_program = testProgram.get();
            g_program->arguments().endKey = tc.keyName;
            ASSERT_TRUE(g_program->init());

            KBDLLHOOKSTRUCT kbdStruct{};
            kbdStruct.vkCode = tc.vkCode;
            
            WindowsKeyboardData winData{};
            winData.wParam = WM_KEYDOWN;
            winData.kbdStruct = &kbdStruct;

            KeyboardData data;
            data.internal = winData;
            
            EXPECT_TRUE(g_program->processKeyEvent(KeyboardInput(data))) << "Failed for key: " << tc.keyName;
            
            g_program = nullptr;
        }
    }

    TEST_F(StopTriggerReproTest, DefaultF3StopKeyWorks)
    {
        ASSERT_TRUE(g_program->init());

        KBDLLHOOKSTRUCT kbdStruct{};
        kbdStruct.vkCode = VK_F3;
        
        WindowsKeyboardData winData{};
        winData.wParam = WM_KEYDOWN;
        winData.kbdStruct = &kbdStruct;

        KeyboardData data;
        data.internal = winData;
        
        EXPECT_TRUE(g_program->processKeyEvent(KeyboardInput(data)));
    }

    TEST_F(StopTriggerReproTest, StartAndEndCanBothBeSpecialKeys)
    {
        g_program->arguments().startKeys = {"home"};
        g_program->arguments().endKey = "end";
        g_program->arguments().keys = {Key::fromString("a")};
        ASSERT_TRUE(g_program->init());

        // Press Home to start
        KBDLLHOOKSTRUCT homeStruct{};
        homeStruct.vkCode = VK_HOME;
        WindowsKeyboardData homeData{ WM_KEYDOWN, &homeStruct };
        KeyboardData dataHome;
        dataHome.internal = homeData;
        EXPECT_TRUE(g_program->processKeyEvent(KeyboardInput(dataHome)));
        
        // Small sleep to let the thread run a bit
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Press End to stop
        KBDLLHOOKSTRUCT endStruct{};
        endStruct.vkCode = VK_END;
        WindowsKeyboardData endData{ WM_KEYDOWN, &endStruct };
        KeyboardData dataEnd;
        dataEnd.internal = endData;
        EXPECT_TRUE(g_program->processKeyEvent(KeyboardInput(dataEnd)));
    }
}
#endif

/**
 * @file focusTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/app/autoinput.h"
#include "autoinput/app/handlerState.h"
#include "autoinput/input/mouse.h"
#include "autoinput/input/keyboard.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace autoinput;

class FocusTest : public ::testing::Test
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
        g_program = nullptr;
        m_testProgram.reset();
    }
};

TEST_F(FocusTest, PauseOnBlacklistedApp)
{
    g_program->arguments().blacklist = {"notepad"};
    
    // Add some handlers
    g_program->arguments().buttons = {MouseButton::Left};
    g_program->arguments().keys = {Key::fromString("a")};
    // Ensure program initializes successfully
    ASSERT_TRUE(g_program->init());
    
    // Initially not paused
    for (auto& [mouse, handler] : g_program->getMouseHandlers())
    {
        // Verify mouse handler is initially not paused
        EXPECT_FALSE(handler.getPaused());
    }
    for (auto& [key, handler] : g_program->getKeyHandlers())
    {
        // Verify key handler is initially not paused
        EXPECT_FALSE(handler.getPaused());
    }
    
    g_program->onFocusChanged("Notepad.exe");
    
    // Should be paused now
    for (auto& [mouse, handler] : g_program->getMouseHandlers())
    {
        // Ensure mouse handler pauses when blacklisted application gains focus
        EXPECT_TRUE(handler.getPaused());
    }
    for (auto& [key, handler] : g_program->getKeyHandlers())
    {
        // Ensure key handler pauses when blacklisted application gains focus
        EXPECT_TRUE(handler.getPaused());
    }

    g_program->onFocusChanged("Explorer.exe");

    // Should be resumed now
    for (auto& [mouse, handler] : g_program->getMouseHandlers())
    {
        // Verify mouse handler resumes when focus moves away from blacklisted app
        EXPECT_FALSE(handler.getPaused());
    }
    for (auto& [key, handler] : g_program->getKeyHandlers())
    {
        // Verify key handler resumes when focus moves away from blacklisted app
        EXPECT_FALSE(handler.getPaused());
    }
}

TEST_F(FocusTest, PauseOnLostFocusFromTargetApp)
{
    g_program->arguments().applicationName = "mygame";
    
    // Add some handlers
    g_program->arguments().buttons = {MouseButton::Left};
    // Ensure program initializes successfully
    ASSERT_TRUE(g_program->init());
    
    g_program->onFocusChanged("mygame.exe");
    // Verify handler is unpaused when target application is focused
    EXPECT_FALSE(g_program->getMouseHandlers().begin()->second.getPaused());

    g_program->onFocusChanged("Notepad.exe");
    // Ensure handler pauses when focus switches away from target application
    EXPECT_TRUE(g_program->getMouseHandlers().begin()->second.getPaused());

    g_program->onFocusChanged("mygame.exe");
    // Verify handler resumes when target application regains focus
    EXPECT_FALSE(g_program->getMouseHandlers().begin()->second.getPaused());
}

TEST_F(FocusTest, CachedForegroundStateUpdatesOnEvent)
{
    ASSERT_TRUE(g_program->init());

    AppWindowInfo info{
        .processName = "code.exe",
        .windowTitle = "Visual Studio Code",
        .pid = 12345,
        .executablePath = "C:\\Program Files\\VSCode\\Code.exe",
        .backendId = "0x1234"
    };

    g_program->onFocusChanged(info);

    auto cached = g_program->getCachedForegroundWindow();
    ASSERT_TRUE(cached.has_value());
    EXPECT_EQ(cached->processName, "code.exe");
    EXPECT_EQ(cached->windowTitle, "Visual Studio Code");
    EXPECT_EQ(cached->pid, 12345u);
    EXPECT_EQ(cached->executablePath, "C:\\Program Files\\VSCode\\Code.exe");
    EXPECT_EQ(cached->backendId, "0x1234");
}

TEST_F(FocusTest, AppMatchingUsesCachedValue)
{
    // Test target app matching by window title
    g_program->arguments().applicationName = "special window title";
    g_program->arguments().buttons = {MouseButton::Left};
    ASSERT_TRUE(g_program->init());

    AppWindowInfo nonMatchingInfo{
        .processName = "app.exe",
        .windowTitle = "Regular Window",
        .pid = 100,
        .executablePath = "/usr/bin/app",
        .backendId = "100"
    };
    g_program->onFocusChanged(nonMatchingInfo);
    EXPECT_TRUE(g_program->getMouseHandlers().begin()->second.getPaused());
    EXPECT_FALSE(g_program->isApplicationBlacklisted());

    AppWindowInfo matchingTitleInfo{
        .processName = "app.exe",
        .windowTitle = "My Special Window Title",
        .pid = 100,
        .executablePath = "/usr/bin/app",
        .backendId = "100"
    };
    g_program->onFocusChanged(matchingTitleInfo);
    EXPECT_FALSE(g_program->getMouseHandlers().begin()->second.getPaused());

    // Test blacklist matching by executable path
    g_program->arguments().applicationName.clear();
    g_program->arguments().blacklist = {"blocked_path"};

    AppWindowInfo blacklistedExeInfo{
        .processName = "runner.exe",
        .windowTitle = "Runner",
        .pid = 200,
        .executablePath = "/opt/blocked_path/runner",
        .backendId = "200"
    };
    g_program->onFocusChanged(blacklistedExeInfo);
    EXPECT_TRUE(g_program->getMouseHandlers().begin()->second.getPaused());
    EXPECT_TRUE(g_program->isApplicationBlacklisted());
}

TEST_F(FocusTest, EmptyOrUnavailableForegroundAppHandledSafely)
{
    g_program->arguments().applicationName = "mygame";
    g_program->arguments().blacklist = {"forbidden"};
    g_program->arguments().buttons = {MouseButton::Left};
    ASSERT_TRUE(g_program->init());

    // Deliver empty AppWindowInfo
    AppWindowInfo emptyInfo{};
    g_program->onFocusChanged(emptyInfo);

    auto cached = g_program->getCachedForegroundWindow();
    ASSERT_TRUE(cached.has_value());
    EXPECT_TRUE(cached->processName.empty());
    EXPECT_TRUE(cached->windowTitle.empty());
    EXPECT_EQ(cached->pid, 0u);

    // Empty app should not match target app "mygame" -> paused
    EXPECT_TRUE(g_program->getMouseHandlers().begin()->second.getPaused());
    // Empty app should not be blacklisted
    EXPECT_FALSE(g_program->isApplicationBlacklisted());
}

TEST_F(FocusTest, ConsecutiveDuplicateEventsDeduplicated)
{
    g_program->arguments().applicationName = "app";
    g_program->arguments().buttons = {MouseButton::Left};
    ASSERT_TRUE(g_program->init());

    AppWindowInfo info{
        .processName = "app.exe",
        .windowTitle = "App Title",
        .pid = 500,
        .executablePath = "C:\\app.exe",
        .backendId = "0x500"
    };

    // First event
    g_program->onFocusChanged(info);
    EXPECT_FALSE(g_program->getMouseHandlers().begin()->second.getPaused());

    // Repeated identical event
    g_program->onFocusChanged(info);
    EXPECT_FALSE(g_program->getMouseHandlers().begin()->second.getPaused());
}

TEST_F(FocusTest, BackendFocusDetectionCapabilities)
{
    FakeBackend fakeBackend;
    EXPECT_TRUE(fakeBackend.capabilities().focusDetection);

#ifdef _WIN32
    auto winBackend = createWindowsBackend();
    ASSERT_TRUE(winBackend != nullptr);
    EXPECT_TRUE(winBackend->capabilities().focusDetection);
#endif
}

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

/**
 * @file focusTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/autoinput.h"
#include "autoinput/handlerState.h"
#include "autoinput/mouse.h"
#include "autoinput/keyboard.h"
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
    ASSERT_TRUE(g_program->init());
    
    // Initially not paused
    for (auto& [mouse, handler] : g_program->getMouseHandlers())
    {
        EXPECT_FALSE(handler.getPaused());
    }
    for (auto& [key, handler] : g_program->getKeyHandlers())
    {
        EXPECT_FALSE(handler.getPaused());
    }
    
    g_program->onFocusChanged("Notepad.exe");
    
    // Should be paused now
    for (auto& [mouse, handler] : g_program->getMouseHandlers())
    {
        EXPECT_TRUE(handler.getPaused());
    }
    for (auto& [key, handler] : g_program->getKeyHandlers())
    {
        EXPECT_TRUE(handler.getPaused());
    }

    g_program->onFocusChanged("Explorer.exe");

    // Should be resumed now
    for (auto& [mouse, handler] : g_program->getMouseHandlers())
    {
        EXPECT_FALSE(handler.getPaused());
    }
    for (auto& [key, handler] : g_program->getKeyHandlers())
    {
        EXPECT_FALSE(handler.getPaused());
    }
}

TEST_F(FocusTest, PauseOnLostFocusFromTargetApp)
{
    g_program->arguments().applicationName = "mygame";
    
    // Add some handlers
    g_program->arguments().buttons = {MouseButton::Left};
    ASSERT_TRUE(g_program->init());
    
    g_program->onFocusChanged("mygame.exe");
    EXPECT_FALSE(g_program->getMouseHandlers().begin()->second.getPaused());

    g_program->onFocusChanged("Notepad.exe");
    EXPECT_TRUE(g_program->getMouseHandlers().begin()->second.getPaused());

    g_program->onFocusChanged("mygame.exe");
    EXPECT_FALSE(g_program->getMouseHandlers().begin()->second.getPaused());
}

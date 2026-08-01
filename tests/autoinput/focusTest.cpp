/**
 * @file focusTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "autoinput/autoInput.h"
#include "autoinput/handlerState.h"
#include "autoinput/mouse.h"
#include "autoinput/keyboard.h"

using namespace autoinput;

class FocusTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_program = std::make_unique<Program>();
    }

    void TearDown() override
    {
        g_program.reset();
    }
};

TEST_F(FocusTest, PauseOnBlacklistedApp)
{
    g_program->arguments().blacklist = {"notepad"};
    
    // Add some handlers
    g_program->arguments().buttons = {MouseButton::LEFT};
    g_program->arguments().keys = {Key::fromString("a")};
    g_program->init();
    
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
    g_program->arguments().buttons = {MouseButton::LEFT};
    g_program->init();
    
    g_program->onFocusChanged("mygame.exe");
    EXPECT_FALSE(g_program->getMouseHandlers().begin()->second.getPaused());

    g_program->onFocusChanged("Notepad.exe");
    EXPECT_TRUE(g_program->getMouseHandlers().begin()->second.getPaused());

    g_program->onFocusChanged("mygame.exe");
    EXPECT_FALSE(g_program->getMouseHandlers().begin()->second.getPaused());
}

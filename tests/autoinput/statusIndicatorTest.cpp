/**
 * @file statusIndicatorTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/app/autoinput.h"
#include "autoinput/app/handlerState.h"
#include "autoinput/input/mouse.h"
#include "autoinput/input/keyboard.h"
#include "autoinput/platform/backend.h"
#include <gtest/gtest.h>

using namespace autoinput;

class StatusIndicatorTest : public ::testing::Test
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

TEST_F(StatusIndicatorTest, StateTransitionsCorrectly)
{
    g_program->arguments().buttons = {MouseButton::Left};
    // Ensure program initializes successfully
    ASSERT_TRUE(g_program->init());
    
    // Initial state
    // Verify status indicator is initially inactive
    EXPECT_FALSE(g_program->getLastIsActiveIndicator());
    
    // Activate a handler
    auto& handler = g_program->getMouseHandlers().at(MouseButton::Left);
    handler.setActive(true);
    g_program->updateStatusIndicator();
    // Verify status indicator is active after handler activation
    EXPECT_TRUE(g_program->getLastIsActiveIndicator());
    
    // Pause it
    handler.setPaused(true);
    g_program->updateStatusIndicator();
    // Verify status indicator is inactive when handler is paused
    EXPECT_FALSE(g_program->getLastIsActiveIndicator());
    
    // Resume it
    handler.setPaused(false);
    g_program->updateStatusIndicator();
    // Verify status indicator is active when handler is resumed
    EXPECT_TRUE(g_program->getLastIsActiveIndicator());
    
    // Deactivate it
    handler.setActive(false);
    g_program->updateStatusIndicator();
    // Verify status indicator is inactive when handler is deactivated
    EXPECT_FALSE(g_program->getLastIsActiveIndicator());
}

TEST_F(StatusIndicatorTest, RespectsJsonOutputMode)
{
    g_program->arguments().buttons = {MouseButton::Left};
    g_program->arguments().jsonOutput = true;
    // Ensure program initializes successfully in JSON output mode
    ASSERT_TRUE(g_program->init());
    
    // Initial state
    // Verify status indicator is initially inactive
    EXPECT_FALSE(g_program->getLastIsActiveIndicator());
    
    // Activate a handler
    auto& handler = g_program->getMouseHandlers().at(MouseButton::Left);
    handler.setActive(true);
    
    // updateStatusIndicator should return early and not update the indicator state
    g_program->updateStatusIndicator();
    // Verify status indicator is not updated when JSON output mode is enabled
    EXPECT_FALSE(g_program->getLastIsActiveIndicator());
}

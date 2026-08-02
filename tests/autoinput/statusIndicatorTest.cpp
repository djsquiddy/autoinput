/**
 * @file statusIndicatorTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include "autoinput/autoInput.h"
#include "autoinput/handlerState.h"
#include "autoinput/mouse.h"
#include "autoinput/keyboard.h"
#include "autoinput/backend.h"

using namespace autoinput;

class StatusIndicatorTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        g_program = std::make_unique<Program>();
        g_program->setBackend(std::make_unique<FakeBackend>());
    }

    void TearDown() override
    {
        g_program.reset();
    }
};

TEST_F(StatusIndicatorTest, StateTransitionsCorrectly)
{
    g_program->arguments().buttons = {MouseButton::Left};
    ASSERT_TRUE(g_program->init());
    
    // Initial state
    EXPECT_FALSE(g_program->getLastIsActiveIndicator());
    
    // Activate a handler
    auto& handler = g_program->getMouseHandlers().at(MouseButton::Left);
    handler.setActive(true);
    g_program->updateStatusIndicator();
    EXPECT_TRUE(g_program->getLastIsActiveIndicator());
    
    // Pause it
    handler.setPaused(true);
    g_program->updateStatusIndicator();
    EXPECT_FALSE(g_program->getLastIsActiveIndicator());
    
    // Resume it
    handler.setPaused(false);
    g_program->updateStatusIndicator();
    EXPECT_TRUE(g_program->getLastIsActiveIndicator());
    
    // Deactivate it
    handler.setActive(false);
    g_program->updateStatusIndicator();
    EXPECT_FALSE(g_program->getLastIsActiveIndicator());
}

TEST_F(StatusIndicatorTest, RespectsJsonOutputMode)
{
    g_program->arguments().buttons = {MouseButton::Left};
    g_program->arguments().jsonOutput = true;
    ASSERT_TRUE(g_program->init());
    
    // Initial state
    EXPECT_FALSE(g_program->getLastIsActiveIndicator());
    
    // Activate a handler
    auto& handler = g_program->getMouseHandlers().at(MouseButton::Left);
    handler.setActive(true);
    
    // updateStatusIndicator should return early and not update the indicator state
    g_program->updateStatusIndicator();
    EXPECT_FALSE(g_program->getLastIsActiveIndicator());
}

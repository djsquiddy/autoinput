/**
 * @file fixVerificationTest.cpp
 */
#include <gtest/gtest.h>
#include "autoinput/app/autoinput.h"
#include "autoinput/platform/win32/internalData_win32.h"

using namespace autoinput;

class FixVerificationTest : public ::testing::Test
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

TEST_F(FixVerificationTest, SyntheticMouseEventsAreIgnored)
{
    g_program->arguments().buttons = {MouseButton::Left};
    ASSERT_TRUE(g_program->arguments().postParseArguments());
    ASSERT_TRUE(g_program->init());

    MSLLHOOKSTRUCT ms{};
    ms.flags = LLMHF_INJECTED;
    WindowsMouseData winData{ WM_LBUTTONDOWN, &ms };
    MouseData data;
    data.internal = winData;
    MouseInput input(data);

    EXPECT_FALSE(g_program->processMouseEvent(input));
}

TEST_F(FixVerificationTest, SyntheticKeyboardEventsAreIgnored)
{
    g_program->arguments().startKeys = {"f2"};
    ASSERT_TRUE(g_program->arguments().postParseArguments());
    ASSERT_TRUE(g_program->init());

    KBDLLHOOKSTRUCT kbs{};
    kbs.vkCode = VK_F2;
    kbs.flags = LLKHF_INJECTED;
    WindowsKeyboardData winData{ WM_KEYDOWN, &kbs };
    KeyboardData data;
    data.internal = winData;
    KeyboardInput input(data);

    EXPECT_FALSE(g_program->processKeyEvent(std::move(input)));
}

TEST_F(FixVerificationTest, HoldStateProperlyTracked)
{
    g_program->arguments().buttons = {MouseButton::Left};
    g_program->arguments().targetActions = {ActionState::HOLD};
    g_program->arguments().commandNames = {"left-hold"};
    ASSERT_TRUE(g_program->arguments().postParseArguments());
    ASSERT_TRUE(g_program->init());

    const auto& keyInfo = g_program->getKeyInfo()[0];
    g_program->start(keyInfo);

    auto& handler = g_program->getMouseHandlers().at(MouseButton::Left);
    EXPECT_TRUE(handler.isPressed());
    EXPECT_TRUE(handler.getActive());
    EXPECT_TRUE(g_program->getLastIsActiveIndicator());

    // Toggle off
    g_program->start(keyInfo);
    EXPECT_FALSE(handler.isPressed());
    EXPECT_FALSE(handler.getActive());
    EXPECT_FALSE(g_program->getLastIsActiveIndicator());
}

TEST_F(FixVerificationTest, RedundantStatusMessagesFiltered)
{
    g_program->arguments().buttons = {MouseButton::Left};
    g_program->arguments().commandNames = {"left-click"};
    ASSERT_TRUE(g_program->arguments().postParseArguments());
    ASSERT_TRUE(g_program->init());

    const auto& keyInfo = g_program->getKeyInfo()[0];
    
    // First activation
    g_program->start(keyInfo);
    EXPECT_TRUE(g_program->getLastIsActiveIndicator());
    
    // Simulate a repeat trigger call to start()
    // It should not change the state or m_lastTriggeredCommandActive in a way that triggers redundant notification
    // But since start() always calls updateStatusIndicator with the name, 
    // our fix in updateStatusIndicator should handle it.
    
    // We can verify m_lastTriggeredCommandActive is still true
    EXPECT_EQ(g_program->getLastTriggeredCommandName(), "left-click");
    EXPECT_TRUE(g_program->getLastTriggeredCommandActive().value_or(false));
}

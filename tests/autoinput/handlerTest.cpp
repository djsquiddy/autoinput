/**
 * @file handlerTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/autoinput.h"
#include "autoinput/keyboard.h"
#include "autoinput/mouse.h"
#include "autoinput/backend.h"
#include "testUtils.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace autoinput;
using ::testing::_;
using ::testing::Exactly;

class MockPlatformBackend : public IPlatformBackend
{
public:
    MOCK_METHOD(bool, installHooks, (), (override));
    MOCK_METHOD(void, runListener, (), (override));
    MOCK_METHOD(void, cleanup, (), (override));
    MOCK_METHOD(void, requestStop, (), (override));
    MOCK_METHOD(void, keyPress, (const Key& key), (override));
    MOCK_METHOD(void, keyRelease, (const Key& key), (override));
    MOCK_METHOD(void, mousePress, (const Mouse& mouse), (override));
    MOCK_METHOD(void, mouseRelease, (const Mouse& mouse), (override));
    MOCK_METHOD(void, keyDown, (const Key& key), (override));
    MOCK_METHOD(void, keyUp, (const Key& key), (override));
    MOCK_METHOD(void, mouseDown, (const Mouse& mouse), (override));
    MOCK_METHOD(void, mouseUp, (const Mouse& mouse), (override));
    MOCK_METHOD(void, moveMouseTo, (int32_t x, int32_t y), (override));
    MOCK_METHOD((std::pair<int32_t, int32_t>), getCursorPosition, (), (override));
    BackendCapabilities capabilities() const override { return {}; }
};

class HandlerTest : public ::testing::Test
{
protected:
    std::unique_ptr<Program> m_testProgram;
    void SetUp() override
    {
        m_testProgram = std::make_unique<Program>();
        auto mock = std::make_unique<MockPlatformBackend>();
        mockPtr = mock.get();
        m_testProgram->setBackend(std::move(mock));
        g_program = m_testProgram.get();
    }

    void TearDown() override
    {
        g_program = nullptr;
        m_testProgram.reset();
    }

    MockPlatformBackend* mockPtr{ nullptr };
};

TEST_F(HandlerTest, KeyHandlerPressAndRelease)
{
    Key key = Key::fromString("a");
    KeyHandler handler(key, mockPtr);

    EXPECT_CALL(*mockPtr, keyPress(key)).Times(Exactly(1));
    handler.press();
    EXPECT_TRUE(handler.isPressed());

    // Redundant press should be ignored
    EXPECT_CALL(*mockPtr, keyPress(_)).Times(0);
    handler.press();

    EXPECT_CALL(*mockPtr, keyRelease(key)).Times(Exactly(1));
    handler.release();
    EXPECT_FALSE(handler.isPressed());

    // Redundant release should be ignored
    EXPECT_CALL(*mockPtr, keyRelease(_)).Times(0);
    handler.release();
}

TEST_F(HandlerTest, MouseHandlerPressAndRelease)
{
    Mouse mouse(MouseButton::Left);
    MouseHandler handler(mouse, mockPtr);

    EXPECT_CALL(*mockPtr, mousePress(mouse)).Times(Exactly(1));
    handler.press();
    EXPECT_TRUE(handler.isPressed());

    // Redundant press should be ignored
    EXPECT_CALL(*mockPtr, mousePress(_)).Times(0);
    handler.press();

    EXPECT_CALL(*mockPtr, mouseRelease(mouse)).Times(Exactly(1));
    handler.release();
    EXPECT_FALSE(handler.isPressed());

    // Redundant release should be ignored
    EXPECT_CALL(*mockPtr, mouseRelease(_)).Times(0);
    handler.release();
}

TEST_F(HandlerTest, HandlerWithNullBackend)
{
    KeyHandler keyHandler(Key::fromString("a"), nullptr);
    keyHandler.press();
    EXPECT_FALSE(keyHandler.isPressed()); // Should not be pressed if backend is null

    MouseHandler mouseHandler(MouseButton::Left, nullptr);
    mouseHandler.press();
    EXPECT_FALSE(mouseHandler.isPressed());
}

TEST_F(HandlerTest, PauseState)
{
    KeyHandler handler(Key::fromString("a"));
    EXPECT_FALSE(handler.getPaused());
    
    handler.setPaused(true);
    EXPECT_TRUE(handler.getPaused());
    
    handler.setPaused(false);
    EXPECT_FALSE(handler.getPaused());
}

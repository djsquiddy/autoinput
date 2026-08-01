/**
 * @file handlerTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "autoinput/keyboard.h"
#include "autoinput/mouse.h"
#include "autoinput/backend.h"
#include "testUtils.h"

using namespace autoinput;
using ::testing::_;
using ::testing::Exactly;

class MockPlatformBackend : public IPlatformBackend
{
public:
    MOCK_METHOD(bool, installHooks, (), (override));
    MOCK_METHOD(void, runListener, (), (override));
    MOCK_METHOD(void, cleanup, (), (override));
    MOCK_METHOD(void, keyPress, (const Key& key), (override));
    MOCK_METHOD(void, keyRelease, (const Key& key), (override));
    MOCK_METHOD(void, mousePress, (const Mouse& mouse), (override));
    MOCK_METHOD(void, mouseRelease, (const Mouse& mouse), (override));
};

class HandlerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        auto mock = std::make_unique<MockPlatformBackend>();
        mockPtr = mock.get();
        m_override = std::make_unique<test::ScopedBackendOverride>(std::move(mock));
    }

    void TearDown() override
    {
        m_override.reset();
    }

    MockPlatformBackend* mockPtr{ nullptr };
    std::unique_ptr<test::ScopedBackendOverride> m_override;
};

TEST_F(HandlerTest, KeyHandlerPressAndRelease)
{
    Key key = Key::fromString("a");
    KeyHandler handler(key);

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
    Mouse mouse(MouseButton::LEFT);
    MouseHandler handler(mouse);

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
    test::ScopedBackendOverride nullOverride(nullptr);
    
    KeyHandler keyHandler(Key::fromString("a"));
    keyHandler.press();
    EXPECT_FALSE(keyHandler.isPressed()); // Should not be pressed if backend is null

    MouseHandler mouseHandler(MouseButton::LEFT);
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

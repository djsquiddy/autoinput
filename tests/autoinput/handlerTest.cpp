#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "autoinput/keyboard.h"
#include "autoinput/mouse.h"
#include "autoinput/backend.h"

using namespace autoinput;
using ::testing::_;
using ::testing::Exactly;

class MockPlatformBackend : public PlatformBackend
{
public:
    MOCK_METHOD(bool, installHooks, (), (override));
    MOCK_METHOD(void, runListener, (), (override));
    MOCK_METHOD(void, cleanup, (), (override));
    MOCK_METHOD(void, keyPress, (const Key& key), (override));
    MOCK_METHOD(void, keyRelease, (const Key& key), (override));
    MOCK_METHOD(void, mousePress, (MouseButton button), (override));
    MOCK_METHOD(void, mouseRelease, (MouseButton button), (override));
};

class HandlerTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        mockBackend = std::make_unique<MockPlatformBackend>();
        g_backend = std::move(mockBackend);
        // We need a pointer to the mock that remains valid after g_backend takes ownership
        // But g_backend is a unique_ptr, so we can't easily keep a pointer to it if we want to reset it.
        // Let's use a raw pointer to the mock for expectations.
        mockPtr = dynamic_cast<MockPlatformBackend*>(g_backend.get());
    }

    void TearDown() override
    {
        g_backend.reset();
    }

    MockPlatformBackend* mockPtr{ nullptr };
    std::unique_ptr<MockPlatformBackend> mockBackend;
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
    MouseButton button = MouseButton::LEFT;
    MouseHandler handler(button);

    EXPECT_CALL(*mockPtr, mousePress(button)).Times(Exactly(1));
    handler.press();
    EXPECT_TRUE(handler.isPressed());

    // Redundant press should be ignored
    EXPECT_CALL(*mockPtr, mousePress(_)).Times(0);
    handler.press();

    EXPECT_CALL(*mockPtr, mouseRelease(button)).Times(Exactly(1));
    handler.release();
    EXPECT_FALSE(handler.isPressed());

    // Redundant release should be ignored
    EXPECT_CALL(*mockPtr, mouseRelease(_)).Times(0);
    handler.release();
}

TEST_F(HandlerTest, HandlerWithNullBackend)
{
    g_backend.reset();
    
    KeyHandler keyHandler(Key::fromString("a"));
    keyHandler.press();
    EXPECT_FALSE(keyHandler.isPressed()); // Should not be pressed if backend is null

    MouseHandler mouseHandler(MouseButton::LEFT);
    mouseHandler.press();
    EXPECT_FALSE(mouseHandler.isPressed());
}

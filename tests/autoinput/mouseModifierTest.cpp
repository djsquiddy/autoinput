/**
 * @file mouseModifierTest.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include "autoinput/app/autoinput.h"
#include "autoinput/platform/backend.h"
#include "testUtils.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace autoinput;
using ::testing::Exactly;

class MockMouseModifierBackend : public IPlatformBackend
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
    MOCK_METHOD(std::vector<AppWindowInfo>, enumerateWindows, (), (override));
    MOCK_METHOD(std::optional<AppWindowInfo>, getForegroundWindow, (), (override));
    BackendCapabilities capabilities() const override { return {}; }
    std::string getName() const override { return "Mock Mouse Modifier Backend"; }
};

TEST(MouseModifierTest, ParsesShiftLeftClick)
{
    auto mouse = Mouse::fromString("shift+left");
    EXPECT_EQ(mouse.button, MouseButton::Left);
    EXPECT_EQ(mouse.modifier, KeyModifier::Shift);
}

TEST(MouseModifierTest, ParsesMultipleModifiersWithClick)
{
    auto mouse = Mouse::fromString("ctrl+alt+right");
    EXPECT_EQ(mouse.button, MouseButton::Right);
    EXPECT_TRUE(static_cast<bool>(mouse.modifier & KeyModifier::Ctrl));
    EXPECT_TRUE(static_cast<bool>(mouse.modifier & KeyModifier::Alt));
}

TEST(MouseModifierTest, ProgramTriggersShiftLeftClick)
{
    Program program;
    ProgramArguments& arguments = program.arguments();
    arguments.buttons.push_back(Mouse(MouseButton::Left, KeyModifier::Shift));
    arguments.targetActions.push_back(ActionState::HOLD);
    arguments.startKeys.push_back("f2");
    ASSERT_TRUE(arguments.postParseArguments());

    auto mock = std::make_unique<MockMouseModifierBackend>();
    MockMouseModifierBackend* mockPtr = mock.get();
    program.setBackend(std::move(mock));
    ASSERT_TRUE(program.init());

    const auto& keyInfo = program.getKeyInfo();
    ASSERT_EQ(keyInfo.size(), 2); // start and end

    Mouse expectedMouse(MouseButton::Left, KeyModifier::Shift);
    EXPECT_CALL(*mockPtr, mousePress(expectedMouse)).Times(Exactly(1));
    
    program.start(keyInfo[0]);
}

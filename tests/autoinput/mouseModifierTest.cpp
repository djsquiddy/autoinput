/**
 * @file mouseModifierTest.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "autoinput/autoInput.h"
#include "autoinput/backend.h"
#include "autoinput/arguments.h"
#include "testUtils.h"

using namespace autoinput;
using ::testing::Exactly;

class MockMouseModifierBackend : public IPlatformBackend
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

TEST(MouseModifierTest, ParsesShiftLeftClick)
{
    ProgramArguments arguments;
    char* argv[] = {(char*)"autoinput", (char*)"shift+left"};
    int argc = sizeof(argv) / sizeof(char*);

    ASSERT_TRUE(arguments.parseArguments(gsl::make_span(argv, argc)));
    ASSERT_EQ(arguments.buttons.size(), 1);
    EXPECT_EQ(arguments.buttons[0].button, MouseButton::LEFT);
    EXPECT_EQ(arguments.buttons[0].modifier, KeyModifier::Shift);
}

TEST(MouseModifierTest, ParsesMultipleModifiersWithClick)
{
    ProgramArguments arguments;
    char* argv[] = {(char*)"autoinput", (char*)"ctrl+alt+right"};
    int argc = sizeof(argv) / sizeof(char*);

    ASSERT_TRUE(arguments.parseArguments(gsl::make_span(argv, argc)));
    ASSERT_EQ(arguments.buttons.size(), 1);
    EXPECT_EQ(arguments.buttons[0].button, MouseButton::RIGHT);
    EXPECT_TRUE(static_cast<bool>(arguments.buttons[0].modifier & KeyModifier::Ctrl));
    EXPECT_TRUE(static_cast<bool>(arguments.buttons[0].modifier & KeyModifier::Alt));
}

TEST(MouseModifierTest, ProgramTriggersShiftLeftClick)
{
    Program program;
    char* argv[] = {(char*)"autoinput", (char*)"hold", (char*)"shift+left", (char*)"-s", (char*)"f2"};
    int argc = sizeof(argv) / sizeof(char*);

    ASSERT_TRUE(program.arguments().parseArguments(gsl::make_span(argv, argc)));
    auto mock = std::make_unique<MockMouseModifierBackend>();
    MockMouseModifierBackend* mockPtr = mock.get();
    program.setBackend(std::move(mock));
    program.init();

    const auto& keyInfo = program.getKeyInfo();
    ASSERT_EQ(keyInfo.size(), 2); // start and end

    Mouse expectedMouse(MouseButton::LEFT, KeyModifier::Shift);
    EXPECT_CALL(*mockPtr, mousePress(expectedMouse)).Times(Exactly(1));
    
    program.start(keyInfo[0]);
}

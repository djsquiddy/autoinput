/**
 * @file positionalArgumentsTest.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include <gtest/gtest.h>
#include "autoinput/arguments.h"

namespace autoinput
{
    class PositionalArgumentsTest : public ::testing::Test
    {
    protected:
        ProgramArguments args;
    };

    TEST_F(PositionalArgumentsTest, ParsesPositionalActionAndButton)
    {
        char* argv[] = {(char*)"autoinput", (char*)"click", (char*)"left"};
        int argc = sizeof(argv) / sizeof(char*);

        EXPECT_TRUE(args.parseArguments(gsl::make_span(argv, argc)));
        EXPECT_EQ(args.actionState, ActionState::CLICK);
        ASSERT_EQ(args.buttons.size(), 1);
        EXPECT_EQ(args.buttons[0], MouseButton::LEFT);
        ASSERT_EQ(args.startKeys.size(), 1);
        EXPECT_EQ(args.startKeys[0], "f2");
        EXPECT_EQ(args.endKey, "f3");
    }

    TEST_F(PositionalArgumentsTest, ParsesPositionalButtonOnlyDefaultsToClick)
    {
        char* argv[] = {(char*)"autoinput", (char*)"left"};
        int argc = sizeof(argv) / sizeof(char*);

        EXPECT_TRUE(args.parseArguments(gsl::make_span(argv, argc)));
        EXPECT_EQ(args.actionState, ActionState::CLICK);
        ASSERT_EQ(args.buttons.size(), 1);
        EXPECT_EQ(args.buttons[0], MouseButton::LEFT);
        ASSERT_EQ(args.startKeys.size(), 1);
        EXPECT_EQ(args.startKeys[0], "f2");
        EXPECT_EQ(args.endKey, "f3");
    }

    TEST_F(PositionalArgumentsTest, ParsesMultiplePositionalButtons)
    {
        char* argv[] = {(char*)"autoinput", (char*)"left", (char*)"right"};
        int argc = sizeof(argv) / sizeof(char*);

        EXPECT_TRUE(args.parseArguments(gsl::make_span(argv, argc)));
        ASSERT_EQ(args.buttons.size(), 2);
        EXPECT_EQ(args.buttons[0], MouseButton::LEFT);
        EXPECT_EQ(args.buttons[1], MouseButton::RIGHT);
        ASSERT_EQ(args.startKeys.size(), 2);
        EXPECT_EQ(args.startKeys[0], "f2");
        EXPECT_EQ(args.startKeys[1], "f2"); // Resized to match target count
        EXPECT_EQ(args.endKey, "f3");
    }

    TEST_F(PositionalArgumentsTest, ParsesPositionalActionWithMultipleButtons)
    {
        char* argv[] = {(char*)"autoinput", (char*)"hold", (char*)"left", (char*)"right"};
        int argc = sizeof(argv) / sizeof(char*);

        EXPECT_TRUE(args.parseArguments(gsl::make_span(argv, argc)));
        EXPECT_EQ(args.actionState, ActionState::HOLD);
        ASSERT_EQ(args.buttons.size(), 2);
        EXPECT_EQ(args.buttons[0], MouseButton::LEFT);
        EXPECT_EQ(args.buttons[1], MouseButton::RIGHT);
    }

    TEST_F(PositionalArgumentsTest, ParsesPositionalKey)
    {
        char* argv[] = {(char*)"autoinput", (char*)"a"};
        int argc = sizeof(argv) / sizeof(char*);

        EXPECT_TRUE(args.parseArguments(gsl::make_span(argv, argc)));
        EXPECT_EQ(args.actionState, ActionState::CLICK);
        ASSERT_EQ(args.keys.size(), 1);
        EXPECT_EQ(args.keys[0].character, "a");
    }

    TEST_F(PositionalArgumentsTest, ParsesMouseButtonsInStartAndEndKeys)
    {
        char* argv[] = {(char*)"autoinput", (char*)"left", (char*)"-s", (char*)"back", (char*)"-e", (char*)"forward"};
        int argc = sizeof(argv) / sizeof(char*);

        EXPECT_TRUE(args.parseArguments(gsl::make_span(argv, argc)));
        ASSERT_EQ(args.startKeys.size(), 1);
        EXPECT_EQ(args.startKeys[0], "back");
        EXPECT_EQ(args.endKey, "forward");
    }

    TEST_F(PositionalArgumentsTest, ParsesPairedTargetAndStartKey)
    {
        char* argv[] = {(char*)"autoinput", (char*)"left", (char*)"f4", (char*)"right", (char*)"f5"};
        int argc = sizeof(argv) / sizeof(char*);

        EXPECT_TRUE(args.parseArguments(gsl::make_span(argv, argc)));
        ASSERT_EQ(args.buttons.size(), 2);
        EXPECT_EQ(args.buttons[0], MouseButton::LEFT);
        EXPECT_EQ(args.buttons[1], MouseButton::RIGHT);
        ASSERT_EQ(args.startKeys.size(), 2);
        EXPECT_EQ(args.startKeys[0], "f4");
        EXPECT_EQ(args.startKeys[1], "f5");
    }

    TEST_F(PositionalArgumentsTest, ParsesPairedActionAndStartKeyDefaultTarget)
    {
        char* argv[] = {(char*)"autoinput", (char*)"click", (char*)"f4", (char*)"hold", (char*)"f5"};
        int argc = sizeof(argv) / sizeof(char*);

        EXPECT_TRUE(args.parseArguments(gsl::make_span(argv, argc)));
        ASSERT_EQ(args.buttons.size(), 2);
        EXPECT_EQ(args.buttons[0], MouseButton::LEFT);
        EXPECT_EQ(args.buttons[1], MouseButton::LEFT);
        ASSERT_EQ(args.targetActions.size(), 2);
        EXPECT_EQ(args.targetActions[0], ActionState::CLICK);
        EXPECT_EQ(args.targetActions[1], ActionState::HOLD);
        ASSERT_EQ(args.startKeys.size(), 2);
        EXPECT_EQ(args.startKeys[0], "f4");
        EXPECT_EQ(args.startKeys[1], "f5");
    }

    TEST_F(PositionalArgumentsTest, ParsesMixedPairedSyntax)
    {
        char* argv[] = {(char*)"autoinput", (char*)"click", (char*)"left", (char*)"f4", (char*)"hold", (char*)"right", (char*)"f5"};
        int argc = sizeof(argv) / sizeof(char*);

        EXPECT_TRUE(args.parseArguments(gsl::make_span(argv, argc)));
        ASSERT_EQ(args.buttons.size(), 2);
        EXPECT_EQ(args.buttons[0], MouseButton::LEFT);
        EXPECT_EQ(args.buttons[1], MouseButton::RIGHT);
        ASSERT_EQ(args.targetActions.size(), 2);
        EXPECT_EQ(args.targetActions[0], ActionState::CLICK);
        EXPECT_EQ(args.targetActions[1], ActionState::HOLD);
        ASSERT_EQ(args.startKeys.size(), 2);
        EXPECT_EQ(args.startKeys[0], "f4");
        EXPECT_EQ(args.startKeys[1], "f5");
    }
}

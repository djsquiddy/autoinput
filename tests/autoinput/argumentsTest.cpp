/**
 * @file argumentsTest.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include <gtest/gtest.h>

#include "autoinput/arguments.h"
#include "autoinput/cli/commandBase.h"

namespace autoinput
{
    TEST(WaitDelayDataTest, HasCorrectDefaultValues)
    {
        WaitDelayData delayData;

        EXPECT_EQ(delayData.minWaitPressDelay, std::chrono::milliseconds{ 100 });
        EXPECT_EQ(delayData.maxWaitPressDelay, std::chrono::milliseconds{ 100 });
        EXPECT_EQ(delayData.minWaitReleaseDelay, std::chrono::milliseconds{ 100 });
        EXPECT_EQ(delayData.maxWaitReleaseDelay, std::chrono::milliseconds{ 100 });
        EXPECT_FALSE(delayData.hasPress);
        EXPECT_FALSE(delayData.hasRelease);
    }

    TEST(WaitDelayDataTest, RejectsEmptyWaitTime)
    {
        WaitDelayData delayData;

        EXPECT_FALSE(delayData.parseWaitTimeDelay("", true));
    }

    TEST(WaitDelayDataTest, ParsesPressMillisecondsWithoutSuffix)
    {
        WaitDelayData delayData;

        EXPECT_TRUE(delayData.parseWaitTimeDelay("250", true));

        EXPECT_TRUE(delayData.hasPress);
        EXPECT_FALSE(delayData.usePressRange);
        EXPECT_EQ(delayData.minWaitPressDelay, std::chrono::milliseconds{ 250 });
        EXPECT_EQ(delayData.maxWaitPressDelay, std::chrono::milliseconds{ 250 });
        EXPECT_EQ(delayData.getPressDelay(), std::chrono::milliseconds{ 250 });
    }

    TEST(WaitDelayDataTest, ParsesReleaseMillisecondsWithoutSuffix)
    {
        WaitDelayData delayData;

        EXPECT_TRUE(delayData.parseWaitTimeDelay("125", false));

        EXPECT_TRUE(delayData.hasRelease);
        EXPECT_FALSE(delayData.useReleaseRange);
        EXPECT_EQ(delayData.minWaitReleaseDelay, std::chrono::milliseconds{ 125 });
        EXPECT_EQ(delayData.maxWaitReleaseDelay, std::chrono::milliseconds{ 125 });
        EXPECT_EQ(delayData.getReleaseDelay(), std::chrono::milliseconds{ 125 });
    }

    TEST(WaitDelayDataTest, ParsesSeconds)
    {
        WaitDelayData delayData;

        EXPECT_TRUE(delayData.parseWaitTimeDelay("2s", true));

        EXPECT_EQ(delayData.minWaitPressDelay, std::chrono::seconds{ 2 });
        EXPECT_EQ(delayData.maxWaitPressDelay, std::chrono::seconds{ 2 });
    }

    TEST(WaitDelayDataTest, ParsesMinutes)
    {
        WaitDelayData delayData;

        EXPECT_TRUE(delayData.parseWaitTimeDelay("3m", true));

        EXPECT_EQ(delayData.minWaitPressDelay, std::chrono::minutes{ 3 });
        EXPECT_EQ(delayData.maxWaitPressDelay, std::chrono::minutes{ 3 });
    }

    TEST(WaitDelayDataTest, ParsesMillisecondSuffix)
    {
        WaitDelayData delayData;

        EXPECT_TRUE(delayData.parseWaitTimeDelay("500ms", true));

        EXPECT_EQ(delayData.minWaitPressDelay, std::chrono::milliseconds{ 500 });
        EXPECT_EQ(delayData.maxWaitPressDelay, std::chrono::milliseconds{ 500 });
    }

    TEST(WaitDelayDataTest, ParsesPressRange)
    {
        WaitDelayData delayData;

        EXPECT_TRUE(delayData.parseWaitTimeDelay("100ms..250ms", true));

        EXPECT_TRUE(delayData.hasPress);
        EXPECT_TRUE(delayData.usePressRange);
        EXPECT_EQ(delayData.minWaitPressDelay, std::chrono::milliseconds{ 100 });
        EXPECT_EQ(delayData.maxWaitPressDelay, std::chrono::milliseconds{ 250 });

        const std::chrono::milliseconds delay = delayData.getPressDelay();
        EXPECT_GE(delay, std::chrono::milliseconds{ 100 });
        EXPECT_LE(delay, std::chrono::milliseconds{ 250 });
    }

    TEST(WaitDelayDataTest, ParsesReleaseRange)
    {
        WaitDelayData delayData;

        EXPECT_TRUE(delayData.parseWaitTimeDelay("1s..2s", false));

        EXPECT_TRUE(delayData.hasRelease);
        EXPECT_TRUE(delayData.useReleaseRange);
        EXPECT_EQ(delayData.minWaitReleaseDelay, std::chrono::seconds{ 1 });
        EXPECT_EQ(delayData.maxWaitReleaseDelay, std::chrono::seconds{ 2 });

        const std::chrono::milliseconds delay = delayData.getReleaseDelay();
        EXPECT_GE(delay, std::chrono::seconds{ 1 });
        EXPECT_LE(delay, std::chrono::seconds{ 2 });
    }

    TEST(CliSafeGetNextArgumentTest, ReturnsNextValue)
    {
        char program[] = "autoinput";
        char command[] = "run";
        char option[] = "--button";
        char value[] = "left";
        char* argv[] = { program, command, option, value };

        EXPECT_EQ(cli::safeGetNextArgument(3, gsl::make_span(argv)), "left");
    }

    TEST(CliSafeGetNextArgumentTest, ReturnsEmptyWhenOutOfBounds)
    {
        char program[] = "autoinput";
        char* argv[] = { program };

        EXPECT_TRUE(cli::safeGetNextArgument(1, gsl::make_span(argv)).empty());
    }

    TEST(CliSafeGetNextArgumentTest, ReturnsEmptyForOption)
    {
        char program[] = "autoinput";
        char option[] = "--button";
        char nextOption[] = "--type";
        char* argv[] = { program, option, nextOption };

        EXPECT_TRUE(cli::safeGetNextArgument(2, gsl::make_span(argv)).empty());
    }

    TEST(ProgramArgumentsTest, PostParseArgumentsDefaultsToLeftButton)
    {
        ProgramArguments arguments;
        arguments.startKeys.emplace_back("f2");
        arguments.endKey = "f3";

        EXPECT_TRUE(arguments.postParseArguments());

        ASSERT_EQ(arguments.buttons.size(), 1);
        EXPECT_EQ(arguments.buttons.front().button, MouseButton::Left);
    }

    TEST(ProgramArgumentsTest, PostParseArgumentsDefaultsToStartAndEndKeys)
    {
        ProgramArguments arguments;
        EXPECT_TRUE(arguments.postParseArguments());

        ASSERT_EQ(arguments.startKeys.size(), 1);
        EXPECT_EQ(arguments.startKeys.front(), "f2");
        EXPECT_EQ(arguments.endKey, "f3");
    }

    TEST(ProgramArgumentsTest, PostParseArgumentsResizesStartKeysToButtonCount)
    {
        ProgramArguments arguments;
        arguments.buttons.emplace_back(Mouse(MouseButton::Left));
        arguments.buttons.emplace_back(Mouse(MouseButton::Right));
        arguments.startKeys.emplace_back("f2");
        arguments.endKey = "f3";

        EXPECT_TRUE(arguments.postParseArguments());

        EXPECT_EQ(arguments.startKeys.size(), arguments.buttons.size());
    }

    TEST(ProgramArgumentsTest, PostParseArgumentsDoesNotDefaultToLeftButtonIfKeyIsProvided)
    {
        ProgramArguments arguments;
        arguments.keys.emplace_back(Key{ .character = "a" });
        arguments.startKeys.emplace_back("f2");
        arguments.endKey = "f3";

        EXPECT_TRUE(arguments.postParseArguments());

        EXPECT_TRUE(arguments.buttons.empty());
        ASSERT_EQ(arguments.keys.size(), 1);
        EXPECT_EQ(arguments.keys.front().character, "a");
    }

    TEST(ProgramArgumentsTest, PostParseArgumentsResizesStartKeysToTargetCount)
    {
        ProgramArguments arguments;
        arguments.buttons.emplace_back(Mouse(MouseButton::Left));
        arguments.keys.emplace_back(Key{ .character = "a" });
        arguments.startKeys.emplace_back("f2");
        arguments.endKey = "f3";

        EXPECT_TRUE(arguments.postParseArguments());

        EXPECT_EQ(arguments.startKeys.size(), 2);
        EXPECT_EQ(arguments.startKeys[0], "f2");
        EXPECT_EQ(arguments.startKeys[1], "f2");
    }

    TEST(ProgramArgumentsTest, IsMovable)
    {
        ProgramArguments args;
        args.programName = "test_program";
        args.endKey = "f10";
        args.commandNames = {"cmd1", "cmd2"};
        
        ProgramArguments moved = std::move(args);
        
        EXPECT_EQ(moved.programName, "test_program");
        EXPECT_EQ(moved.endKey, "f10");
        EXPECT_EQ(moved.commandNames.size(), 2);
        EXPECT_EQ(moved.commandNames[0], "cmd1");
        
        // Original should be empty
        EXPECT_TRUE(args.programName.empty());
        EXPECT_TRUE(args.commandNames.empty());
    }
}
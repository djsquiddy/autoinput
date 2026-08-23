/**
 * @file argumentsTest.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include <gtest/gtest.h>

#include "autoinput/cli/arguments.h"
#include "autoinput/cli/commandBase.h"

namespace autoinput
{
    TEST(WaitDelayDataTest, HasCorrectDefaultValues)
    {
        WaitDelayData delayData;

        // Verify default press and release delay values are 100ms
        EXPECT_EQ(delayData.minWaitPressDelay, std::chrono::milliseconds{ 100 });
        EXPECT_EQ(delayData.maxWaitPressDelay, std::chrono::milliseconds{ 100 });
        EXPECT_EQ(delayData.minWaitReleaseDelay, std::chrono::milliseconds{ 100 });
        EXPECT_EQ(delayData.maxWaitReleaseDelay, std::chrono::milliseconds{ 100 });
        // Verify press and release flags are initially false
        EXPECT_FALSE(delayData.hasPress);
        EXPECT_FALSE(delayData.hasRelease);
    }

    TEST(WaitDelayDataTest, RejectsEmptyWaitTime)
    {
        WaitDelayData delayData;

        // Verify empty wait string is rejected by parser
        EXPECT_FALSE(delayData.parseWaitTimeDelay("", true));
    }

    TEST(WaitDelayDataTest, ParsesPressMillisecondsWithoutSuffix)
    {
        WaitDelayData delayData;

        // Verify numeric string parses as milliseconds
        EXPECT_TRUE(delayData.parseWaitTimeDelay("250", true));

        // Verify parsed state for single value press delay
        EXPECT_TRUE(delayData.hasPress);
        EXPECT_FALSE(delayData.usePressRange);
        EXPECT_EQ(delayData.minWaitPressDelay, std::chrono::milliseconds{ 250 });
        EXPECT_EQ(delayData.maxWaitPressDelay, std::chrono::milliseconds{ 250 });
        EXPECT_EQ(delayData.getPressDelay(), std::chrono::milliseconds{ 250 });
    }

    TEST(WaitDelayDataTest, ParsesReleaseMillisecondsWithoutSuffix)
    {
        WaitDelayData delayData;

        // Verify numeric string parses as release milliseconds
        EXPECT_TRUE(delayData.parseWaitTimeDelay("125", false));

        // Verify parsed state for single value release delay
        EXPECT_TRUE(delayData.hasRelease);
        EXPECT_FALSE(delayData.useReleaseRange);
        EXPECT_EQ(delayData.minWaitReleaseDelay, std::chrono::milliseconds{ 125 });
        EXPECT_EQ(delayData.maxWaitReleaseDelay, std::chrono::milliseconds{ 125 });
        EXPECT_EQ(delayData.getReleaseDelay(), std::chrono::milliseconds{ 125 });
    }

    TEST(WaitDelayDataTest, ParsesSeconds)
    {
        WaitDelayData delayData;

        // Verify seconds suffix 's' parses correctly
        EXPECT_TRUE(delayData.parseWaitTimeDelay("2s", true));

        // Verify parsed press delay in seconds
        EXPECT_EQ(delayData.minWaitPressDelay, std::chrono::seconds{ 2 });
        EXPECT_EQ(delayData.maxWaitPressDelay, std::chrono::seconds{ 2 });
    }

    TEST(WaitDelayDataTest, ParsesMinutes)
    {
        WaitDelayData delayData;

        // Verify minutes suffix 'm' parses correctly
        EXPECT_TRUE(delayData.parseWaitTimeDelay("3m", true));

        // Verify parsed press delay in minutes
        EXPECT_EQ(delayData.minWaitPressDelay, std::chrono::minutes{ 3 });
        EXPECT_EQ(delayData.maxWaitPressDelay, std::chrono::minutes{ 3 });
    }

    TEST(WaitDelayDataTest, ParsesMillisecondSuffix)
    {
        WaitDelayData delayData;

        // Verify explicit 'ms' suffix parses correctly
        EXPECT_TRUE(delayData.parseWaitTimeDelay("500ms", true));

        // Verify parsed press delay with ms suffix
        EXPECT_EQ(delayData.minWaitPressDelay, std::chrono::milliseconds{ 500 });
        EXPECT_EQ(delayData.maxWaitPressDelay, std::chrono::milliseconds{ 500 });
    }

    TEST(WaitDelayDataTest, ParsesPressRange)
    {
        WaitDelayData delayData;

        // Verify range syntax '100ms..250ms' parses for press delay
        EXPECT_TRUE(delayData.parseWaitTimeDelay("100ms..250ms", true));

        // Verify range flags and min/max boundaries
        EXPECT_TRUE(delayData.hasPress);
        EXPECT_TRUE(delayData.usePressRange);
        EXPECT_EQ(delayData.minWaitPressDelay, std::chrono::milliseconds{ 100 });
        EXPECT_EQ(delayData.maxWaitPressDelay, std::chrono::milliseconds{ 250 });

        // Verify sampled random delay falls within the parsed range
        const std::chrono::milliseconds delay = delayData.getPressDelay();
        EXPECT_GE(delay, std::chrono::milliseconds{ 100 });
        EXPECT_LE(delay, std::chrono::milliseconds{ 250 });
    }

    TEST(WaitDelayDataTest, ParsesReleaseRange)
    {
        WaitDelayData delayData;

        // Verify range syntax '1s..2s' parses for release delay
        EXPECT_TRUE(delayData.parseWaitTimeDelay("1s..2s", false));

        // Verify release range flags and min/max boundaries
        EXPECT_TRUE(delayData.hasRelease);
        EXPECT_TRUE(delayData.useReleaseRange);
        EXPECT_EQ(delayData.minWaitReleaseDelay, std::chrono::seconds{ 1 });
        EXPECT_EQ(delayData.maxWaitReleaseDelay, std::chrono::seconds{ 2 });

        // Verify sampled release delay falls within the parsed range
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

        // Verify next positional argument value after option is retrieved
        EXPECT_EQ(cli::safeGetNextArgument(3, gsl::make_span(argv)), "left");
    }

    TEST(CliSafeGetNextArgumentTest, ReturnsEmptyWhenOutOfBounds)
    {
        char program[] = "autoinput";
        char* argv[] = { program };

        // Verify out of bounds index returns empty string view
        EXPECT_TRUE(cli::safeGetNextArgument(1, gsl::make_span(argv)).empty());
    }

    TEST(CliSafeGetNextArgumentTest, ReturnsEmptyForOption)
    {
        char program[] = "autoinput";
        char option[] = "--button";
        char nextOption[] = "--type";
        char* argv[] = { program, option, nextOption };

        // Verify encountering next option flag instead of value returns empty
        EXPECT_TRUE(cli::safeGetNextArgument(2, gsl::make_span(argv)).empty());
    }

    TEST(ProgramArgumentsTest, PostParseArgumentsDefaultsToLeftButton)
    {
        ProgramArguments arguments;
        arguments.startKeys.emplace_back("f2");
        arguments.endKey = "f3";

        // Verify postParseArguments succeeds with start/end keys
        EXPECT_TRUE(arguments.postParseArguments());

        // Verify default button is Left mouse button
        ASSERT_EQ(arguments.buttons.size(), 1);
        EXPECT_EQ(arguments.buttons.front().button, MouseButton::Left);
    }

    TEST(ProgramArgumentsTest, PostParseArgumentsDefaultsToStartAndEndKeys)
    {
        ProgramArguments arguments;
        // Verify default post-parse setup succeeds
        EXPECT_TRUE(arguments.postParseArguments());

        // Verify start key defaults to f2 and end key to f3
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

        // Verify postParseArguments succeeds
        EXPECT_TRUE(arguments.postParseArguments());

        // Verify startKeys count is resized to match button count
        EXPECT_EQ(arguments.startKeys.size(), arguments.buttons.size());
    }

    TEST(ProgramArgumentsTest, PostParseArgumentsDoesNotDefaultToLeftButtonIfKeyIsProvided)
    {
        ProgramArguments arguments;
        arguments.keys.emplace_back(Key{ .character = "a" });
        arguments.startKeys.emplace_back("f2");
        arguments.endKey = "f3";

        // Verify postParseArguments succeeds when key action is specified
        EXPECT_TRUE(arguments.postParseArguments());

        // Verify mouse buttons remain empty and key action is preserved
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

        // Verify postParseArguments succeeds with mixed mouse and key actions
        EXPECT_TRUE(arguments.postParseArguments());

        // Verify start keys array expanded for both action targets
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
        
        // Verify moved-to object contains transferred values
        EXPECT_EQ(moved.programName, "test_program");
        EXPECT_EQ(moved.endKey, "f10");
        EXPECT_EQ(moved.commandNames.size(), 2);
        EXPECT_EQ(moved.commandNames[0], "cmd1");
        
        // Verify moved-from object has empty string and container fields
        EXPECT_TRUE(args.programName.empty());
        EXPECT_TRUE(args.commandNames.empty());
    }
}
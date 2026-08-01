/**
 * @file argumentsTest.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include <gtest/gtest.h>

#include "autoinput/arguments.h"

namespace autoinput
{
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

    TEST(ProgramArgumentsTest, SafeGetNextArgumentReturnsNextValue)
    {
        char program[] = "autoinput";
        char option[] = "-s";
        char value[] = "f2";
        char* argv[] = { program, option, value };

        EXPECT_EQ(ProgramArguments::safeGetNextArgument(2, gsl::make_span(argv)), "f2");
    }

    TEST(ProgramArgumentsTest, SafeGetNextArgumentReturnsEmptyWhenOutOfBounds)
    {
        char program[] = "autoinput";
        char* argv[] = { program };

        EXPECT_TRUE(ProgramArguments::safeGetNextArgument(1, gsl::make_span(argv)).empty());
    }

    TEST(ProgramArgumentsTest, SafeGetNextArgumentReturnsEmptyForOption)
    {
        char program[] = "autoinput";
        char option[] = "-s";
        char nextOption[] = "-e";
        char* argv[] = { program, option, nextOption };

        EXPECT_TRUE(ProgramArguments::safeGetNextArgument(2, gsl::make_span(argv)).empty());
    }

    TEST(ProgramArgumentsTest, PostParseArgumentsDefaultsToLeftButton)
    {
        ProgramArguments arguments;
        arguments.startKeys.emplace_back("f2");
        arguments.endKey = "f3";

        EXPECT_TRUE(arguments.postParseArguments());

        ASSERT_EQ(arguments.buttons.size(), 1);
        EXPECT_EQ(arguments.buttons.front().button, MouseButton::LEFT);
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
        arguments.buttons.emplace_back(Mouse(MouseButton::LEFT));
        arguments.buttons.emplace_back(Mouse(MouseButton::RIGHT));
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
        arguments.buttons.emplace_back(Mouse(MouseButton::LEFT));
        arguments.keys.emplace_back(Key{ .character = "a" });
        arguments.startKeys.emplace_back("f2");
        arguments.endKey = "f3";

        EXPECT_TRUE(arguments.postParseArguments());

        EXPECT_EQ(arguments.startKeys.size(), 2);
        EXPECT_EQ(arguments.startKeys[0], "f2");
        EXPECT_EQ(arguments.startKeys[1], "f2");
    }
    TEST(ProgramArgumentsTest, ParseArgumentsCorrectlyParsesHoldAction)
    {
        ProgramArguments arguments;
        char program[] = "autoinput";
        char typeOpt[] = "-t";
        char typeVal[] = "hold";
        char buttonOpt[] = "-b";
        char buttonVal[] = "left";
        char startOpt[] = "-s";
        char startVal[] = "f2";
        char endOpt[] = "-e";
        char endVal[] = "f3";
        char* argv[] = { program, typeOpt, typeVal, buttonOpt, buttonVal, startOpt, startVal, endOpt, endVal };

        EXPECT_TRUE(arguments.parseArguments(gsl::make_span(argv), false));
        EXPECT_EQ(arguments.actionState, ActionState::HOLD);
        ASSERT_EQ(arguments.buttons.size(), 1);
        EXPECT_EQ(arguments.buttons.front().button, MouseButton::LEFT);
        ASSERT_EQ(arguments.startKeys.size(), 1);
        EXPECT_EQ(arguments.startKeys.front(), "f2");
        EXPECT_EQ(arguments.endKey, "f3");
    }

    TEST(ProgramArgumentsTest, ParseArgumentsCorrectlyParsesApplicationName)
    {
        ProgramArguments arguments;
        char program[] = "autoinput";
        char appOpt[] = "--app";
        char appVal[] = "notepad.exe";
        char* argv[] = { program, appOpt, appVal };

        EXPECT_TRUE(arguments.parseArguments(gsl::make_span(argv), false));
        EXPECT_EQ(arguments.applicationName, "notepad.exe");
    }

    TEST(ProgramArgumentsTest, ParseArgumentsCorrectlyParsesListApplications)
    {
        ProgramArguments arguments;
        char program[] = "autoinput";
        char listOpt[] = "--list-apps";
        char* argv[] = { program, listOpt };

        EXPECT_TRUE(arguments.parseArguments(gsl::make_span(argv), false));
        EXPECT_TRUE(arguments.listApplications);
    }

    TEST(ProgramArgumentsTest, ParseArgumentsCorrectlyParsesShortListApplications)
    {
        ProgramArguments arguments;
        char program[] = "autoinput";
        char listOpt[] = "-L";
        char* argv[] = { program, listOpt };

        EXPECT_TRUE(arguments.parseArguments(gsl::make_span(argv), false));
        EXPECT_TRUE(arguments.listApplications);
    }

    TEST(ProgramArgumentsTest, ParseArgumentsHandlesMissingLogArgument)
    {
        ProgramArguments arguments;
        char program[] = "autoinput";
        char logOpt[] = "-l";
        char* argv[] = { program, logOpt };

        EXPECT_FALSE(arguments.parseArguments(gsl::make_span(argv)));
    }
    TEST(ProgramArgumentsTest, ParsesBlacklistArguments)
    {
        char program[] = "autoinput";
        char blacklistOption[] = "--blacklist";
        char blacklistValue[] = "game.exe";
        char shortBlacklistOption[] = "-B";
        char shortBlacklistValue[] = "other.exe";
        std::vector<char*> args = { program, blacklistOption, blacklistValue, shortBlacklistOption, shortBlacklistValue };

        ProgramArguments programArguments;
        EXPECT_TRUE(programArguments.parseArguments(args, false));

        EXPECT_EQ(programArguments.blacklist.size(), 2);
        EXPECT_EQ(programArguments.blacklist[0], "game.exe");
        EXPECT_EQ(programArguments.blacklist[1], "other.exe");
    }
    
    TEST(ProgramArgumentsTest, ParseArgumentsCorrectlyParsesListConfigs)
    {
        ProgramArguments arguments;
        char program[] = "autoinput";
        char listOpt[] = "--list-configs";
        char* argv[] = { program, listOpt };

        EXPECT_TRUE(arguments.parseArguments(gsl::make_span(argv), false));
        EXPECT_TRUE(arguments.listConfigs);
    }

    TEST(ProgramArgumentsTest, ParseArgumentsCorrectlyParsesShortListConfigs)
    {
        ProgramArguments arguments;
        char program[] = "autoinput";
        char listOpt[] = "-C";
        char* argv[] = { program, listOpt };

        EXPECT_TRUE(arguments.parseArguments(gsl::make_span(argv), false));
        EXPECT_TRUE(arguments.listConfigs);
    }

}
/**
 * @file typesTest.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include <gtest/gtest.h>

#include "autoinput/types.h"

namespace autoinput
{

    TEST(ActionStateFromArgumentsTest, ParsesClickAliases)
    {
        EXPECT_EQ(actionStateFromArguments("c"), ActionState::CLICK);
        EXPECT_EQ(actionStateFromArguments("click"), ActionState::CLICK);
    }

    TEST(ActionStateFromArgumentsTest, ParsesHoldAliases)
    {
        EXPECT_EQ(actionStateFromArguments("h"), ActionState::HOLD);
        EXPECT_EQ(actionStateFromArguments("hold"), ActionState::HOLD);
    }

    TEST(ActionStateFromArgumentsTest, ReturnsInvalidForUnknownInput)
    {
        EXPECT_EQ(actionStateFromArguments(""), ActionState::INVALID);
        EXPECT_EQ(actionStateFromArguments("invalid"), ActionState::INVALID);
        EXPECT_EQ(actionStateFromArguments("press"), ActionState::INVALID);
    }

    TEST(ParseStringToIntTest, ParsesValidIntegers)
    {
        EXPECT_EQ(parseStringToInt("0"), 0);
        EXPECT_EQ(parseStringToInt("123"), 123);
        EXPECT_EQ(parseStringToInt("-42"), -42);
    }

    TEST(ParseStringToIntTest, ParsesPrefixBeforeTrailingCharacters)
    {
        EXPECT_EQ(parseStringToInt("123abc"), 123);
    }

    TEST(ParseStringToIntTest, ReturnsZeroForInvalidInput)
    {
        EXPECT_EQ(parseStringToInt(""), 0);
        EXPECT_EQ(parseStringToInt("abc"), 0);
    }

    TEST(ConsoleColorTest, ReturnsAnsiEscapeCodes)
    {
        EXPECT_EQ(getConsoleColor(ConsoleColor::Bold), "\033[1m");
        EXPECT_EQ(getConsoleColor(ConsoleColor::White), "\033[37m");
        EXPECT_EQ(getConsoleColor(ConsoleColor::Red), "\033[31m");
        EXPECT_EQ(getConsoleColor(ConsoleColor::Green), "\033[32m");
        EXPECT_EQ(getConsoleColor(ConsoleColor::Yellow), "\033[33m");
        EXPECT_EQ(getConsoleColor(ConsoleColor::Blue), "\033[34m");
        EXPECT_EQ(getConsoleColor(ConsoleColor::Magenta), "\033[35m");
        EXPECT_EQ(getConsoleColor(ConsoleColor::Cyan), "\033[36m");
        EXPECT_EQ(getConsoleColor(ConsoleColor::Reset), "\033[0m");
    }

    TEST(FormatterTest, FormatsMouseButton)
    {
        EXPECT_EQ(std::format("{}", MouseButton::LEFT), "left");
        EXPECT_EQ(std::format("{}", MouseButton::NONE), "");
    }

    TEST(FormatterTest, FormatsKey)
    {
        const Key key{
            .character = "a",
            .modifier = KeyModifier::Ctrl
        };

        EXPECT_EQ(std::format("{}", key), "ctrl+a");
    }

    TEST(FormatterTest, FormatsConsoleColor)
    {
        EXPECT_EQ(std::format("{}", ConsoleColor::Red), "\033[31m");
    }

    TEST(FormatterTest, FormatsQuotedString)
    {
        EXPECT_EQ(std::format("{}", Quoted{ "hello" }), "\"hello\"");
        EXPECT_EQ(std::format("{}", Quoted{ "hello \"world\"" }), "\"hello \\\"world\\\"\"");
    }

    TEST(BoldTest, WritesAnsiBoldText)
    {
        const std::string text = "important";
        std::ostringstream stream;

        stream << bold{ text };

        EXPECT_EQ(stream.str(), "\033[1mimportant\033[0m");
    }

}
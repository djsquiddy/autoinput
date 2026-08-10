/**
 * @file typesTest.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include <gtest/gtest.h>

#include "autoinput/support/types.h"
#include "autoinput/platform/terminal.h"

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

    TEST(TerminalColorTest, ReturnsAnsiEscapeCodes)
    {
        EXPECT_EQ(terminal::colorToAnsi(terminal::Color::Bold), "\033[1m");
        EXPECT_EQ(terminal::colorToAnsi(terminal::Color::White), "\033[37m");
        EXPECT_EQ(terminal::colorToAnsi(terminal::Color::Red), "\033[31m");
        EXPECT_EQ(terminal::colorToAnsi(terminal::Color::Green), "\033[32m");
        EXPECT_EQ(terminal::colorToAnsi(terminal::Color::Yellow), "\033[33m");
        EXPECT_EQ(terminal::colorToAnsi(terminal::Color::Blue), "\033[34m");
        EXPECT_EQ(terminal::colorToAnsi(terminal::Color::Magenta), "\033[35m");
        EXPECT_EQ(terminal::colorToAnsi(terminal::Color::Cyan), "\033[36m");
        EXPECT_EQ(terminal::colorToAnsi(terminal::Color::Gray), "\033[90m");
        EXPECT_EQ(terminal::colorToAnsi(terminal::Color::Reset), "\033[0m");
    }

    TEST(FormatterTest, FormatsMouseButton)
    {
        EXPECT_EQ(std::format("{}", MouseButton::Left), "left");
        EXPECT_EQ(std::format("{}", MouseButton::None), "");
    }

    TEST(FormatterTest, FormatsKey)
    {
        const Key key{
            .character = "a",
            .modifier = KeyModifier::Ctrl
        };

        EXPECT_EQ(std::format("{}", key), "ctrl+a");
    }

    TEST(FormatterTest, FormatsTerminalColor)
    {
        EXPECT_EQ(std::format("{}", terminal::Color::Red), "\033[31m");
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

        stream << terminal::bold{ text };

        EXPECT_EQ(stream.str(), "\033[1mimportant\033[0m");
    }

}
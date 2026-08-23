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
        // Verify short alias 'c' resolves to ActionState::CLICK
        EXPECT_EQ(actionStateFromArguments("c"), ActionState::CLICK);
        // Verify full name 'click' resolves to ActionState::CLICK
        EXPECT_EQ(actionStateFromArguments("click"), ActionState::CLICK);
    }

    TEST(ActionStateFromArgumentsTest, ParsesHoldAliases)
    {
        // Verify short alias 'h' resolves to ActionState::HOLD
        EXPECT_EQ(actionStateFromArguments("h"), ActionState::HOLD);
        // Verify full name 'hold' resolves to ActionState::HOLD
        EXPECT_EQ(actionStateFromArguments("hold"), ActionState::HOLD);
    }

    TEST(ActionStateFromArgumentsTest, ReturnsInvalidForUnknownInput)
    {
        // Verify empty input string produces ActionState::INVALID
        EXPECT_EQ(actionStateFromArguments(""), ActionState::INVALID);
        // Verify unrecognized string produces ActionState::INVALID
        EXPECT_EQ(actionStateFromArguments("invalid"), ActionState::INVALID);
        // Verify unsupported action name produces ActionState::INVALID
        EXPECT_EQ(actionStateFromArguments("press"), ActionState::INVALID);
    }

    TEST(ParseStringToIntTest, ParsesValidIntegers)
    {
        // Verify parsing zero string returns 0
        EXPECT_EQ(parseStringToInt("0"), 0);
        // Verify parsing positive integer string returns corresponding value
        EXPECT_EQ(parseStringToInt("123"), 123);
        // Verify parsing negative integer string returns corresponding negative value
        EXPECT_EQ(parseStringToInt("-42"), -42);
    }

    TEST(ParseStringToIntTest, ParsesPrefixBeforeTrailingCharacters)
    {
        // Verify numeric prefix is parsed when followed by trailing non-digit characters
        EXPECT_EQ(parseStringToInt("123abc"), 123);
    }

    TEST(ParseStringToIntTest, ReturnsZeroForInvalidInput)
    {
        // Verify empty string input returns 0
        EXPECT_EQ(parseStringToInt(""), 0);
        // Verify non-numeric string input returns 0
        EXPECT_EQ(parseStringToInt("abc"), 0);
    }

    TEST(TerminalColorTest, ReturnsAnsiEscapeCodes)
    {
        // Verify ANSI code for bold styling
        EXPECT_EQ(terminal::colorToAnsi(terminal::Color::Bold), "\033[1m");
        // Verify ANSI code for white text
        EXPECT_EQ(terminal::colorToAnsi(terminal::Color::White), "\033[37m");
        // Verify ANSI code for red text
        EXPECT_EQ(terminal::colorToAnsi(terminal::Color::Red), "\033[31m");
        // Verify ANSI code for green text
        EXPECT_EQ(terminal::colorToAnsi(terminal::Color::Green), "\033[32m");
        // Verify ANSI code for yellow text
        EXPECT_EQ(terminal::colorToAnsi(terminal::Color::Yellow), "\033[33m");
        // Verify ANSI code for blue text
        EXPECT_EQ(terminal::colorToAnsi(terminal::Color::Blue), "\033[34m");
        // Verify ANSI code for magenta text
        EXPECT_EQ(terminal::colorToAnsi(terminal::Color::Magenta), "\033[35m");
        // Verify ANSI code for cyan text
        EXPECT_EQ(terminal::colorToAnsi(terminal::Color::Cyan), "\033[36m");
        // Verify ANSI code for gray text
        EXPECT_EQ(terminal::colorToAnsi(terminal::Color::Gray), "\033[90m");
        // Verify ANSI code for text attribute reset
        EXPECT_EQ(terminal::colorToAnsi(terminal::Color::Reset), "\033[0m");
    }

    TEST(FormatterTest, FormatsMouseButton)
    {
        // Verify std::format formats Left mouse button to lowercase string name
        EXPECT_EQ(std::format("{}", MouseButton::Left), "left");
        // Verify std::format formats None mouse button to empty string
        EXPECT_EQ(std::format("{}", MouseButton::None), "");
    }

    TEST(FormatterTest, FormatsKey)
    {
        const Key key{
            .character = "a",
            .modifier = KeyModifier::Ctrl
        };

        // Verify std::format formats Key struct with modifier and character
        EXPECT_EQ(std::format("{}", key), "ctrl+a");
    }

    TEST(FormatterTest, FormatsTerminalColor)
    {
        // Verify std::format formats TerminalColor to corresponding ANSI escape sequence
        EXPECT_EQ(std::format("{}", terminal::Color::Red), "\033[31m");
    }

    TEST(FormatterTest, FormatsQuotedString)
    {
        // Verify Quoted wrapper surrounds simple string with double quotes
        EXPECT_EQ(std::format("{}", Quoted{ "hello" }), "\"hello\"");
        // Verify Quoted wrapper escapes embedded double quotes
        EXPECT_EQ(std::format("{}", Quoted{ "hello \"world\"" }), "\"hello \\\"world\\\"\"");
    }

    TEST(BoldTest, WritesAnsiBoldText)
    {
        const std::string text = "important";
        std::ostringstream stream;

        stream << terminal::bold{ text };

        // Verify terminal::bold stream manipulator wraps text in ANSI bold and reset escape sequences
        EXPECT_EQ(stream.str(), "\033[1mimportant\033[0m");
    }

}
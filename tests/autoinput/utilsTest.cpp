/**
* @file utilsTest.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include <gtest/gtest.h>

#include "autoinput/utils.h"

namespace autoinput
{
    TEST(ToLowerCaseTest, ConvertsUppercaseLetters)
    {
        EXPECT_EQ(toLowerCase("HELLO"), "hello");
    }

    TEST(ToLowerCaseTest, LeavesLowercaseLettersUnchanged)
    {
        EXPECT_EQ(toLowerCase("hello"), "hello");
    }

    TEST(ToLowerCaseTest, ConvertsMixedCaseText)
    {
        EXPECT_EQ(toLowerCase("HeLlO WoRlD"), "hello world");
    }

    TEST(ToLowerCaseTest, PreservesNumbersAndSymbols)
    {
        EXPECT_EQ(toLowerCase("ABC123_+-"), "abc123_+-");
    }

    TEST(ToLowerCaseTest, HandlesEmptyString)
    {
        EXPECT_EQ(toLowerCase(""), "");
    }

    TEST(JoinTest, JoinsEmptyVector)
    {
        const std::vector<std::string> values;

        EXPECT_EQ(join(values, "+"), "");
    }

    TEST(JoinTest, JoinsSingleValue)
    {
        const std::vector<std::string> values{ "ctrl" };

        EXPECT_EQ(join(values, "+"), "ctrl");
    }

    TEST(JoinTest, JoinsMultipleValues)
    {
        const std::vector<std::string> values{ "ctrl", "alt", "shift" };

        EXPECT_EQ(join(values, "+"), "ctrl+alt+shift");
    }

    TEST(JoinTest, SupportsMultiCharacterDelimiter)
    {
        const std::vector<std::string> values{ "one", "two", "three" };

        EXPECT_EQ(join(values, " -> "), "one -> two -> three");
    }
}
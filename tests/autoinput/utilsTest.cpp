/**
* @file utilsTest.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include <gtest/gtest.h>

#include "autoinput/support/utils.h"

namespace autoinput
{
    TEST(ToLowerCaseTest, ConvertsUppercaseLetters)
    {
        // Verify all uppercase letters are converted to lowercase
        EXPECT_EQ(toLowerCase("HELLO"), "hello");
    }

    TEST(ToLowerCaseTest, LeavesLowercaseLettersUnchanged)
    {
        // Verify lowercase letters remain unchanged
        EXPECT_EQ(toLowerCase("hello"), "hello");
    }

    TEST(ToLowerCaseTest, ConvertsMixedCaseText)
    {
        // Verify mixed-case text is converted to lowercase
        EXPECT_EQ(toLowerCase("HeLlO WoRlD"), "hello world");
    }

    TEST(ToLowerCaseTest, PreservesNumbersAndSymbols)
    {
        // Verify digits and special symbols are preserved while uppercase letters are converted
        EXPECT_EQ(toLowerCase("ABC123_+-"), "abc123_+-");
    }

    TEST(ToLowerCaseTest, HandlesEmptyString)
    {
        // Verify empty string input returns an empty string
        EXPECT_EQ(toLowerCase(""), "");
    }

    TEST(JoinTest, JoinsEmptyVector)
    {
        const std::vector<std::string> values;

        // Verify joining an empty vector returns an empty string
        EXPECT_EQ(join(values, "+"), "");
    }

    TEST(JoinTest, JoinsSingleValue)
    {
        const std::vector<std::string> values{ "ctrl" };

        // Verify joining a single element returns the element without delimiter
        EXPECT_EQ(join(values, "+"), "ctrl");
    }

    TEST(JoinTest, JoinsMultipleValues)
    {
        const std::vector<std::string> values{ "ctrl", "alt", "shift" };

        // Verify joining multiple elements produces a delimiter-separated string
        EXPECT_EQ(join(values, "+"), "ctrl+alt+shift");
    }

    TEST(JoinTest, SupportsMultiCharacterDelimiter)
    {
        const std::vector<std::string> values{ "one", "two", "three" };

        // Verify joining elements with a multi-character delimiter
        EXPECT_EQ(join(values, " -> "), "one -> two -> three");
    }
}
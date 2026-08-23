/**
 * @file configMetadataTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include <algorithm>
#include "autoinput/config/configMetadata.h"
#include "autoinput/config/defaults.h"

namespace autoinput
{
    TEST(ConfigMetadataTest, ValidActionNamesContainsExpectedActions)
    {
        const auto actions = ConfigMetadata::validActionNames();
        // Verify valid action names include click and hold
        EXPECT_NE(std::find(actions.begin(), actions.end(), "click"), actions.end());
        EXPECT_NE(std::find(actions.begin(), actions.end(), "hold"), actions.end());
    }

    TEST(ConfigMetadataTest, ValidActionAliasesContainsExpectedAliases)
    {
        const auto actions = ConfigMetadata::validActionAliases();
        // Verify action aliases include full names and single-letter abbreviations
        EXPECT_NE(std::find(actions.begin(), actions.end(), "click"), actions.end());
        EXPECT_NE(std::find(actions.begin(), actions.end(), "c"), actions.end());
        EXPECT_NE(std::find(actions.begin(), actions.end(), "hold"), actions.end());
        EXPECT_NE(std::find(actions.begin(), actions.end(), "h"), actions.end());
    }

    TEST(ConfigMetadataTest, ValidActionChoicesReturnsFormattedString)
    {
        // Verify action choices formatted set string
        EXPECT_EQ(ConfigMetadata::validActionChoices(), "{click,c,hold,h}");
    }

    TEST(ConfigMetadataTest, ValidMouseButtonNamesContainsExpectedButtons)
    {
        const auto buttons = ConfigMetadata::validMouseButtonNames();
        // Verify supported mouse button names
        EXPECT_NE(std::find(buttons.begin(), buttons.end(), "left"), buttons.end());
        EXPECT_NE(std::find(buttons.begin(), buttons.end(), "right"), buttons.end());
        EXPECT_NE(std::find(buttons.begin(), buttons.end(), "middle"), buttons.end());
        EXPECT_NE(std::find(buttons.begin(), buttons.end(), "back"), buttons.end());
        EXPECT_NE(std::find(buttons.begin(), buttons.end(), "forward"), buttons.end());
    }

    TEST(ConfigMetadataTest, ValidMouseButtonAliasesContainsExpectedAliases)
    {
        const auto buttons = ConfigMetadata::validMouseButtonAliases();
        // Verify supported mouse button aliases
        EXPECT_NE(std::find(buttons.begin(), buttons.end(), "left"), buttons.end());
        EXPECT_NE(std::find(buttons.begin(), buttons.end(), "l"), buttons.end());
        EXPECT_NE(std::find(buttons.begin(), buttons.end(), "right"), buttons.end());
        EXPECT_NE(std::find(buttons.begin(), buttons.end(), "r"), buttons.end());
        EXPECT_NE(std::find(buttons.begin(), buttons.end(), "middle"), buttons.end());
        EXPECT_NE(std::find(buttons.begin(), buttons.end(), "m"), buttons.end());
        EXPECT_NE(std::find(buttons.begin(), buttons.end(), "back"), buttons.end());
        EXPECT_NE(std::find(buttons.begin(), buttons.end(), "forward"), buttons.end());
    }

    TEST(ConfigMetadataTest, ValidMouseButtonChoicesReturnsFormattedString)
    {
        // Verify mouse button choices formatted set string
        EXPECT_EQ(ConfigMetadata::validMouseButtonChoices(), "{left,l,right,r,middle,m,back,forward}");
    }

    TEST(ConfigMetadataTest, DefaultMouseButtonNameMatchesDefaults)
    {
        // Verify default mouse button name constant
        EXPECT_EQ(ConfigMetadata::defaultMouseButtonName(), defaults::DefaultMouseButtonName);
    }

    TEST(ConfigMetadataTest, DefaultActionNameMatchesDefaults)
    {
        // Verify default action name constant
        EXPECT_EQ(ConfigMetadata::defaultActionName(), defaults::DefaultActionName);
    }

    TEST(ConfigMetadataTest, DefaultStartKeyMatchesDefaults)
    {
        // Verify default start key constant
        EXPECT_EQ(ConfigMetadata::defaultStartKey(), defaults::StartKey);
    }

    TEST(ConfigMetadataTest, DefaultEndKeyMatchesDefaults)
    {
        // Verify default end key constant
        EXPECT_EQ(ConfigMetadata::defaultEndKey(), defaults::EndKey);
    }

    TEST(ConfigMetadataTest, ValidLogLevelNamesContainsExpectedLevels)
    {
        const auto levels = ConfigMetadata::validLogLevelNames();
        // Verify standard log level names
        EXPECT_NE(std::find(levels.begin(), levels.end(), "debug"), levels.end());
        EXPECT_NE(std::find(levels.begin(), levels.end(), "info"), levels.end());
        EXPECT_NE(std::find(levels.begin(), levels.end(), "warning"), levels.end());
        EXPECT_NE(std::find(levels.begin(), levels.end(), "error"), levels.end());
        EXPECT_NE(std::find(levels.begin(), levels.end(), "fatal"), levels.end());
    }

    TEST(ConfigMetadataTest, ValidLogLevelAliasesContainsExpectedAliases)
    {
        const auto levels = ConfigMetadata::validLogLevelAliases();
        // Verify log level aliases and short names
        EXPECT_NE(std::find(levels.begin(), levels.end(), "d"), levels.end());
        EXPECT_NE(std::find(levels.begin(), levels.end(), "debug"), levels.end());
        EXPECT_NE(std::find(levels.begin(), levels.end(), "i"), levels.end());
        EXPECT_NE(std::find(levels.begin(), levels.end(), "info"), levels.end());
        EXPECT_NE(std::find(levels.begin(), levels.end(), "w"), levels.end());
        EXPECT_NE(std::find(levels.begin(), levels.end(), "warn"), levels.end());
        EXPECT_NE(std::find(levels.begin(), levels.end(), "warning"), levels.end());
        EXPECT_NE(std::find(levels.begin(), levels.end(), "e"), levels.end());
        EXPECT_NE(std::find(levels.begin(), levels.end(), "error"), levels.end());
        EXPECT_NE(std::find(levels.begin(), levels.end(), "f"), levels.end());
        EXPECT_NE(std::find(levels.begin(), levels.end(), "fatal"), levels.end());
    }

    TEST(ConfigMetadataTest, ValidLogLevelChoicesReturnsFormattedString)
    {
        // Verify log level choices formatted set string
        EXPECT_EQ(ConfigMetadata::validLogLevelChoices(), "{d,debug,i,info,w,warn,warning,e,error,f,fatal}");
    }

    TEST(ConfigMetadataTest, ValidSpecialKeyNamesContainsExpectedKeys)
    {
        const auto keys = ConfigMetadata::validSpecialKeyNames();
        // Verify special key name constants
        EXPECT_NE(std::find(keys.begin(), keys.end(), "space"), keys.end());
        EXPECT_NE(std::find(keys.begin(), keys.end(), "enter"), keys.end());
        EXPECT_NE(std::find(keys.begin(), keys.end(), "esc"), keys.end());
    }

    TEST(ConfigMetadataTest, WildcardInputHelpersRecognizeInputs)
    {
        // Verify mouse wildcard trigger patterns
        EXPECT_TRUE(ConfigMetadata::isMouseAllTrigger("mouse.all"));
        EXPECT_TRUE(ConfigMetadata::isMouseAllTrigger("mouse.*"));
        EXPECT_TRUE(ConfigMetadata::isMouseAllTrigger("mouse.any"));
        EXPECT_FALSE(ConfigMetadata::isMouseAllTrigger("keys.all"));

        // Verify keyboard wildcard trigger patterns
        EXPECT_TRUE(ConfigMetadata::isKeysAllTrigger("keys.all"));
        EXPECT_TRUE(ConfigMetadata::isKeysAllTrigger("key.all"));
        EXPECT_TRUE(ConfigMetadata::isKeysAllTrigger("keys.*"));
        EXPECT_FALSE(ConfigMetadata::isKeysAllTrigger("mouse.all"));

        // Verify universal input wildcard trigger patterns
        EXPECT_TRUE(ConfigMetadata::isInputAllTrigger("input.all"));
        EXPECT_TRUE(ConfigMetadata::isInputAllTrigger("input.*"));
        EXPECT_TRUE(ConfigMetadata::isInputAllTrigger("all"));
        EXPECT_TRUE(ConfigMetadata::isInputAllTrigger("any"));
        EXPECT_FALSE(ConfigMetadata::isInputAllTrigger("mouse.left"));

        // Verify general wildcard trigger helper
        EXPECT_TRUE(ConfigMetadata::isWildcardTrigger("mouse.all"));
        EXPECT_TRUE(ConfigMetadata::isWildcardTrigger("keys.all"));
        EXPECT_TRUE(ConfigMetadata::isWildcardTrigger("input.all"));
        EXPECT_FALSE(ConfigMetadata::isWildcardTrigger("f2"));
    }
}

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
        EXPECT_NE(std::find(actions.begin(), actions.end(), "click"), actions.end());
        EXPECT_NE(std::find(actions.begin(), actions.end(), "hold"), actions.end());
    }

    TEST(ConfigMetadataTest, ValidActionAliasesContainsExpectedAliases)
    {
        const auto actions = ConfigMetadata::validActionAliases();
        EXPECT_NE(std::find(actions.begin(), actions.end(), "click"), actions.end());
        EXPECT_NE(std::find(actions.begin(), actions.end(), "c"), actions.end());
        EXPECT_NE(std::find(actions.begin(), actions.end(), "hold"), actions.end());
        EXPECT_NE(std::find(actions.begin(), actions.end(), "h"), actions.end());
    }

    TEST(ConfigMetadataTest, ValidActionChoicesReturnsFormattedString)
    {
        EXPECT_EQ(ConfigMetadata::validActionChoices(), "{click,c,hold,h}");
    }

    TEST(ConfigMetadataTest, ValidMouseButtonNamesContainsExpectedButtons)
    {
        const auto buttons = ConfigMetadata::validMouseButtonNames();
        EXPECT_NE(std::find(buttons.begin(), buttons.end(), "left"), buttons.end());
        EXPECT_NE(std::find(buttons.begin(), buttons.end(), "right"), buttons.end());
        EXPECT_NE(std::find(buttons.begin(), buttons.end(), "middle"), buttons.end());
        EXPECT_NE(std::find(buttons.begin(), buttons.end(), "back"), buttons.end());
        EXPECT_NE(std::find(buttons.begin(), buttons.end(), "forward"), buttons.end());
    }

    TEST(ConfigMetadataTest, ValidMouseButtonAliasesContainsExpectedAliases)
    {
        const auto buttons = ConfigMetadata::validMouseButtonAliases();
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
        EXPECT_EQ(ConfigMetadata::validMouseButtonChoices(), "{left,l,right,r,middle,m,back,forward}");
    }

    TEST(ConfigMetadataTest, DefaultMouseButtonNameMatchesDefaults)
    {
        EXPECT_EQ(ConfigMetadata::defaultMouseButtonName(), defaults::DefaultMouseButtonName);
    }

    TEST(ConfigMetadataTest, DefaultActionNameMatchesDefaults)
    {
        EXPECT_EQ(ConfigMetadata::defaultActionName(), defaults::DefaultActionName);
    }

    TEST(ConfigMetadataTest, DefaultStartKeyMatchesDefaults)
    {
        EXPECT_EQ(ConfigMetadata::defaultStartKey(), defaults::StartKey);
    }

    TEST(ConfigMetadataTest, DefaultEndKeyMatchesDefaults)
    {
        EXPECT_EQ(ConfigMetadata::defaultEndKey(), defaults::EndKey);
    }

    TEST(ConfigMetadataTest, ValidLogLevelNamesContainsExpectedLevels)
    {
        const auto levels = ConfigMetadata::validLogLevelNames();
        EXPECT_NE(std::find(levels.begin(), levels.end(), "debug"), levels.end());
        EXPECT_NE(std::find(levels.begin(), levels.end(), "info"), levels.end());
        EXPECT_NE(std::find(levels.begin(), levels.end(), "warning"), levels.end());
        EXPECT_NE(std::find(levels.begin(), levels.end(), "error"), levels.end());
        EXPECT_NE(std::find(levels.begin(), levels.end(), "fatal"), levels.end());
    }

    TEST(ConfigMetadataTest, ValidLogLevelAliasesContainsExpectedAliases)
    {
        const auto levels = ConfigMetadata::validLogLevelAliases();
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
        EXPECT_EQ(ConfigMetadata::validLogLevelChoices(), "{d,debug,i,info,w,warn,warning,e,error,f,fatal}");
    }

    TEST(ConfigMetadataTest, ValidSpecialKeyNamesContainsExpectedKeys)
    {
        const auto keys = ConfigMetadata::validSpecialKeyNames();
        EXPECT_NE(std::find(keys.begin(), keys.end(), "space"), keys.end());
        EXPECT_NE(std::find(keys.begin(), keys.end(), "enter"), keys.end());
        EXPECT_NE(std::find(keys.begin(), keys.end(), "esc"), keys.end());
    }
}

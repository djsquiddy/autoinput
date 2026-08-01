/**
 * @file runtimeConfigTest.cpp
 * @author djsquiddy
 * @date August 2026
 */

#include <gtest/gtest.h>
#include "autoinput/runtimeConfig.h"

namespace autoinput
{
    TEST(RuntimeConfigTest, ConvertValidConfigData)
    {
        ConfigData config;
        CommandData cmd;
        cmd.action = "click";
        cmd.buttons = { "left" };
        cmd.keys = { "a" };
        cmd.startKeys = { "f1" };
        cmd.pressWait = "100ms";
        cmd.releaseWait = "200ms";
        config.commands.push_back(cmd);
        config.endKey = "f3";
        config.application = "notepad.exe";
        config.blacklist = { "game.exe" };

        auto result = convertConfigToRuntime(config);
        ASSERT_TRUE(std::holds_alternative<RuntimeConfig>(result));

        const auto& runtimeConfig = std::get<RuntimeConfig>(result);
        ASSERT_EQ(runtimeConfig.commands.size(), 1);
        EXPECT_EQ(runtimeConfig.commands[0].action, ActionState::CLICK);
        ASSERT_EQ(runtimeConfig.commands[0].buttons.size(), 1);
        EXPECT_EQ(runtimeConfig.commands[0].buttons[0].button, MouseButton::LEFT);
        ASSERT_EQ(runtimeConfig.commands[0].keys.size(), 1);
        EXPECT_EQ(runtimeConfig.commands[0].keys[0].character, "a");
        ASSERT_EQ(runtimeConfig.commands[0].startKeys.size(), 1);
        EXPECT_EQ(runtimeConfig.commands[0].startKeys[0].character, "1");
        EXPECT_TRUE(static_cast<bool>(runtimeConfig.commands[0].startKeys[0].modifier & KeyModifier::Function));
        
        EXPECT_EQ(runtimeConfig.commands[0].wait.minWaitPressDelay.count(), 100);
        EXPECT_EQ(runtimeConfig.commands[0].wait.minWaitReleaseDelay.count(), 200);

        EXPECT_EQ(runtimeConfig.endKey.character, "3");
        EXPECT_TRUE(static_cast<bool>(runtimeConfig.endKey.modifier & KeyModifier::Function));
        EXPECT_EQ(runtimeConfig.application, "notepad.exe");
        ASSERT_EQ(runtimeConfig.blacklist.size(), 1);
        EXPECT_EQ(runtimeConfig.blacklist[0], "game.exe");
    }

    TEST(RuntimeConfigTest, ConvertInvalidAction)
    {
        CommandData cmd;
        cmd.action = "invalid_action";
        
        auto result = convertCommandToRuntime(cmd);
        ASSERT_TRUE(std::holds_alternative<ValidationError>(result));
        EXPECT_EQ(std::get<ValidationError>(result).message, "Invalid action: 'invalid_action'");
    }

    TEST(RuntimeConfigTest, ConvertInvalidMouseButton)
    {
        CommandData cmd;
        cmd.action = "click";
        cmd.buttons = { "invalid_button" };
        
        auto result = convertCommandToRuntime(cmd);
        ASSERT_TRUE(std::holds_alternative<ValidationError>(result));
        EXPECT_EQ(std::get<ValidationError>(result).message, "Invalid mouse button: 'invalid_button'");
    }

    TEST(RuntimeConfigTest, ConvertInvalidKey)
    {
        CommandData cmd;
        cmd.action = "click";
        // Key::fromString currently returns an empty character if it can't parse, 
        // but it's hard to make it fail since almost any string is a character.
        // However, we can test with empty string if the parser handles it.
        cmd.keys = { "" }; 
        
        auto result = convertCommandToRuntime(cmd);
        ASSERT_TRUE(std::holds_alternative<ValidationError>(result));
        EXPECT_EQ(std::get<ValidationError>(result).message, "Invalid key: ''");
    }

    TEST(RuntimeConfigTest, ConvertInvalidWaitDelay)
    {
        CommandData cmd;
        cmd.action = "click";
        cmd.pressWait = "abc"; // Not a valid delay
        
        auto result = convertCommandToRuntime(cmd);
        ASSERT_TRUE(std::holds_alternative<ValidationError>(result));
        EXPECT_EQ(std::get<ValidationError>(result).message, "Invalid press wait delay: 'abc'");
    }

    TEST(RuntimeConfigTest, ConvertInvalidEndKey)
    {
        ConfigData config;
        config.endKey = ""; 
        
        auto result = convertConfigToRuntime(config);
        ASSERT_TRUE(std::holds_alternative<ValidationError>(result));
        EXPECT_EQ(std::get<ValidationError>(result).message, "Invalid end key: ''");
    }
}

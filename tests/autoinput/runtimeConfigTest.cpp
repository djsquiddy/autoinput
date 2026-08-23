/**
 * @file runtimeConfigTest.cpp
 * @author djsquiddy
 * @date August 2026
 */

#include <gtest/gtest.h>
#include "autoinput/config/runtimeConfig.h"
#include "autoinput/config/config.h"

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
        // Ensure config conversion succeeded and returned a RuntimeConfig variant
        ASSERT_TRUE(std::holds_alternative<RuntimeConfig>(result));

        const auto& runtimeConfig = std::get<RuntimeConfig>(result);
        // Ensure exactly one command is present in converted runtime configuration
        ASSERT_EQ(runtimeConfig.commands.size(), 1);
        // Verify the command action is converted to CLICK
        EXPECT_EQ(runtimeConfig.commands[0].action, ActionState::CLICK);
        // Ensure exactly one button is present in the command
        ASSERT_EQ(runtimeConfig.commands[0].buttons.size(), 1);
        // Verify the button is converted to MouseButton::Left
        EXPECT_EQ(runtimeConfig.commands[0].buttons[0].button, MouseButton::Left);
        // Ensure exactly one key is present in the command
        ASSERT_EQ(runtimeConfig.commands[0].keys.size(), 1);
        // Verify the key character is 'a'
        EXPECT_EQ(runtimeConfig.commands[0].keys[0].character, "a");
        // Ensure exactly one start key is present
        ASSERT_EQ(runtimeConfig.commands[0].startKeys.size(), 1);
        // Verify the start key character is '1'
        EXPECT_EQ(runtimeConfig.commands[0].startKeys[0].character, "1");
        // Verify the start key has the Function modifier set for F1
        EXPECT_TRUE(static_cast<bool>(runtimeConfig.commands[0].startKeys[0].modifier & KeyModifier::Function));
        
        // Verify the press wait delay is parsed as 100 milliseconds
        EXPECT_EQ(runtimeConfig.commands[0].wait.minWaitPressDelay.count(), 100);
        // Verify the release wait delay is parsed as 200 milliseconds
        EXPECT_EQ(runtimeConfig.commands[0].wait.minWaitReleaseDelay.count(), 200);

        // Verify the end key character is '3'
        EXPECT_EQ(runtimeConfig.endKey.character, "3");
        // Verify the end key has the Function modifier set for F3
        EXPECT_TRUE(static_cast<bool>(runtimeConfig.endKey.modifier & KeyModifier::Function));
        // Verify the target application is preserved as notepad.exe
        EXPECT_EQ(runtimeConfig.application, "notepad.exe");
        // Ensure exactly one blacklist entry exists
        ASSERT_EQ(runtimeConfig.blacklist.size(), 1);
        // Verify the blacklist entry is game.exe
        EXPECT_EQ(runtimeConfig.blacklist[0], "game.exe");
    }

    TEST(RuntimeConfigTest, ConvertInvalidAction)
    {
        CommandData cmd;
        cmd.action = "invalid_action";
        
        auto result = convertCommandToRuntime(cmd);
        // Ensure converting a command with an invalid action results in a ValidationError
        ASSERT_TRUE(std::holds_alternative<ValidationError>(result));
        // Verify the error message specifies the invalid action name
        EXPECT_EQ(std::get<ValidationError>(result).message, "Invalid action: 'invalid_action'");
    }

    TEST(RuntimeConfigTest, ConvertInvalidMouseButton)
    {
        CommandData cmd;
        cmd.action = "click";
        cmd.buttons = { "invalid_button" };
        
        auto result = convertCommandToRuntime(cmd);
        // Ensure converting a command with an invalid mouse button results in a ValidationError
        ASSERT_TRUE(std::holds_alternative<ValidationError>(result));
        // Verify the error message specifies the invalid mouse button name
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
        // Ensure converting a command with an invalid empty key results in a ValidationError
        ASSERT_TRUE(std::holds_alternative<ValidationError>(result));
        // Verify the error message indicates the key is invalid
        EXPECT_EQ(std::get<ValidationError>(result).message, "Invalid key: ''");
    }

    TEST(RuntimeConfigTest, ConvertInvalidWaitDelay)
    {
        CommandData cmd;
        cmd.action = "click";
        cmd.pressWait = "abc"; // Not a valid delay
        
        auto result = convertCommandToRuntime(cmd);
        // Ensure converting a command with an invalid wait delay string results in a ValidationError
        ASSERT_TRUE(std::holds_alternative<ValidationError>(result));
        // Verify the error message indicates the invalid press wait delay value
        EXPECT_EQ(std::get<ValidationError>(result).message, "Invalid press wait delay: 'abc'");
    }

    TEST(RuntimeConfigTest, ConvertInvalidEndKey)
    {
        ConfigData config;
        config.endKey = ""; 
        
        auto result = convertConfigToRuntime(config);
        // Ensure converting a configuration with an invalid empty end key results in a ValidationError
        ASSERT_TRUE(std::holds_alternative<ValidationError>(result));
        // Verify the error message indicates the end key is invalid
        EXPECT_EQ(std::get<ValidationError>(result).message, "Invalid end key: ''");
    }
}

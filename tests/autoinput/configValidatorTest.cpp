/**
 * @file configValidatorTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include "autoinput/configValidator.h"
#include "autoinput/config.h"
#include "autoinput/runtimeConfig.h"

namespace autoinput
{
    TEST(ConfigValidatorTest, ValidateConfigData_Valid)
    {
        ConfigData config;
        CommandData cmd;
        cmd.action = "click";
        cmd.buttons = {"left"};
        cmd.keys = {"a"};
        cmd.startKeys = {"f1"};
        cmd.pressWait = "100ms";
        cmd.releaseWait = "200ms";
        config.commands.push_back(cmd);
        config.endKey = "f10";

        auto errors = validateConfigData(config);
        EXPECT_TRUE(errors.empty());
    }

    TEST(ConfigValidatorTest, ValidateConfigData_EmptyCommands)
    {
        ConfigData config;
        config.endKey = "f10";

        auto errors = validateConfigData(config);
        ASSERT_FALSE(errors.empty());
        EXPECT_EQ(errors[0].message, "Command list is empty.");
    }

    TEST(ConfigValidatorTest, ValidateConfigData_InvalidAction)
    {
        ConfigData config;
        CommandData cmd;
        cmd.action = "invalid_action";
        config.commands.push_back(cmd);
        config.endKey = "f10";

        auto errors = validateConfigData(config);
        ASSERT_FALSE(errors.empty());
        EXPECT_TRUE(errors[0].message.find("Invalid action") != std::string::npos);
    }

    TEST(ConfigValidatorTest, ValidateCommandData_Valid)
    {
        CommandData cmd;
        cmd.action = "click";
        cmd.buttons = {"left"};

        auto errors = validateCommandData(cmd);
        EXPECT_TRUE(errors.empty());
    }

    TEST(ConfigValidatorTest, ValidateCommandData_Invalid)
    {
        CommandData cmd;
        cmd.action = "invalid";

        auto errors = validateCommandData(cmd);
        ASSERT_FALSE(errors.empty());
        EXPECT_EQ(errors[0].message, "Invalid action: 'invalid'");
    }

    TEST(ConfigValidatorTest, ValidateConfigData_DuplicateName)
    {
        ConfigData config;
        config.endKey = "f10";

        CommandData cmd1;
        cmd1.name = "duplicate";
        cmd1.action = "click";
        cmd1.buttons = {"left"};
        config.commands.push_back(cmd1);

        CommandData cmd2;
        cmd2.name = "duplicate";
        cmd2.action = "click";
        cmd2.buttons = {"right"};
        config.commands.push_back(cmd2);

        auto errors = validateConfigData(config);
        ASSERT_FALSE(errors.empty());
        EXPECT_EQ(errors[0].message, "Duplicate command name: 'duplicate'");
    }

    TEST(ConfigValidatorTest, ValidateConfigData_EmptyNameAllowed)
    {
        ConfigData config;
        config.endKey = "f10";

        CommandData cmd1;
        cmd1.name = "";
        cmd1.action = "click";
        cmd1.buttons = {"left"};
        config.commands.push_back(cmd1);

        CommandData cmd2;
        cmd2.name = "";
        cmd2.action = "click";
        cmd2.buttons = {"right"};
        config.commands.push_back(cmd2);

        auto errors = validateConfigData(config);
        EXPECT_TRUE(errors.empty());
    }

    TEST(ConfigValidatorTest, ValidateConfigData_InvalidButton)
    {
        ConfigData config;
        CommandData cmd;
        cmd.action = "click";
        cmd.buttons = {"invalid_button"};
        config.commands.push_back(cmd);
        config.endKey = "f10";

        auto errors = validateConfigData(config);
        ASSERT_FALSE(errors.empty());
        EXPECT_TRUE(errors[0].message.find("Invalid mouse button") != std::string::npos);
    }

    TEST(ConfigValidatorTest, ValidateConfigData_InvalidKey)
    {
        ConfigData config;
        CommandData cmd;
        cmd.action = "click";
        cmd.keys = {""};
        config.commands.push_back(cmd);
        config.endKey = "f10";

        auto errors = validateConfigData(config);
        ASSERT_FALSE(errors.empty());
        EXPECT_TRUE(errors[0].message.find("Invalid key") != std::string::npos);
    }

    TEST(ConfigValidatorTest, ValidateConfigData_InvalidStartKey)
    {
        ConfigData config;
        CommandData cmd;
        cmd.action = "click";
        cmd.startKeys = {"invalid_key_name_that_is_too_long"};
        config.commands.push_back(cmd);
        config.endKey = "f10";

        auto errors = validateConfigData(config);
        ASSERT_FALSE(errors.empty());
        EXPECT_TRUE(errors[0].message.find("Invalid start key") != std::string::npos);
    }

    TEST(ConfigValidatorTest, ValidateConfigData_InvalidWait)
    {
        ConfigData config;
        CommandData cmd;
        cmd.action = "click";
        cmd.pressWait = "abc";
        config.commands.push_back(cmd);
        config.endKey = "f10";

        auto errors = validateConfigData(config);
        ASSERT_FALSE(errors.empty());
        EXPECT_TRUE(errors[0].message.find("Invalid press wait delay") != std::string::npos);
    }

    TEST(ConfigValidatorTest, ValidateConfigData_InvalidEndKey)
    {
        ConfigData config;
        CommandData cmd;
        cmd.action = "click";
        config.commands.push_back(cmd);
        config.endKey = "";

        auto errors = validateConfigData(config);
        ASSERT_FALSE(errors.empty());
        EXPECT_TRUE(errors[0].message.find("Invalid end key") != std::string::npos);
    }

    TEST(ConfigValidatorTest, ValidateRuntimeConfig_Valid)
    {
        RuntimeConfig config;
        RuntimeCommand cmd;
        cmd.action = ActionState::CLICK;
        cmd.buttons = { Mouse(MouseButton::LEFT) };
        cmd.keys = { Key{.character = "a"} };
        cmd.startKeys = { Key{.character = "f1"} };
        config.commands.push_back(cmd);
        config.endKey = Key{.character = "f10"};

        auto errors = validateRuntimeConfig(config);
        EXPECT_TRUE(errors.empty());
    }

    TEST(ConfigValidatorTest, ValidateRuntimeConfig_InvalidAction)
    {
        RuntimeConfig config;
        RuntimeCommand cmd;
        cmd.action = ActionState::INVALID;
        config.commands.push_back(cmd);
        config.endKey = Key{.character = "f10"};

        auto errors = validateRuntimeConfig(config);
        ASSERT_FALSE(errors.empty());
        EXPECT_TRUE(errors[0].message.find("Invalid action") != std::string::npos);
    }

    TEST(ConfigValidatorTest, MouseTriggerValidation_StartBackIsValid)
    {
        ConfigData config;
        CommandData cmd;
        cmd.action = "click";
        cmd.buttons = {"left"};
        cmd.startKeys = {"back"};
        config.commands.push_back(cmd);
        config.endKey = "f10";

        auto errors = validateConfigData(config);
        EXPECT_TRUE(errors.empty()) << (errors.empty() ? "" : errors[0].message);
    }

    TEST(ConfigValidatorTest, MouseTriggerValidation_StartForwardIsValid)
    {
        ConfigData config;
        CommandData cmd;
        cmd.action = "click";
        cmd.buttons = {"left"};
        cmd.startKeys = {"forward"};
        config.commands.push_back(cmd);
        config.endKey = "f10";

        auto errors = validateConfigData(config);
        EXPECT_TRUE(errors.empty()) << (errors.empty() ? "" : errors[0].message);
    }

    TEST(ConfigValidatorTest, MouseTriggerValidation_EndBackIsValid)
    {
        ConfigData config;
        CommandData cmd;
        cmd.action = "click";
        cmd.buttons = {"left"};
        cmd.startKeys = {"f1"};
        config.commands.push_back(cmd);
        config.endKey = "back";

        auto errors = validateConfigData(config);
        EXPECT_TRUE(errors.empty()) << (errors.empty() ? "" : errors[0].message);
    }

    TEST(ConfigValidatorTest, MouseTriggerValidation_EndForwardIsValid)
    {
        ConfigData config;
        CommandData cmd;
        cmd.action = "click";
        cmd.buttons = {"left"};
        cmd.startKeys = {"f1"};
        config.commands.push_back(cmd);
        config.endKey = "forward";

        auto errors = validateConfigData(config);
        EXPECT_TRUE(errors.empty()) << (errors.empty() ? "" : errors[0].message);
    }

    TEST(ConfigValidatorTest, MouseTriggerValidation_InvalidTriggerStillFails)
    {
        ConfigData config;
        CommandData cmd;
        cmd.action = "click";
        cmd.startKeys = {"invalid_trigger"};
        config.commands.push_back(cmd);
        config.endKey = "f10";

        auto errors = validateConfigData(config);
        EXPECT_FALSE(errors.empty());
        EXPECT_TRUE(errors[0].message.find("Invalid start key") != std::string::npos);
        
        config.commands[0].startKeys = {"f1"};
        config.endKey = "invalid_trigger";
        errors = validateConfigData(config);
        EXPECT_FALSE(errors.empty());
        EXPECT_TRUE(errors[0].message.find("Invalid end key") != std::string::npos);
    }

    TEST(ConfigValidatorTest, MouseTriggerValidation_ExistingKeyboardTriggerStillPasses)
    {
        ConfigData config;
        CommandData cmd;
        cmd.action = "click";
        cmd.startKeys = {"f1", "ctrl+a", "space"};
        config.commands.push_back(cmd);
        config.endKey = "enter";

        auto errors = validateConfigData(config);
        EXPECT_TRUE(errors.empty()) << (errors.empty() ? "" : errors[0].message);
    }
}

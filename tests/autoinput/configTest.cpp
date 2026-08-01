/**
 * @file configTest.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include <fstream>
#include <gtest/gtest.h>

#include "autoinput/config.h"
#include "testUtils.h"

namespace autoinput
{
    namespace
    {
        std::filesystem::path makeConfigFile(const std::filesystem::path& dir, const std::string& fileName, const std::string& contents)
        {
            const std::filesystem::path path = dir / fileName;

            std::ofstream file{ path };
            file << contents;
            file.close();

            return path;
        }
    }

    TEST(ConfigTest, GetConfigFilePathAddsTomlExtension)
    {
        const std::filesystem::path path = getConfigFilePath("example");

        EXPECT_EQ(path.filename(), "example.toml");
    }

    TEST(ConfigTest, GetConfigFilePathKeepsTomlExtension)
    {
        const std::filesystem::path path = getConfigFilePath("example.toml");

        EXPECT_EQ(path.filename(), "example.toml");
    }

    TEST(ConfigTest, DoesConfigDataExistsReturnsTrueForExistingFile)
    {
        test::TemporaryDirectory tempDir("config_test_exists");
        const std::filesystem::path path = makeConfigFile(
            tempDir.path(),
            "autoinput_existing_config_test.toml",
            "[command]\naction = \"click\"\n"
        );

        EXPECT_TRUE(doesConfigDataExists(path));
    }

    TEST(ConfigTest, DoesConfigDataExistsReturnsFalseForMissingFile)
    {
        test::TemporaryDirectory tempDir("config_test_missing");
        const std::filesystem::path path = tempDir.path() / "autoinput_missing_config_test.toml";

        EXPECT_FALSE(doesConfigDataExists(path));
    }

    TEST(ConfigTest, LoadConfigDataReturnsNulloptForMissingFile)
    {
        test::TemporaryDirectory tempDir("config_test_missing_load");
        const std::filesystem::path path = tempDir.path() / "autoinput_missing_load_config_test.toml";

        EXPECT_FALSE(loadConfigData(path).has_value());
    }

    TEST(ConfigTest, LoadConfigDataParsesCommandActionAndEndKey)
    {
        test::TemporaryDirectory tempDir("config_test_parse");
        const std::filesystem::path path = makeConfigFile(
            tempDir.path(),
            "autoinput_config_action_end_test.toml",
            R"toml(
[command]
action = "click"
end = "f3"
)toml"
        );

        const std::optional<ConfigData> configData = loadConfigData(path);

        ASSERT_TRUE(configData.has_value());
        ASSERT_EQ(configData->commands.size(), 1);
        EXPECT_EQ(configData->commands[0].action, "click");
        EXPECT_EQ(configData->endKey, "f3");
    }

    TEST(ConfigTest, LoadConfigDataParsesWaitTimes)
    {
        test::TemporaryDirectory tempDir("config_test_wait");
        const std::filesystem::path path = makeConfigFile(
            tempDir.path(),
            "autoinput_config_wait_test.toml",
            R"toml(
[command]
action = "click"
end = "f3"

[command.time]
press = "100ms..250ms"
release = "1s..2s"
)toml"
        );

        const std::optional<ConfigData> configData = loadConfigData(path);

        ASSERT_TRUE(configData.has_value());
        ASSERT_EQ(configData->commands.size(), 1);
        EXPECT_EQ(configData->commands[0].pressWait, "100ms..250ms");
        EXPECT_EQ(configData->commands[0].releaseWait, "1s..2s");
    }

    TEST(ConfigTest, LoadConfigDataParsesButtonAndStartKeys)
    {
        test::TemporaryDirectory tempDir("config_test_button_start");
        const std::filesystem::path path = makeConfigFile(
            tempDir.path(),
            "autoinput_config_button_start_test.toml",
            R"toml(
[command]
button = "left"
start = ["f2", "f4"]
)toml"
        );

        const std::optional<ConfigData> configData = loadConfigData(path);

        ASSERT_TRUE(configData.has_value());
        ASSERT_EQ(configData->commands.size(), 1);
        ASSERT_EQ(configData->commands[0].buttons.size(), 1);
        EXPECT_EQ(configData->commands[0].buttons[0], "left");
        ASSERT_EQ(configData->commands[0].startKeys.size(), 2);
        EXPECT_EQ(configData->commands[0].startKeys[0], "f2");
        EXPECT_EQ(configData->commands[0].startKeys[1], "f4");
    }

    TEST(ConfigTest, LoadConfigDataParsesMultipleButtonsAndSingleStartKey)
    {
        test::TemporaryDirectory tempDir("config_test_multi_button");
        const std::filesystem::path path = makeConfigFile(
            tempDir.path(),
            "autoinput_config_multi_button_test.toml",
            R"toml(
[command]
button = ["left", "right"]
start = "f2"
)toml"
        );

        const std::optional<ConfigData> configData = loadConfigData(path);

        ASSERT_TRUE(configData.has_value());
        ASSERT_EQ(configData->commands.size(), 1);
        ASSERT_EQ(configData->commands[0].buttons.size(), 2);
        EXPECT_EQ(configData->commands[0].buttons[0], "left");
        EXPECT_EQ(configData->commands[0].buttons[1], "right");
        ASSERT_EQ(configData->commands[0].startKeys.size(), 1);
        EXPECT_EQ(configData->commands[0].startKeys[0], "f2");
    }

    TEST(ConfigTest, LoadConfigDataParsesKeys)
    {
        test::TemporaryDirectory tempDir("config_test_keys");
        const std::filesystem::path path = makeConfigFile(
            tempDir.path(),
            "autoinput_config_keys_test.toml",
            R"toml(
[command]
key = ["a", "b"]
)toml"
        );

        const std::optional<ConfigData> configData = loadConfigData(path);

        ASSERT_TRUE(configData.has_value());
        ASSERT_EQ(configData->commands.size(), 1);
        ASSERT_EQ(configData->commands[0].keys.size(), 2);
        EXPECT_EQ(configData->commands[0].keys[0], "a");
        EXPECT_EQ(configData->commands[0].keys[1], "b");
    }

    TEST(ConfigTest, LoadConfigDataParsesApplicationName)
    {
        test::TemporaryDirectory tempDir("config_test_app");
        const std::filesystem::path path = makeConfigFile(
            tempDir.path(),
            "autoinput_config_app_test.toml",
            R"toml(
[command]
application = "notepad.exe"
)toml"
        );

        const std::optional<ConfigData> configData = loadConfigData(path);

        ASSERT_TRUE(configData.has_value());
        EXPECT_EQ(configData->application, "notepad.exe");
    }

    TEST(ConfigTest, LoadConfigDataParsesBlacklist)
    {
        test::TemporaryDirectory tempDir("config_test_blacklist");
        const std::filesystem::path path = makeConfigFile(
            tempDir.path(),
            "autoinput_config_blacklist_test.toml",
            R"toml(
[command]
action = "click"
blacklist = ["game.exe", "other.exe"]
)toml"
        );

        const std::optional<ConfigData> configData = loadConfigData(path);

        ASSERT_TRUE(configData.has_value());
        ASSERT_EQ(configData->blacklist.size(), 2);
        EXPECT_EQ(configData->blacklist[0], "game.exe");
        EXPECT_EQ(configData->blacklist[1], "other.exe");
        EXPECT_TRUE(configData->appendBlacklist);
    }

    TEST(ConfigTest, LoadConfigDataParsesSingleBlacklist)
    {
        test::TemporaryDirectory tempDir("config_test_single_blacklist");
        const std::filesystem::path path = makeConfigFile(
            tempDir.path(),
            "autoinput_config_single_blacklist_test.toml",
            R"toml(
[command]
action = "click"
blacklist = "only.exe"
appendBlacklist = false
)toml"
        );

        const std::optional<ConfigData> configData = loadConfigData(path);

        ASSERT_TRUE(configData.has_value());
        ASSERT_EQ(configData->blacklist.size(), 1);
        EXPECT_EQ(configData->blacklist[0], "only.exe");
        EXPECT_FALSE(configData->appendBlacklist);
    }

    TEST(ConfigTest, LoadConfigDataParsesMultipleCommands)
    {
        test::TemporaryDirectory tempDir("config_test_multi_command");
        const std::filesystem::path path = makeConfigFile(
            tempDir.path(),
            "autoinput_config_multi_command_test.toml",
            R"toml(
end = "f10"
application = "notepad.exe"
blacklist = ["discord.exe"]

[[command]]
action = "click"
button = "left"
start = "f8"

[[command]]
action = "hold"
key = "a"
start = "f9"
)toml"
        );

        const std::optional<ConfigData> configData = loadConfigData(path);

        ASSERT_TRUE(configData.has_value());
        ASSERT_EQ(configData->commands.size(), 2);

        EXPECT_EQ(configData->commands[0].action, "click");
        ASSERT_EQ(configData->commands[0].buttons.size(), 1);
        EXPECT_EQ(configData->commands[0].buttons[0], "left");
        ASSERT_EQ(configData->commands[0].startKeys.size(), 1);
        EXPECT_EQ(configData->commands[0].startKeys[0], "f8");

        EXPECT_EQ(configData->commands[1].action, "hold");
        ASSERT_EQ(configData->commands[1].keys.size(), 1);
        EXPECT_EQ(configData->commands[1].keys[0], "a");
        ASSERT_EQ(configData->commands[1].startKeys.size(), 1);
        EXPECT_EQ(configData->commands[1].startKeys[0], "f9");

        EXPECT_EQ(configData->endKey, "f10");
        EXPECT_EQ(configData->application, "notepad.exe");
        ASSERT_EQ(configData->blacklist.size(), 1);
        EXPECT_EQ(configData->blacklist[0], "discord.exe");
    }

    TEST(ConfigTest, TryGetTableValueReturnsTrueWhenValueExists)
    {
        const toml::table table{
            { "name", "autoinput" }
        };

        std::string value;

        EXPECT_TRUE(tryGetTableValue(table, "name", value));
        EXPECT_EQ(value, "autoinput");
    }

    TEST(ConfigTest, TryGetTableValueReturnsFalseWhenValueIsMissing)
    {
        const toml::table table{
            { "name", "autoinput" }
        };

        std::string value = "unchanged";

        EXPECT_FALSE(tryGetTableValue(table, "missing", value));
        EXPECT_EQ(value, "unchanged");
    }

    TEST(ConfigTest, TryGetTableValueReturnsFalseWhenTypeDoesNotMatch)
    {
        const toml::table table{
            { "count", 10 }
        };

        std::string value = "unchanged";

        EXPECT_FALSE(tryGetTableValue(table, "count", value));
        EXPECT_EQ(value, "unchanged");
    }
}
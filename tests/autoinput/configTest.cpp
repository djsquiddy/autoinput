/**
 * @file configTest.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include <fstream>
#include <gtest/gtest.h>

#include "autoinput/config.h"

namespace autoinput
{
    namespace
    {
        std::filesystem::path makeTemporaryConfigFile(const std::string& fileName, const std::string& contents)
        {
            const std::filesystem::path path = std::filesystem::temp_directory_path() / fileName;

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
        const std::filesystem::path path = makeTemporaryConfigFile(
            "autoinput_existing_config_test.toml",
            "[command]\naction = \"click\"\n"
        );

        EXPECT_TRUE(doesConfigDataExists(path));

        std::filesystem::remove(path);
    }

    TEST(ConfigTest, DoesConfigDataExistsReturnsFalseForMissingFile)
    {
        const std::filesystem::path path = std::filesystem::temp_directory_path() / "autoinput_missing_config_test.toml";

        std::filesystem::remove(path);

        EXPECT_FALSE(doesConfigDataExists(path));
    }

    TEST(ConfigTest, LoadConfigDataReturnsNulloptForMissingFile)
    {
        const std::filesystem::path path = std::filesystem::temp_directory_path() / "autoinput_missing_load_config_test.toml";

        std::filesystem::remove(path);

        EXPECT_FALSE(loadConfigData(path).has_value());
    }

    TEST(ConfigTest, LoadConfigDataParsesCommandActionAndEndKey)
    {
        const std::filesystem::path path = makeTemporaryConfigFile(
            "autoinput_config_action_end_test.toml",
            R"toml(
[command]
action = "click"
end = "f3"
)toml"
        );

        const std::optional<ConfigData> configData = loadConfigData(path);

        ASSERT_TRUE(configData.has_value());
        EXPECT_EQ(configData->action, "click");
        EXPECT_EQ(configData->endKey, "f3");

        std::filesystem::remove(path);
    }

    TEST(ConfigTest, LoadConfigDataParsesWaitTimes)
    {
        const std::filesystem::path path = makeTemporaryConfigFile(
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
        EXPECT_EQ(configData->pressWait, "100ms..250ms");
        EXPECT_EQ(configData->releaseWait, "1s..2s");

        std::filesystem::remove(path);
    }

    TEST(ConfigTest, LoadConfigDataParsesButtonAndStartKeys)
    {
        const std::filesystem::path path = makeTemporaryConfigFile(
            "autoinput_config_button_start_test.toml",
            R"toml(
[command]
button = "left"
start = ["f2", "f4"]
)toml"
        );

        const std::optional<ConfigData> configData = loadConfigData(path);

        ASSERT_TRUE(configData.has_value());
        ASSERT_EQ(configData->buttons.size(), 1);
        EXPECT_EQ(configData->buttons[0], "left");
        ASSERT_EQ(configData->startKeys.size(), 2);
        EXPECT_EQ(configData->startKeys[0], "f2");
        EXPECT_EQ(configData->startKeys[1], "f4");

        std::filesystem::remove(path);
    }

    TEST(ConfigTest, LoadConfigDataParsesMultipleButtonsAndSingleStartKey)
    {
        const std::filesystem::path path = makeTemporaryConfigFile(
            "autoinput_config_multi_button_test.toml",
            R"toml(
[command]
button = ["left", "right"]
start = "f2"
)toml"
        );

        const std::optional<ConfigData> configData = loadConfigData(path);

        ASSERT_TRUE(configData.has_value());
        ASSERT_EQ(configData->buttons.size(), 2);
        EXPECT_EQ(configData->buttons[0], "left");
        EXPECT_EQ(configData->buttons[1], "right");
        ASSERT_EQ(configData->startKeys.size(), 1);
        EXPECT_EQ(configData->startKeys[0], "f2");

        std::filesystem::remove(path);
    }

    TEST(ConfigTest, LoadConfigDataParsesKeys)
    {
        const std::filesystem::path path = makeTemporaryConfigFile(
            "autoinput_config_keys_test.toml",
            R"toml(
[command]
key = ["a", "b"]
)toml"
        );

        const std::optional<ConfigData> configData = loadConfigData(path);

        ASSERT_TRUE(configData.has_value());
        ASSERT_EQ(configData->keys.size(), 2);
        EXPECT_EQ(configData->keys[0], "a");
        EXPECT_EQ(configData->keys[1], "b");

        std::filesystem::remove(path);
    }

    TEST(ConfigTest, LoadConfigDataParsesApplicationName)
    {
        const std::filesystem::path path = makeTemporaryConfigFile(
            "autoinput_config_app_test.toml",
            R"toml(
[command]
application = "notepad.exe"
)toml"
        );

        const std::optional<ConfigData> configData = loadConfigData(path);

        ASSERT_TRUE(configData.has_value());
        EXPECT_EQ(configData->application, "notepad.exe");

        std::filesystem::remove(path);
    }

    TEST(ConfigTest, LoadConfigDataParsesBlacklist)
    {
        const std::filesystem::path path = makeTemporaryConfigFile(
            "autoinput_config_blacklist_test.toml",
            R"toml(
[command]
action = "click"
blacklist = ["game.exe", "other.exe"]
)toml"
        );

        const std::optional<ConfigData> configData = loadConfigData(path);

        ASSERT_TRUE(configData.has_value());
        EXPECT_EQ(configData->blacklist.size(), 2);
        EXPECT_EQ(configData->blacklist[0], "game.exe");
        EXPECT_EQ(configData->blacklist[1], "other.exe");

        std::filesystem::remove(path);
    }

    TEST(ConfigTest, LoadConfigDataParsesSingleBlacklist)
    {
        const std::filesystem::path path = makeTemporaryConfigFile(
            "autoinput_config_single_blacklist_test.toml",
            R"toml(
[command]
action = "click"
blacklist = "only.exe"
)toml"
        );

        const std::optional<ConfigData> configData = loadConfigData(path);

        ASSERT_TRUE(configData.has_value());
        EXPECT_EQ(configData->blacklist.size(), 1);
        EXPECT_EQ(configData->blacklist[0], "only.exe");

        std::filesystem::remove(path);
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
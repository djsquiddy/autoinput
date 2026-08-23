/**
 * @file settingsTest.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include <fstream>
#include <gtest/gtest.h>

#include "autoinput/config/settings.h"
#include "autoinput/config/defaults.h"

namespace autoinput
{
    namespace
    {
        std::filesystem::path makeTemporarySettingsFile(const std::string& fileName, const std::string& contents)
        {
            const std::filesystem::path path = std::filesystem::temp_directory_path() / fileName;

            std::ofstream file{ path };
            file << contents;
            file.close();

            return path;
        }
    }

    TEST(SettingsTest, LoadReturnsFalseForMissingFile)
    {
        Settings settings;
        const std::filesystem::path path = std::filesystem::temp_directory_path() / "autoinput_missing_settings_test.toml";
        std::filesystem::remove(path);

        // Verify that loading a non-existent settings file returns false
        EXPECT_FALSE(settings.load(path));
    }

    TEST(SettingsTest, LoadParsesDefaultStartAndEndKeys)
    {
        const std::filesystem::path path = makeTemporarySettingsFile(
            "autoinput_settings_defaults_test.toml",
            R"toml(
start = "f8"
end = "f9"
press = "500ms"
release = "1s..2s"
action = "hold"
button = "right"
)toml"
        );

        Settings settings;
        // Ensure the settings file loads successfully
        ASSERT_TRUE(settings.load(path));

        const auto& defaults = settings.getDefaults();
        // Verify default start key is parsed correctly
        EXPECT_EQ(defaults.start, "f8");
        // Verify default end key is parsed correctly
        EXPECT_EQ(defaults.end, "f9");
        // Verify default press duration is parsed correctly
        EXPECT_EQ(defaults.press, "500ms");
        // Verify default release duration range is parsed correctly
        EXPECT_EQ(defaults.release, "1s..2s");
        // Verify default action is parsed correctly
        EXPECT_EQ(defaults.action, "hold");
        // Verify default mouse button is parsed correctly
        EXPECT_EQ(defaults.button, "right");

        std::filesystem::remove(path);
    }

    TEST(SettingsTest, LoadHandlesPartialDefaults)
    {
        const std::filesystem::path path = makeTemporarySettingsFile(
            "autoinput_settings_partial_test.toml",
            R"toml(
start = "f10"
)toml"
        );

        Settings settings;
        // Ensure the settings file loads successfully
        ASSERT_TRUE(settings.load(path));

        const auto& defaults = settings.getDefaults();
        // Verify overridden start key is parsed correctly
        EXPECT_EQ(defaults.start, "f10");
        // Verify end key falls back to default constant
        EXPECT_EQ(defaults.end, defaults::EndKey);
        // Verify press duration is empty when not specified
        EXPECT_TRUE(defaults.press.empty());
        // Verify release duration is empty when not specified
        EXPECT_TRUE(defaults.release.empty());
        // Verify action falls back to default action name
        EXPECT_EQ(defaults.action, defaults::DefaultActionName);
        // Verify button falls back to default mouse button name
        EXPECT_EQ(defaults.button, defaults::DefaultMouseButtonName);
        
        std::filesystem::remove(path);
    }

    TEST(SettingsTest, LoadParsesTopLevelSettings)
    {
        const std::filesystem::path path = makeTemporarySettingsFile(
            "autoinput_settings_top_level_test.toml",
            R"toml(
start = "f10"
end = "f11"
blacklist = ["app1.exe"]
)toml"
        );

        Settings settings;
        // Ensure the settings file loads successfully
        ASSERT_TRUE(settings.load(path));

        const auto& defaults = settings.getDefaults();
        // Verify start key is parsed from top-level settings
        EXPECT_EQ(defaults.start, "f10");
        // Verify end key is parsed from top-level settings
        EXPECT_EQ(defaults.end, "f11");
        // Ensure blacklist contains the expected number of entries
        ASSERT_EQ(defaults.blacklist.size(), 1);
        // Verify the blacklisted application name is parsed correctly
        EXPECT_EQ(defaults.blacklist[0], "app1.exe");

        std::filesystem::remove(path);
    }
}

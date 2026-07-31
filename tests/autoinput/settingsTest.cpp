/**
 * @file settingsTest.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include <fstream>
#include <gtest/gtest.h>

#include "autoinput/settings.h"

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

        EXPECT_FALSE(settings.load(path));
    }

    TEST(SettingsTest, LoadParsesDefaultStartAndEndKeys)
    {
        const std::filesystem::path path = makeTemporarySettingsFile(
            "autoinput_settings_defaults_test.toml",
            R"toml(
[defaults]
start = "f8"
end = "f9"
press = "500ms"
release = "1s..2s"
action = "hold"
button = "right"
)toml"
        );

        Settings settings;
        ASSERT_TRUE(settings.load(path));

        const auto& defaults = settings.getDefaults();
        EXPECT_EQ(defaults.start, "f8");
        EXPECT_EQ(defaults.end, "f9");
        EXPECT_EQ(defaults.press, "500ms");
        EXPECT_EQ(defaults.release, "1s..2s");
        EXPECT_EQ(defaults.action, "hold");
        EXPECT_EQ(defaults.button, "right");

        std::filesystem::remove(path);
    }

    TEST(SettingsTest, LoadHandlesPartialDefaults)
    {
        const std::filesystem::path path = makeTemporarySettingsFile(
            "autoinput_settings_partial_test.toml",
            R"toml(
[defaults]
start = "f10"
)toml"
        );

        Settings settings;
        ASSERT_TRUE(settings.load(path));

        const auto& defaults = settings.getDefaults();
        EXPECT_EQ(defaults.start, "f10");
        EXPECT_TRUE(defaults.end.empty());
        EXPECT_TRUE(defaults.press.empty());
        EXPECT_TRUE(defaults.release.empty());
        EXPECT_TRUE(defaults.action.empty());
        EXPECT_TRUE(defaults.button.empty());
        
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
        ASSERT_TRUE(settings.load(path));

        const auto& defaults = settings.getDefaults();
        EXPECT_EQ(defaults.start, "f10");
        EXPECT_EQ(defaults.end, "f11");
        ASSERT_EQ(defaults.blacklist.size(), 1);
        EXPECT_EQ(defaults.blacklist[0], "app1.exe");

        std::filesystem::remove(path);
    }
}

/**
 * @file userSettingsTest.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include <fstream>
#include <gtest/gtest.h>
#include <filesystem>

#ifdef _WIN32
#include <windows.h>
#else
#include <stdlib.h>
#endif

#include "autoinput/config/settings.h"
#include "autoinput/config/config.h"
#include "autoinput/platform/environment.h"
#include "testUtils.h"

namespace autoinput
{
    namespace
    {
        void createSettingsFile(const std::filesystem::path& path, const std::string& contents)
        {
            std::filesystem::create_directories(path.parent_path());
            std::ofstream file{ path };
            file << contents;
            file.close();
        }
    }

    class UserSettingsTest : public ::testing::Test
    {
    protected:
        UserSettingsTest()
            : m_tempHome("autoinput_user_settings_test")
#ifdef _WIN32
            , m_scopedHome("USERPROFILE", m_tempHome.path().string())
#else
            , m_scopedHome("HOME", m_tempHome.path().string())
#endif
        {}

        void SetUp() override
        {
            // Ensure built-in configs dir exists
            m_builtinConfigsDir = SystemEnvironment::instance().executableDirectoryPath() / "configs";
            std::filesystem::create_directories(m_builtinConfigsDir);
            
            // Clean up any existing settings files
            std::filesystem::remove(m_builtinConfigsDir / "settings.toml");
            // No need to clean up temp home, it's a new unique directory each time
        }

        void TearDown() override
        {
            std::filesystem::remove(m_builtinConfigsDir / "settings.toml");
        }

        test::TemporaryDirectory m_tempHome;
        test::ScopedEnvironmentVariable m_scopedHome;
        std::filesystem::path m_builtinConfigsDir;
    };

    TEST_F(UserSettingsTest, UserSettingsOverrideBuiltin)
    {
        createSettingsFile(m_builtinConfigsDir / "settings.toml", R"toml(
start = "f1"
end = "f2"
)toml");

        createSettingsFile(m_tempHome / ".autoinput" / "settings.toml", R"toml(
start = "f3"
)toml");

        Settings settings;
        ASSERT_TRUE(settings.load());

        const auto& defaults = settings.getDefaults();
        EXPECT_EQ(defaults.start, "f3"); // Overridden
        EXPECT_EQ(defaults.end, "f2");   // Kept from built-in
    }

    TEST_F(UserSettingsTest, UserBlacklistReplacesBuiltinBlacklist)
    {
        createSettingsFile(m_builtinConfigsDir / "settings.toml", R"toml(
blacklist = ["app1.exe", "app2.exe"]
)toml");

        createSettingsFile(m_tempHome / ".autoinput" / "settings.toml", R"toml(
appendBlacklist = false
blacklist = ["app3.exe"]
)toml");

        Settings settings;
        ASSERT_TRUE(settings.load());

        const auto& defaults = settings.getDefaults();
        ASSERT_EQ(defaults.blacklist.size(), 1);
        EXPECT_EQ(defaults.blacklist[0], "app3.exe");
    }

    TEST_F(UserSettingsTest, UserBlacklistAppendsToBuiltinBlacklistWhenFlagIsSet)
    {
        createSettingsFile(m_builtinConfigsDir / "settings.toml", R"toml(
blacklist = ["app1.exe", "app2.exe"]
)toml");

        createSettingsFile(m_tempHome / ".autoinput" / "settings.toml", R"toml(
appendBlacklist = true
blacklist = ["app3.exe"]
)toml");

        Settings settings;
        ASSERT_TRUE(settings.load());

        const auto& defaults = settings.getDefaults();
        ASSERT_EQ(defaults.blacklist.size(), 3);
        EXPECT_EQ(defaults.blacklist[0], "app1.exe");
        EXPECT_EQ(defaults.blacklist[1], "app2.exe");
        EXPECT_EQ(defaults.blacklist[2], "app3.exe");
    }

    TEST_F(UserSettingsTest, LoadReturnsTrueIfOnlyUserLevelExists)
    {
        createSettingsFile(m_tempHome / ".autoinput" / "settings.toml", R"toml(
start = "f10"
)toml");

        Settings settings;
        ASSERT_TRUE(settings.load());

        EXPECT_EQ(settings.getDefaults().start, "f10");
    }

    TEST_F(UserSettingsTest, LoadReturnsTrueIfOnlyBuiltinExists)
    {
        createSettingsFile(m_builtinConfigsDir / "settings.toml", R"toml(
start = "f11"
)toml");

        Settings settings;
        ASSERT_TRUE(settings.load());

        EXPECT_EQ(settings.getDefaults().start, "f11");
    }
}

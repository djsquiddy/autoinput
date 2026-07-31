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

#include "autoinput/settings.h"
#include "autoinput/config.h"
#include "autoinput/platform.h"

namespace autoinput
{
    namespace
    {
        void setEnvVar(const std::string& name, const std::string& value)
        {
#ifdef _WIN32
            SetEnvironmentVariableA(name.c_str(), value.c_str());
            _putenv((name + "=" + value).c_str());
#else
            setenv(name.c_str(), value.c_str(), 1);
#endif
        }

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
        void SetUp() override
        {
            m_tempHome = std::filesystem::temp_directory_path() / "autoinput_test_home";
            std::filesystem::create_directories(m_tempHome);
            
            // Backup old env var
#ifdef _WIN32
            const char* oldProfile = std::getenv("USERPROFILE");
            if (oldProfile) m_oldEnv = oldProfile;
            setEnvVar("USERPROFILE", m_tempHome.string());
#else
            const char* oldHome = std::getenv("HOME");
            if (oldHome) m_oldEnv = oldHome;
            setEnvVar("HOME", m_tempHome.string());
#endif

            // Ensure built-in configs dir exists
            m_builtinConfigsDir = platform::getExecutablePath() / "configs";
            std::filesystem::create_directories(m_builtinConfigsDir);
            
            // Clean up any existing settings files
            std::filesystem::remove(m_builtinConfigsDir / "settings.toml");
            std::filesystem::remove(m_tempHome / ".autoinput" / "settings.toml");
        }

        void TearDown() override
        {
            std::filesystem::remove_all(m_tempHome);
            std::filesystem::remove(m_builtinConfigsDir / "settings.toml");
            
            // Restore old env var
#ifdef _WIN32
            setEnvVar("USERPROFILE", m_oldEnv);
#else
            setEnvVar("HOME", m_oldEnv);
#endif
        }

        std::filesystem::path m_tempHome;
        std::filesystem::path m_builtinConfigsDir;
        std::string m_oldEnv;
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

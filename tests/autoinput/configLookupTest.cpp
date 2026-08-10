/**
 * @file configLookupTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include "autoinput/config/config.h"
#include "autoinput/platform/platform.h"
#include "testUtils.h"

namespace autoinput
{
    class ConfigLookupTest : public ::testing::Test
    {
    protected:
        ConfigLookupTest()
            : tempDir("autoinput_test_config")
#ifdef _WIN32
            , scopedHome("USERPROFILE", tempDir.path().string())
#else
            , scopedHome("HOME", tempDir.path().string())
#endif
        {}

        test::TemporaryDirectory tempDir;
        test::ScopedEnvironmentVariable scopedHome;
    };

    TEST_F(ConfigLookupTest, FindInUserConfigDir)
    {
        std::filesystem::path userConfigDir = tempDir.path() / ".autoinput";
        std::filesystem::create_directories(userConfigDir);
        
        std::filesystem::path configFile = userConfigDir / "test_user_config.toml";
        std::ofstream file{ configFile };
        file << "[command]\naction = \"click\"\n";
        file.close();

        std::filesystem::path foundPath = getConfigFilePath("test_user_config");
        
        EXPECT_EQ(foundPath, configFile);
    }

    TEST_F(ConfigLookupTest, FallbackToGlobalConfigDir)
    {
        // Don't create user config, just global one
        std::filesystem::path globalConfigDir = getConfigsPath();
        if (!std::filesystem::exists(globalConfigDir))
        {
            std::filesystem::create_directories(globalConfigDir);
        }
        
        std::filesystem::path configFile = globalConfigDir / "test_global_config.toml";
        std::ofstream file{ configFile };
        file << "[command]\naction = \"move\"\n";
        file.close();

        std::filesystem::path foundPath = getConfigFilePath("test_global_config");
        
        EXPECT_EQ(foundPath, configFile);

        // Clean up global config created for test
        std::filesystem::remove(configFile);
    }

    TEST_F(ConfigLookupTest, UserConfigPriorityOverGlobal)
    {
        std::filesystem::path userConfigDir = tempDir.path() / ".autoinput";
        std::filesystem::create_directories(userConfigDir);
        
        std::filesystem::path userConfigFile = userConfigDir / "priority_test.toml";
        std::ofstream ufile{ userConfigFile };
        ufile << "[command]\naction = \"user\"\n";
        ufile.close();

        std::filesystem::path globalConfigDir = getConfigsPath();
        if (!std::filesystem::exists(globalConfigDir))
        {
            std::filesystem::create_directories(globalConfigDir);
        }
        
        std::filesystem::path globalConfigFile = globalConfigDir / "priority_test.toml";
        std::ofstream gfile{ globalConfigFile };
        gfile << "[command]\naction = \"global\"\n";
        gfile.close();

        std::filesystem::path foundPath = getConfigFilePath("priority_test");
        
        // Should find user one
        EXPECT_EQ(foundPath, userConfigFile);

        // Clean up global config
        std::filesystem::remove(globalConfigFile);
    }

    TEST_F(ConfigLookupTest, NotFoundReturnsGlobalPath)
    {
        // Neither user nor global exists
        std::filesystem::path globalConfigDir = getConfigsPath();
        std::filesystem::path expectedPath = globalConfigDir / "non_existent.toml";
        
        std::filesystem::path foundPath = getConfigFilePath("non_existent");
        
        // It should return global path by default
        EXPECT_EQ(foundPath, expectedPath);
    }
}

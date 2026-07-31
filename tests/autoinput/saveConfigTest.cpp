/**
 * @file saveConfigTest.cpp
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

#include "autoinput/arguments.h"
#include "autoinput/config.h"
#include "autoinput/platform.h"
#include "autoinput/types.h"

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
    }

    class DumpTest : public ::testing::Test
    {
    protected:
        void SetUp() override
        {
            m_tempHome = std::filesystem::temp_directory_path() / "autoinput_dump_test_home";
            std::filesystem::remove_all(m_tempHome);
            std::filesystem::create_directories(m_tempHome);
            
#ifdef _WIN32
            const char* oldProfile = std::getenv("USERPROFILE");
            if (oldProfile) m_oldEnv = oldProfile;
            setEnvVar("USERPROFILE", m_tempHome.string());
#else
            const char* oldHome = std::getenv("HOME");
            if (oldHome) m_oldEnv = oldHome;
            setEnvVar("HOME", m_tempHome.string());
#endif
        }

        void TearDown() override
        {
            std::filesystem::remove_all(m_tempHome);
            
#ifdef _WIN32
            setEnvVar("USERPROFILE", m_oldEnv);
#else
            setEnvVar("HOME", m_oldEnv);
#endif
        }

        std::filesystem::path m_tempHome;
        std::string m_oldEnv;
    };

    TEST_F(DumpTest, SaveConfigCreatesCorrectFile)
    {
        ProgramArguments args;
        // Use a vector of strings to keep pointers valid
        std::vector<std::string> argvStr = { "autoinput", "hold", "left", "-s", "f2", "-e", "f3", "-S", "test_config" };
        std::vector<char*> argv;
        for (auto& s : argvStr) argv.push_back(s.data());

        ASSERT_TRUE(args.parseArguments(gsl::make_span(argv.data(), argv.size())));

        ConfigData configData = args.toConfigData();
        std::filesystem::path dumpPath = getUserConfigsPath() / "test_config.toml";
        
        ASSERT_TRUE(saveConfigData(configData, dumpPath));
        ASSERT_TRUE(std::filesystem::exists(dumpPath));

        auto loaded = loadConfigData(dumpPath);
        ASSERT_TRUE(loaded.has_value());
        ASSERT_EQ(loaded->commands.size(), 1);
        EXPECT_EQ(loaded->commands[0].action, "hold");
        ASSERT_EQ(loaded->commands[0].buttons.size(), 1);
        EXPECT_EQ(loaded->commands[0].buttons[0], "left");
        ASSERT_EQ(loaded->commands[0].startKeys.size(), 1);
        EXPECT_EQ(loaded->commands[0].startKeys[0], "f2");
        EXPECT_EQ(loaded->endKey, "f3");
    }

    TEST_F(DumpTest, SaveConfigHandlesWaitTimes)
    {
        ProgramArguments args;
        std::vector<std::string> argvStr = { "autoinput", "click", "left", "-w", "500ms..1s", "--release-wait", "2s", "--save-config", "wait_config" };
        std::vector<char*> argv;
        for (auto& s : argvStr) argv.push_back(s.data());

        ASSERT_TRUE(args.parseArguments(gsl::make_span(argv.data(), argv.size())));

        ConfigData configData = args.toConfigData();
        std::filesystem::path dumpPath = getUserConfigsPath() / "wait_config.toml";
        
        ASSERT_TRUE(saveConfigData(configData, dumpPath));
        
        auto loaded = loadConfigData(dumpPath);
        ASSERT_TRUE(loaded.has_value());
        ASSERT_EQ(loaded->commands.size(), 1);
        EXPECT_EQ(loaded->commands[0].pressWait, "500ms..1s");
        EXPECT_EQ(loaded->commands[0].releaseWait, "2s");
    }

    TEST_F(DumpTest, SaveConfigHandlesBlacklist)
    {
        ProgramArguments args;
        std::vector<std::string> argvStr = { "autoinput", "left", "-B", "app1.exe", "--blacklist", "app2.exe", "--save-config", "blacklist_config" };
        std::vector<char*> argv;
        for (auto& s : argvStr) argv.push_back(s.data());

        ASSERT_TRUE(args.parseArguments(gsl::make_span(argv.data(), argv.size())));

        ConfigData configData = args.toConfigData();
        std::filesystem::path dumpPath = getUserConfigsPath() / "blacklist_config.toml";
        
        ASSERT_TRUE(saveConfigData(configData, dumpPath));
        
        auto loaded = loadConfigData(dumpPath);
        ASSERT_TRUE(loaded.has_value());
        ASSERT_EQ(loaded->blacklist.size(), 2);
        EXPECT_EQ(loaded->blacklist[0], "app1.exe");
        EXPECT_EQ(loaded->blacklist[1], "app2.exe");
        EXPECT_TRUE(loaded->appendBlacklist);
    }

    TEST_F(DumpTest, SaveConfigHandlesMultipleCommands)
    {
        ProgramArguments args;
        // click left start f8, hold right start f9
        std::vector<std::string> argvStr = { "autoinput", "click", "left", "hold", "right", "-s", "f8", "f9", "--save-config", "multi_config" };
        std::vector<char*> argv;
        for (auto& s : argvStr) argv.push_back(s.data());

        ASSERT_TRUE(args.parseArguments(gsl::make_span(argv.data(), argv.size())));

        ConfigData configData = args.toConfigData();
        std::filesystem::path dumpPath = getUserConfigsPath() / "multi_config.toml";
        
        ASSERT_TRUE(saveConfigData(configData, dumpPath));
        
        auto loaded = loadConfigData(dumpPath);
        ASSERT_TRUE(loaded.has_value());
        ASSERT_EQ(loaded->commands.size(), 2);
        
        EXPECT_EQ(loaded->commands[0].action, "click");
        ASSERT_EQ(loaded->commands[0].buttons.size(), 1);
        EXPECT_EQ(loaded->commands[0].buttons[0], "left");
        ASSERT_EQ(loaded->commands[0].startKeys.size(), 1);
        EXPECT_EQ(loaded->commands[0].startKeys[0], "f8");

        EXPECT_EQ(loaded->commands[1].action, "hold");
        ASSERT_EQ(loaded->commands[1].buttons.size(), 1);
        EXPECT_EQ(loaded->commands[1].buttons[0], "right");
        ASSERT_EQ(loaded->commands[1].startKeys.size(), 1);
        EXPECT_EQ(loaded->commands[1].startKeys[0], "f9");
    }
}

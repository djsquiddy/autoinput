/**
 * @file runCommandRefactorTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include <filesystem>
#include <vector>
#include <string>

#include "autoinput/cli/cliApplication.h"
#include "autoinput/config.h"
#include "autoinput/errorCode.h"
#include "testUtils.h"

namespace autoinput
{
    class RunCommandRefactorTest : public ::testing::Test
    {
    protected:
        RunCommandRefactorTest()
            : m_tempHome("autoinput_refactor_test_home")
#ifdef _WIN32
            , m_scopedHome("USERPROFILE", m_tempHome.path().string())
#else
            , m_scopedHome("HOME", m_tempHome.path().string())
#endif
        {}

        test::TemporaryDirectory m_tempHome;
        test::ScopedEnvironmentVariable m_scopedHome;
    };

    TEST_F(RunCommandRefactorTest, ParallelTypeAssociation)
    {
        cli::CliApplication app;
        // run --type hold --button middle --type hold --button right
        std::vector<std::string> argvStr = { "autoinput", "run", "--type", "hold", "--button", "middle", "--type", "hold", "--button", "right", "--save-config", "refactor_type" };
        std::vector<char*> argv;
        for (auto& s : argvStr) argv.push_back(s.data());

        ASSERT_TRUE(app.parse(gsl::make_span(argv.data(), argv.size())));
        ASSERT_EQ(app.execute(), static_cast<i32>(ErrorCode::Success));

        std::filesystem::path dumpPath = getUserConfigsPath() / "refactor_type.toml";
        auto loaded = loadConfigData(dumpPath);
        ASSERT_TRUE(loaded.has_value());
        ASSERT_EQ(loaded->commands.size(), 2);
        
        EXPECT_EQ(loaded->commands[0].action, "hold");
        ASSERT_FALSE(loaded->commands[0].buttons.empty());
        EXPECT_EQ(loaded->commands[0].buttons[0], "middle");

        EXPECT_EQ(loaded->commands[1].action, "hold");
        ASSERT_FALSE(loaded->commands[1].buttons.empty());
        EXPECT_EQ(loaded->commands[1].buttons[0], "right");
    }

    TEST_F(RunCommandRefactorTest, ParallelStartKeyAssociation)
    {
        cli::CliApplication app;
        // run --button middle --start f4 --button right --start f5
        std::vector<std::string> argvStr = { "autoinput", "run", "--button", "middle", "--start", "f4", "--button", "right", "--start", "f5", "--save-config", "refactor_start" };
        std::vector<char*> argv;
        for (auto& s : argvStr) argv.push_back(s.data());

        ASSERT_TRUE(app.parse(gsl::make_span(argv.data(), argv.size())));
        ASSERT_EQ(app.execute(), static_cast<i32>(ErrorCode::Success));

        std::filesystem::path dumpPath = getUserConfigsPath() / "refactor_start.toml";
        auto loaded = loadConfigData(dumpPath);
        ASSERT_TRUE(loaded.has_value());
        ASSERT_EQ(loaded->commands.size(), 2);
        
        ASSERT_FALSE(loaded->commands[0].buttons.empty());
        EXPECT_EQ(loaded->commands[0].buttons[0], "middle");
        ASSERT_FALSE(loaded->commands[0].startKeys.empty());
        EXPECT_EQ(loaded->commands[0].startKeys[0], "f4");

        ASSERT_FALSE(loaded->commands[1].buttons.empty());
        EXPECT_EQ(loaded->commands[1].buttons[0], "right");
        ASSERT_FALSE(loaded->commands[1].startKeys.empty());
        EXPECT_EQ(loaded->commands[1].startKeys[0], "f5");
    }

    TEST_F(RunCommandRefactorTest, MouseModifierPreservation)
    {
        cli::CliApplication app;
        // run --type hold --button shift+left --start f4
        std::vector<std::string> argvStr = { "autoinput", "run", "--type", "hold", "--button", "shift+left", "--start", "f4", "--save-config", "refactor_mod" };
        std::vector<char*> argv;
        for (auto& s : argvStr) argv.push_back(s.data());

        ASSERT_TRUE(app.parse(gsl::make_span(argv.data(), argv.size())));
        ASSERT_EQ(app.execute(), static_cast<i32>(ErrorCode::Success));

        std::filesystem::path dumpPath = getUserConfigsPath() / "refactor_mod.toml";
        auto loaded = loadConfigData(dumpPath);
        ASSERT_TRUE(loaded.has_value());
        ASSERT_EQ(loaded->commands.size(), 1);
        
        EXPECT_EQ(loaded->commands[0].action, "hold");
        ASSERT_FALSE(loaded->commands[0].buttons.empty());
        EXPECT_EQ(loaded->commands[0].buttons[0], "shift+left");
        ASSERT_FALSE(loaded->commands[0].startKeys.empty());
        EXPECT_EQ(loaded->commands[0].startKeys[0], "f4");
    }

    TEST_F(RunCommandRefactorTest, KeyTargetPreservation)
    {
        cli::CliApplication app;
        // run --key space --start f6
        std::vector<std::string> argvStr = { "autoinput", "run", "--key", "space", "--start", "f6", "--save-config", "refactor_key" };
        std::vector<char*> argv;
        for (auto& s : argvStr) argv.push_back(s.data());

        ASSERT_TRUE(app.parse(gsl::make_span(argv.data(), argv.size())));
        ASSERT_EQ(app.execute(), static_cast<i32>(ErrorCode::Success));

        std::filesystem::path dumpPath = getUserConfigsPath() / "refactor_key.toml";
        auto loaded = loadConfigData(dumpPath);
        ASSERT_TRUE(loaded.has_value());
        ASSERT_EQ(loaded->commands.size(), 1);
        
        ASSERT_FALSE(loaded->commands[0].keys.empty());
        EXPECT_EQ(loaded->commands[0].keys[0], "space");
        ASSERT_FALSE(loaded->commands[0].startKeys.empty());
        EXPECT_EQ(loaded->commands[0].startKeys[0], "f6");
    }
}

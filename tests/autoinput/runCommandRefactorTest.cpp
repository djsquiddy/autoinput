/**
 * @file runCommandRefactorTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/cli/cliApplication.h"
#include "autoinput/config/config.h"
#include "autoinput/support/errorCode.h"
#include "testUtils.h"

#include <filesystem>
#include <vector>
#include <string>
#include <gtest/gtest.h>


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

        // Ensure parsing CLI arguments with parallel command types succeeds
        ASSERT_TRUE(app.parse(gsl::make_span(argv.data(), argv.size())) == ErrorCode::Success);
        // Ensure executing the parsed CLI command succeeds
        ASSERT_EQ(app.execute(), ErrorCode::Success);

        std::filesystem::path dumpPath = getUserConfigsPath() / "refactor_type.toml";
        auto loaded = loadConfigData(dumpPath);
        // Ensure the generated config file was saved and can be loaded successfully
        ASSERT_TRUE(loaded.has_value());
        // Ensure exactly two commands were created from parallel type associations
        ASSERT_EQ(loaded->commands.size(), 2);
        
        // Verify the action of the first command is hold
        EXPECT_EQ(loaded->commands[0].action, "hold");
        // Ensure buttons list for the first command is not empty
        ASSERT_FALSE(loaded->commands[0].buttons.empty());
        // Verify the target button for the first command is middle
        EXPECT_EQ(loaded->commands[0].buttons[0], "middle");

        // Verify the action of the second command is hold
        EXPECT_EQ(loaded->commands[1].action, "hold");
        // Ensure buttons list for the second command is not empty
        ASSERT_FALSE(loaded->commands[1].buttons.empty());
        // Verify the target button for the second command is right
        EXPECT_EQ(loaded->commands[1].buttons[0], "right");
    }

    TEST_F(RunCommandRefactorTest, ParallelStartKeyAssociation)
    {
        cli::CliApplication app;
        // run --button middle --start f4 --button right --start f5
        std::vector<std::string> argvStr = { "autoinput", "run", "--button", "middle", "--start", "f4", "--button", "right", "--start", "f5", "--save-config", "refactor_start" };
        std::vector<char*> argv;
        for (auto& s : argvStr) argv.push_back(s.data());

        // Ensure parsing CLI arguments with parallel start keys succeeds
        ASSERT_TRUE(app.parse(gsl::make_span(argv.data(), argv.size())) == ErrorCode::Success);
        // Ensure executing the command succeeds
        ASSERT_EQ(app.execute(), ErrorCode::Success);

        std::filesystem::path dumpPath = getUserConfigsPath() / "refactor_start.toml";
        auto loaded = loadConfigData(dumpPath);
        // Ensure the saved config file exists and can be loaded successfully
        ASSERT_TRUE(loaded.has_value());
        // Ensure two commands were created from parallel start key configurations
        ASSERT_EQ(loaded->commands.size(), 2);
        
        // Ensure buttons list for the first command is not empty
        ASSERT_FALSE(loaded->commands[0].buttons.empty());
        // Verify the first command's button is middle
        EXPECT_EQ(loaded->commands[0].buttons[0], "middle");
        // Ensure startKeys list for the first command is not empty
        ASSERT_FALSE(loaded->commands[0].startKeys.empty());
        // Verify the first command's start key is f4
        EXPECT_EQ(loaded->commands[0].startKeys[0], "f4");

        // Ensure buttons list for the second command is not empty
        ASSERT_FALSE(loaded->commands[1].buttons.empty());
        // Verify the second command's button is right
        EXPECT_EQ(loaded->commands[1].buttons[0], "right");
        // Ensure startKeys list for the second command is not empty
        ASSERT_FALSE(loaded->commands[1].startKeys.empty());
        // Verify the second command's start key is f5
        EXPECT_EQ(loaded->commands[1].startKeys[0], "f5");
    }

    TEST_F(RunCommandRefactorTest, MouseModifierPreservation)
    {
        cli::CliApplication app;
        // run --type hold --button shift+left --start f4
        std::vector<std::string> argvStr = { "autoinput", "run", "--type", "hold", "--button", "shift+left", "--start", "f4", "--save-config", "refactor_mod" };
        std::vector<char*> argv;
        for (auto& s : argvStr) argv.push_back(s.data());

        // Ensure parsing CLI arguments with mouse button modifier succeeds
        ASSERT_TRUE(app.parse(gsl::make_span(argv.data(), argv.size())) == ErrorCode::Success);
        // Ensure command execution succeeds
        ASSERT_EQ(app.execute(), ErrorCode::Success);

        std::filesystem::path dumpPath = getUserConfigsPath() / "refactor_mod.toml";
        auto loaded = loadConfigData(dumpPath);
        // Ensure saved config file can be loaded successfully
        ASSERT_TRUE(loaded.has_value());
        // Ensure exactly one command was saved
        ASSERT_EQ(loaded->commands.size(), 1);
        
        // Verify the action type is hold
        EXPECT_EQ(loaded->commands[0].action, "hold");
        // Ensure buttons list is not empty
        ASSERT_FALSE(loaded->commands[0].buttons.empty());
        // Verify the modifier button combination shift+left is preserved
        EXPECT_EQ(loaded->commands[0].buttons[0], "shift+left");
        // Ensure startKeys list is not empty
        ASSERT_FALSE(loaded->commands[0].startKeys.empty());
        // Verify the start key f4 is preserved
        EXPECT_EQ(loaded->commands[0].startKeys[0], "f4");
    }

    TEST_F(RunCommandRefactorTest, KeyTargetPreservation)
    {
        cli::CliApplication app;
        // run --key space --start f6
        std::vector<std::string> argvStr = { "autoinput", "run", "--key", "space", "--start", "f6", "--save-config", "refactor_key" };
        std::vector<char*> argv;
        for (auto& s : argvStr) argv.push_back(s.data());

        // Ensure parsing CLI arguments with key target succeeds
        ASSERT_TRUE(app.parse(gsl::make_span(argv.data(), argv.size())) == ErrorCode::Success);
        // Ensure command execution succeeds
        ASSERT_EQ(app.execute(), ErrorCode::Success);

        std::filesystem::path dumpPath = getUserConfigsPath() / "refactor_key.toml";
        auto loaded = loadConfigData(dumpPath);
        // Ensure saved config file can be loaded successfully
        ASSERT_TRUE(loaded.has_value());
        // Ensure exactly one command was saved
        ASSERT_EQ(loaded->commands.size(), 1);
        
        // Ensure keys list is not empty
        ASSERT_FALSE(loaded->commands[0].keys.empty());
        // Verify the target key space is preserved
        EXPECT_EQ(loaded->commands[0].keys[0], "space");
        // Ensure startKeys list is not empty
        ASSERT_FALSE(loaded->commands[0].startKeys.empty());
        // Verify the start key f6 is preserved
        EXPECT_EQ(loaded->commands[0].startKeys[0], "f6");
    }
}

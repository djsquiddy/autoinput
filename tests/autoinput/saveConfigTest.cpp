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

#include "autoinput/config/config.h"
#include "autoinput/platform/platform.h"
#include "autoinput/support/types.h"
#include "autoinput/cli/cliApplication.h"
#include "autoinput/cli/runCommand.h"
#include "autoinput/cli/commandBase.h"
#include "autoinput/support/errorCode.h"
#include "testUtils.h"

namespace autoinput
{
    class DumpTest : public ::testing::Test
    {
    protected:
        DumpTest()
            : m_tempHome("autoinput_dump_test_home")
#ifdef _WIN32
            , m_scopedHome("USERPROFILE", m_tempHome.path().string())
#else
            , m_scopedHome("HOME", m_tempHome.path().string())
#endif
        {}

        test::TemporaryDirectory m_tempHome;
        test::ScopedEnvironmentVariable m_scopedHome;
    };

    TEST_F(DumpTest, SaveConfigCreatesCorrectFile)
    {
        cli::CliApplication app;
        std::vector<std::string> argvStr = { "autoinput", "run", "--type", "hold", "--button", "right", "-s", "f5", "-e", "f6", "-S", "test_config" };
        std::vector<char*> argv;
        for (auto& s : argvStr) argv.push_back(s.data());

        // Ensure parsing CLI arguments for saving config succeeds
        ASSERT_TRUE(app.parse(gsl::make_span(argv.data(), argv.size())) == ErrorCode::Success);
        // Ensure CLI application execution succeeds
        ASSERT_EQ(app.execute(), ErrorCode::Success);

        std::filesystem::path dumpPath = getUserConfigsPath() / "test_config.toml";
        // Ensure the output configuration file is created at the expected path
        ASSERT_TRUE(std::filesystem::exists(dumpPath));

        auto loaded = loadConfigData(dumpPath);
        // Ensure the saved config file can be loaded successfully
        ASSERT_TRUE(loaded.has_value());
        // Ensure exactly one command was saved in the configuration
        ASSERT_EQ(loaded->commands.size(), 1);
        // Verify the saved command action is 'hold'
        EXPECT_EQ(loaded->commands[0].action, "hold");
        // Ensure exactly one button was saved for the command
        ASSERT_EQ(loaded->commands[0].buttons.size(), 1);
        // Verify the saved button is 'right'
        EXPECT_EQ(loaded->commands[0].buttons[0], "right");
        // Ensure exactly one start key was saved for the command
        ASSERT_EQ(loaded->commands[0].startKeys.size(), 1);
        // Verify the saved start key is 'f5'
        EXPECT_EQ(loaded->commands[0].startKeys[0], "f5");
        // Verify the saved end key is 'f6'
        EXPECT_EQ(loaded->endKey, "f6");
    }

    TEST_F(DumpTest, SaveConfigHandlesWaitTimes)
    {
        cli::CliApplication app;
        std::vector<std::string> argvStr = { "autoinput", "run", "--type", "click", "--button", "left", "-w", "500ms..1s", "--release-wait", "2s", "--save-config", "wait_config" };
        std::vector<char*> argv;
        for (auto& s : argvStr) argv.push_back(s.data());

        // Ensure parsing CLI arguments with wait times succeeds
        ASSERT_TRUE(app.parse(gsl::make_span(argv.data(), argv.size())) == ErrorCode::Success);
        // Ensure executing CLI application succeeds
        ASSERT_EQ(app.execute(), ErrorCode::Success);

        std::filesystem::path dumpPath = getUserConfigsPath() / "wait_config.toml";
        
        auto loaded = loadConfigData(dumpPath);
        // Ensure saved config file with wait times can be loaded
        ASSERT_TRUE(loaded.has_value());
        // Ensure exactly one command was saved
        ASSERT_EQ(loaded->commands.size(), 1);
        // Verify press wait delay range was correctly saved
        EXPECT_EQ(loaded->commands[0].pressWait, "500ms..1s");
        // Verify release wait delay was correctly saved
        EXPECT_EQ(loaded->commands[0].releaseWait, "2s");
    }

    TEST_F(DumpTest, SaveConfigHandlesBlacklist)
    {
        cli::CliApplication app;
        std::vector<std::string> argvStr = { "autoinput", "run", "--button", "left", "-B", "app1.exe", "--blacklist", "app2.exe", "--save-config", "blacklist_config" };
        std::vector<char*> argv;
        for (auto& s : argvStr) argv.push_back(s.data());

        // Ensure parsing CLI arguments with blacklist options succeeds
        ASSERT_TRUE(app.parse(gsl::make_span(argv.data(), argv.size())) == ErrorCode::Success);
        // Ensure executing CLI application succeeds
        ASSERT_EQ(app.execute(), ErrorCode::Success);

        std::filesystem::path dumpPath = getUserConfigsPath() / "blacklist_config.toml";
        
        auto loaded = loadConfigData(dumpPath);
        // Ensure saved blacklist config file can be loaded
        ASSERT_TRUE(loaded.has_value());
        // Ensure both blacklisted applications were saved
        ASSERT_EQ(loaded->blacklist.size(), 2);
        // Verify the first blacklisted executable is 'app1.exe'
        EXPECT_EQ(loaded->blacklist[0], "app1.exe");
        // Verify the second blacklisted executable is 'app2.exe'
        EXPECT_EQ(loaded->blacklist[1], "app2.exe");
    }

    TEST_F(DumpTest, SaveConfigHandlesMultipleCommands)
    {
        cli::CliApplication app;
        // hold middle start f8, hold right start f9
        std::vector<std::string> argvStr = { "autoinput", "run", "--type", "hold", "--button", "middle", "--start", "f8", "--type", "hold", "--button", "right", "--start", "f9", "--save-config", "multi_config" };
        std::vector<char*> argv;
        for (auto& s : argvStr) argv.push_back(s.data());

        // Ensure parsing CLI arguments with multiple commands succeeds
        ASSERT_TRUE(app.parse(gsl::make_span(argv.data(), argv.size())) == ErrorCode::Success);
        // Ensure executing CLI application succeeds
        ASSERT_EQ(app.execute(), ErrorCode::Success);

        std::filesystem::path dumpPath = getUserConfigsPath() / "multi_config.toml";
        
        auto loaded = loadConfigData(dumpPath);
        // Ensure saved multi-command config file can be loaded
        ASSERT_TRUE(loaded.has_value());
        // Ensure two commands were saved to the configuration
        ASSERT_EQ(loaded->commands.size(), 2);
        
        // Verify the action of the first command is 'hold'
        EXPECT_EQ(loaded->commands[0].action, "hold");
        // Ensure one button was saved for the first command
        ASSERT_EQ(loaded->commands[0].buttons.size(), 1);
        // Verify the button for the first command is 'middle'
        EXPECT_EQ(loaded->commands[0].buttons[0], "middle");
        // Ensure one start key was saved for the first command
        ASSERT_EQ(loaded->commands[0].startKeys.size(), 1);
        // Verify the start key for the first command is 'f8'
        EXPECT_EQ(loaded->commands[0].startKeys[0], "f8");

        // Verify the action of the second command is 'hold'
        EXPECT_EQ(loaded->commands[1].action, "hold");
        // Ensure one button was saved for the second command
        ASSERT_EQ(loaded->commands[1].buttons.size(), 1);
        // Verify the button for the second command is 'right'
        EXPECT_EQ(loaded->commands[1].buttons[0], "right");
        // Ensure one start key was saved for the second command
        ASSERT_EQ(loaded->commands[1].startKeys.size(), 1);
        // Verify the start key for the second command is 'f9'
        EXPECT_EQ(loaded->commands[1].startKeys[0], "f9");
    }

    TEST_F(DumpTest, SaveConfigHandlesNameAndExclusiveGroup)
    {
        ConfigData configData;
        CommandData cmd1;
        cmd1.name = "cmd1";
        cmd1.exclusiveGroup = "group1";
        cmd1.action = "click";
        cmd1.buttons.push_back("left");
        cmd1.startKeys.push_back("f2");
        configData.commands.push_back(cmd1);

        CommandData cmd2;
        cmd2.name = "cmd2";
        cmd2.exclusiveGroup = "group1";
        cmd2.action = "click";
        cmd2.buttons.push_back("right");
        cmd2.startKeys.push_back("f4");
        configData.commands.push_back(cmd2);

        std::filesystem::path dumpPath = getUserConfigsPath() / "group_config.toml";
        // Ensure saving configuration with named commands and exclusive groups succeeds
        ASSERT_TRUE(saveConfigData(configData, dumpPath));

        auto loaded = loadConfigData(dumpPath);
        // Ensure saved config file can be loaded
        ASSERT_TRUE(loaded.has_value());
        // Ensure two commands are present in the loaded configuration
        ASSERT_EQ(loaded->commands.size(), 2);
        // Verify the first command's name is 'cmd1'
        EXPECT_EQ(loaded->commands[0].name, "cmd1");
        // Verify the first command's exclusive group is 'group1'
        EXPECT_EQ(loaded->commands[0].exclusiveGroup, "group1");
        // Verify the second command's name is 'cmd2'
        EXPECT_EQ(loaded->commands[1].name, "cmd2");
        // Verify the second command's exclusive group is 'group1'
        EXPECT_EQ(loaded->commands[1].exclusiveGroup, "group1");
    }

    TEST_F(DumpTest, SaveConfigUsesInlineTiming)
    {
        ConfigData configData;
        CommandData cmd;
        cmd.action = "click";
        cmd.buttons = {"left"};
        cmd.pressWait = "100ms";
        cmd.releaseWait = "250ms";
        configData.commands.push_back(cmd);
        configData.endKey = "f6";

        std::filesystem::path dumpPath = m_tempHome.path() / "inline_test.toml";
        // Note: saveConfigData might use getUserConfigsPath internally if only a filename is given, 
        // but here we pass a full path.
        // Ensure saving configuration with timing settings succeeds
        ASSERT_TRUE(saveConfigData(configData, dumpPath));

        std::ifstream file(dumpPath);
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        // Check if it contains the inline table. We accept single or double quotes.
        bool foundDouble = content.find("time = { press = \"100ms\", release = \"250ms\" }") != std::string::npos;
        bool foundSingle = content.find("time = { press = '100ms', release = '250ms' }") != std::string::npos;
        // Verify that the timing table is written in inline format
        EXPECT_TRUE(foundDouble || foundSingle) << "Actual content:\n" << content;

        // And NOT the nested table
        // Verify that the timing table is not written as a nested table header
        EXPECT_EQ(content.find("[command.time]"), std::string::npos);
    }

    TEST_F(DumpTest, SaveConfigUsesInlineTimingOnlyPress)
    {
        ConfigData configData;
        CommandData cmd;
        cmd.action = "click";
        cmd.buttons = {"left"};
        cmd.pressWait = "100ms";
        configData.commands.push_back(cmd);

        std::filesystem::path dumpPath = m_tempHome.path() / "inline_press.toml";
        // Ensure saving configuration with only press timing succeeds
        ASSERT_TRUE(saveConfigData(configData, dumpPath));

        std::ifstream file(dumpPath);
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        bool foundDouble = content.find("time = { press = \"100ms\" }") != std::string::npos;
        bool foundSingle = content.find("time = { press = '100ms' }") != std::string::npos;
        // Verify that the press timing is written as an inline table
        EXPECT_TRUE(foundDouble || foundSingle) << "Actual content:\n" << content;
        // Verify that release timing key is omitted when not specified
        EXPECT_EQ(content.find("release ="), std::string::npos);
    }

    TEST_F(DumpTest, SaveConfigUsesInlineTimingOnlyRelease)
    {
        ConfigData configData;
        CommandData cmd;
        cmd.action = "click";
        cmd.buttons = {"left"};
        cmd.releaseWait = "250ms";
        configData.commands.push_back(cmd);

        std::filesystem::path dumpPath = m_tempHome.path() / "inline_release.toml";
        // Ensure saving configuration with only release timing succeeds
        ASSERT_TRUE(saveConfigData(configData, dumpPath));

        std::ifstream file(dumpPath);
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        bool foundDouble = content.find("time = { release = \"250ms\" }") != std::string::npos;
        bool foundSingle = content.find("time = { release = '250ms' }") != std::string::npos;
        // Verify that the release timing is written as an inline table
        EXPECT_TRUE(foundDouble || foundSingle) << "Actual content:\n" << content;
        // Verify that press timing key is omitted when not specified
        EXPECT_EQ(content.find("press ="), std::string::npos);
    }

    TEST_F(DumpTest, SaveConfigUsesInlineTimingMultipleCommands)
    {
        ConfigData configData;
        {
            CommandData cmd;
            cmd.action = "click";
            cmd.buttons = {"left"};
            cmd.pressWait = "100ms";
            cmd.releaseWait = "250ms";
            configData.commands.push_back(cmd);
        }
        {
            CommandData cmd;
            cmd.action = "hold";
            cmd.buttons = {"right"};
            cmd.pressWait = "50ms";
            cmd.releaseWait = "1s";
            configData.commands.push_back(cmd);
        }

        std::filesystem::path dumpPath = m_tempHome.path() / "inline_multi.toml";
        // Ensure saving multiple commands with inline timing succeeds
        ASSERT_TRUE(saveConfigData(configData, dumpPath));

        std::ifstream file(dumpPath);
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        // Verify the first command's inline timing table was written correctly
        EXPECT_TRUE(content.find("time = { press = '100ms', release = '250ms' }") != std::string::npos ||
                    content.find("time = { press = \"100ms\", release = \"250ms\" }") != std::string::npos);
        // Verify the second command's inline timing table was written correctly
        EXPECT_TRUE(content.find("time = { press = '50ms', release = '1s' }") != std::string::npos ||
                    content.find("time = { press = \"50ms\", release = \"1s\" }") != std::string::npos);
    }

    TEST_F(DumpTest, SaveConfigOmitsTimingIfDefault)
    {
        DefaultSettings defaults;
        defaults.press = "100ms";
        defaults.release = "200ms";

        ConfigData configData;
        CommandData cmd;
        cmd.action = "click";
        cmd.buttons = {"left"};
        cmd.pressWait = "100ms";
        cmd.releaseWait = "200ms";
        configData.commands.push_back(cmd);

        std::filesystem::path dumpPath = m_tempHome.path() / "inline_default.toml";
        // Ensure saving configuration with default settings succeeds
        ASSERT_TRUE(saveConfigData(configData, dumpPath, defaults));

        std::ifstream file(dumpPath);
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        // Verify timing section is omitted when timings match the default values
        EXPECT_EQ(content.find("time ="), std::string::npos);
    }

    TEST_F(DumpTest, SaveAndLoadInlineTiming)
    {
        ConfigData configData;
        CommandData cmd;
        cmd.action = "click";
        cmd.buttons = {"left"};
        cmd.pressWait = "123ms";
        cmd.releaseWait = "456ms";
        configData.commands.push_back(cmd);

        std::filesystem::path dumpPath = m_tempHome.path() / "save_load_test.toml";
        // Ensure saving configuration with custom timing succeeds
        ASSERT_TRUE(saveConfigData(configData, dumpPath));

        auto loaded = loadConfigData(dumpPath);
        // Ensure the saved config file can be loaded back successfully
        ASSERT_TRUE(loaded.has_value());
        // Ensure one command is present in the loaded configuration
        ASSERT_EQ(loaded->commands.size(), 1);
        // Verify the loaded press wait delay matches the saved 123ms value
        EXPECT_EQ(loaded->commands[0].pressWait, "123ms");
        // Verify the loaded release wait delay matches the saved 456ms value
        EXPECT_EQ(loaded->commands[0].releaseWait, "456ms");
    }

    TEST_F(DumpTest, LoadNestedTimingCompatibility)
    {
        std::filesystem::path configPath = m_tempHome.path() / "nested_compat.toml";
        {
            std::ofstream file(configPath);
            file << "[command]\n"
                 << "action = \"click\"\n"
                 << "button = \"left\"\n"
                 << "[command.time]\n"
                 << "press = \"789ms\"\n"
                 << "release = \"1s\"\n";
        }

        auto loaded = loadConfigData(configPath);
        // Ensure configuration file with legacy nested timing table format can be loaded
        ASSERT_TRUE(loaded.has_value());
        // Ensure one command was loaded from nested timing configuration
        ASSERT_EQ(loaded->commands.size(), 1);
        // Verify press wait delay from nested table was parsed correctly
        EXPECT_EQ(loaded->commands[0].pressWait, "789ms");
        // Verify release wait delay from nested table was parsed correctly
        EXPECT_EQ(loaded->commands[0].releaseWait, "1s");
    }
}

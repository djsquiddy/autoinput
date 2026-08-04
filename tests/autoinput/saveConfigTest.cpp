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

#include "autoinput/config.h"
#include "autoinput/platform.h"
#include "autoinput/types.h"
#include "autoinput/cli/cliApplication.h"
#include "autoinput/cli/runCommand.h"
#include "autoinput/cli/commandBase.h"
#include "autoinput/errorCode.h"
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

        ASSERT_TRUE(app.parse(gsl::make_span(argv.data(), argv.size())));
        ASSERT_EQ(app.execute(), static_cast<i32>(ErrorCode::Success));

        std::filesystem::path dumpPath = getUserConfigsPath() / "test_config.toml";
        ASSERT_TRUE(std::filesystem::exists(dumpPath));

        auto loaded = loadConfigData(dumpPath);
        ASSERT_TRUE(loaded.has_value());
        ASSERT_EQ(loaded->commands.size(), 1);
        EXPECT_EQ(loaded->commands[0].action, "hold");
        ASSERT_EQ(loaded->commands[0].buttons.size(), 1);
        EXPECT_EQ(loaded->commands[0].buttons[0], "right");
        ASSERT_EQ(loaded->commands[0].startKeys.size(), 1);
        EXPECT_EQ(loaded->commands[0].startKeys[0], "f5");
        EXPECT_EQ(loaded->endKey, "f6");
    }

    TEST_F(DumpTest, SaveConfigHandlesWaitTimes)
    {
        cli::CliApplication app;
        std::vector<std::string> argvStr = { "autoinput", "run", "--type", "click", "--button", "left", "-w", "500ms..1s", "--release-wait", "2s", "--save-config", "wait_config" };
        std::vector<char*> argv;
        for (auto& s : argvStr) argv.push_back(s.data());

        ASSERT_TRUE(app.parse(gsl::make_span(argv.data(), argv.size())));
        ASSERT_EQ(app.execute(), static_cast<i32>(ErrorCode::Success));

        std::filesystem::path dumpPath = getUserConfigsPath() / "wait_config.toml";
        
        auto loaded = loadConfigData(dumpPath);
        ASSERT_TRUE(loaded.has_value());
        ASSERT_EQ(loaded->commands.size(), 1);
        EXPECT_EQ(loaded->commands[0].pressWait, "500ms..1s");
        EXPECT_EQ(loaded->commands[0].releaseWait, "2s");
    }

    TEST_F(DumpTest, SaveConfigHandlesBlacklist)
    {
        cli::CliApplication app;
        std::vector<std::string> argvStr = { "autoinput", "run", "--button", "left", "-B", "app1.exe", "--blacklist", "app2.exe", "--save-config", "blacklist_config" };
        std::vector<char*> argv;
        for (auto& s : argvStr) argv.push_back(s.data());

        ASSERT_TRUE(app.parse(gsl::make_span(argv.data(), argv.size())));
        ASSERT_EQ(app.execute(), static_cast<i32>(ErrorCode::Success));

        std::filesystem::path dumpPath = getUserConfigsPath() / "blacklist_config.toml";
        
        auto loaded = loadConfigData(dumpPath);
        ASSERT_TRUE(loaded.has_value());
        ASSERT_EQ(loaded->blacklist.size(), 2);
        EXPECT_EQ(loaded->blacklist[0], "app1.exe");
        EXPECT_EQ(loaded->blacklist[1], "app2.exe");
        EXPECT_TRUE(loaded->appendBlacklist);
    }

    TEST_F(DumpTest, SaveConfigHandlesMultipleCommands)
    {
        cli::CliApplication app;
        // hold middle start f8, hold right start f9
        std::vector<std::string> argvStr = { "autoinput", "run", "--type", "hold", "--button", "middle", "--start", "f8", "--type", "hold", "--button", "right", "--start", "f9", "--save-config", "multi_config" };
        std::vector<char*> argv;
        for (auto& s : argvStr) argv.push_back(s.data());

        ASSERT_TRUE(app.parse(gsl::make_span(argv.data(), argv.size())));
        ASSERT_EQ(app.execute(), static_cast<i32>(ErrorCode::Success));

        std::filesystem::path dumpPath = getUserConfigsPath() / "multi_config.toml";
        
        auto loaded = loadConfigData(dumpPath);
        ASSERT_TRUE(loaded.has_value());
        ASSERT_EQ(loaded->commands.size(), 2);
        
        EXPECT_EQ(loaded->commands[0].action, "hold");
        ASSERT_EQ(loaded->commands[0].buttons.size(), 1);
        EXPECT_EQ(loaded->commands[0].buttons[0], "middle");
        ASSERT_EQ(loaded->commands[0].startKeys.size(), 1);
        EXPECT_EQ(loaded->commands[0].startKeys[0], "f8");

        EXPECT_EQ(loaded->commands[1].action, "hold");
        ASSERT_EQ(loaded->commands[1].buttons.size(), 1);
        EXPECT_EQ(loaded->commands[1].buttons[0], "right");
        ASSERT_EQ(loaded->commands[1].startKeys.size(), 1);
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
        ASSERT_TRUE(saveConfigData(configData, dumpPath));

        auto loaded = loadConfigData(dumpPath);
        ASSERT_TRUE(loaded.has_value());
        ASSERT_EQ(loaded->commands.size(), 2);
        EXPECT_EQ(loaded->commands[0].name, "cmd1");
        EXPECT_EQ(loaded->commands[0].exclusiveGroup, "group1");
        EXPECT_EQ(loaded->commands[1].name, "cmd2");
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
        ASSERT_TRUE(saveConfigData(configData, dumpPath));

        std::ifstream file(dumpPath);
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        // Check if it contains the inline table. We accept single or double quotes.
        bool foundDouble = content.find("time = { press = \"100ms\", release = \"250ms\" }") != std::string::npos;
        bool foundSingle = content.find("time = { press = '100ms', release = '250ms' }") != std::string::npos;
        EXPECT_TRUE(foundDouble || foundSingle) << "Actual content:\n" << content;

        // And NOT the nested table
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
        ASSERT_TRUE(saveConfigData(configData, dumpPath));

        std::ifstream file(dumpPath);
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        bool foundDouble = content.find("time = { press = \"100ms\" }") != std::string::npos;
        bool foundSingle = content.find("time = { press = '100ms' }") != std::string::npos;
        EXPECT_TRUE(foundDouble || foundSingle) << "Actual content:\n" << content;
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
        ASSERT_TRUE(saveConfigData(configData, dumpPath));

        std::ifstream file(dumpPath);
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        bool foundDouble = content.find("time = { release = \"250ms\" }") != std::string::npos;
        bool foundSingle = content.find("time = { release = '250ms' }") != std::string::npos;
        EXPECT_TRUE(foundDouble || foundSingle) << "Actual content:\n" << content;
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
        ASSERT_TRUE(saveConfigData(configData, dumpPath));

        std::ifstream file(dumpPath);
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
        EXPECT_TRUE(content.find("time = { press = '100ms', release = '250ms' }") != std::string::npos ||
                    content.find("time = { press = \"100ms\", release = \"250ms\" }") != std::string::npos);
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
        ASSERT_TRUE(saveConfigData(configData, dumpPath, defaults));

        std::ifstream file(dumpPath);
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        
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
        ASSERT_TRUE(saveConfigData(configData, dumpPath));

        auto loaded = loadConfigData(dumpPath);
        ASSERT_TRUE(loaded.has_value());
        ASSERT_EQ(loaded->commands.size(), 1);
        EXPECT_EQ(loaded->commands[0].pressWait, "123ms");
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
        ASSERT_TRUE(loaded.has_value());
        ASSERT_EQ(loaded->commands.size(), 1);
        EXPECT_EQ(loaded->commands[0].pressWait, "789ms");
        EXPECT_EQ(loaded->commands[0].releaseWait, "1s");
    }
}

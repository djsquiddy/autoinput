/**
 * @file configServiceTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include "autoinput/config.h"
#include "autoinput/arguments.h"
#include "testUtils.h"
#include <fstream>

#include "autoinput/services/configService.h"

namespace autoinput
{
    class MockEnvironment : public IEnvironment
    {
    public:
        std::filesystem::path m_exePath;
        std::filesystem::path m_homePath;

        [[nodiscard]] std::filesystem::path executablePath() const override { return m_exePath; }
        [[nodiscard]] std::filesystem::path executableDirectoryPath() const override { return m_exePath.parent_path(); }
        [[nodiscard]] std::filesystem::path userHomePath() const override { return m_homePath; }
        [[nodiscard]] std::optional<std::string> environmentVariable(std::string_view name) const override { return std::nullopt; }
    };

    TEST(ConfigServiceTest, ApplyConfigToArgumentsLoadsCorrectData)
    {
        test::TemporaryDirectory tempDir("config_service_test");
        auto configsDir = tempDir.path() / "configs";
        std::filesystem::create_directories(configsDir);
        
        auto configPath = configsDir / "test_config.toml";
        std::ofstream file(configPath);
        file << R"toml(
[[command]]
name = "test-cmd"
action = "click"
button = "left"
start = "f2"
)toml";
        file.close();

        MockEnvironment env;
        env.m_exePath = tempDir.path() / "test_exe";
        env.m_homePath = tempDir.path();

        services::ConfigService service(env);
        ProgramArguments args;
        
        ASSERT_TRUE(service.applyConfigToArguments("test_config", args));
        
        ASSERT_EQ(args.commandNames.size(), 1);
        EXPECT_EQ(args.commandNames[0], "test-cmd");
        ASSERT_EQ(args.buttons.size(), 1);
        EXPECT_EQ(args.buttons[0].button, MouseButton::Left);
        ASSERT_EQ(args.startKeys.size(), 1);
        EXPECT_EQ(args.startKeys[0], "f2");
    }
}

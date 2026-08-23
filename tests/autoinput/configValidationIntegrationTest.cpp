/**
 * @file configValidationIntegrationTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include <fstream>
#include "autoinput/config/config.h"
#include "autoinput/config/configValidator.h"
#include "testUtils.h"

namespace autoinput
{
    namespace
    {
        std::filesystem::path makeIntegrationConfigFile(const std::filesystem::path& dir, const std::string& fileName, const std::string& contents)
        {
            const std::filesystem::path path = dir / fileName;
            std::ofstream file{ path };
            file << contents;
            file.close();
            return path;
        }
    }

    TEST(ConfigValidationIntegrationTest, ValidConfigIntegrationSucceeds)
    {
        test::TemporaryDirectory tempDir("config_val_int_valid");
        const std::filesystem::path path = makeIntegrationConfigFile(
            tempDir.path(),
            "valid_config.toml",
            R"toml(
end = "f3"
[[command]]
action = "click"
button = "left"
start = "f2"
)toml"
        );

        auto configData = loadConfigData(path);
        // Verify valid configuration file is loaded successfully
        ASSERT_TRUE(configData.has_value());
        auto errors = validateConfigData(*configData);
        // Ensure no validation errors are produced for a valid configuration
        EXPECT_TRUE(errors.empty());
    }

    TEST(ConfigValidationIntegrationTest, InvalidConfigIntegrationFails)
    {
        test::TemporaryDirectory tempDir("config_val_int_invalid");
        const std::filesystem::path path = makeIntegrationConfigFile(
            tempDir.path(),
            "invalid_config.toml",
            R"toml(
end = "f3"
[[command]]
action = "invalid"
button = "left"
start = "f2"
)toml"
        );

        auto configData = loadConfigData(path);
        // Verify invalid configuration file is parsed into config data
        ASSERT_TRUE(configData.has_value());
        auto errors = validateConfigData(*configData);
        // Ensure validation errors are reported for invalid command configuration
        EXPECT_FALSE(errors.empty());
    }

    TEST(ConfigValidationIntegrationTest, MissingConfigIntegrationFails)
    {
        test::TemporaryDirectory tempDir("config_val_int_missing");
        const std::filesystem::path path = tempDir.path() / "missing.toml";

        // Verify nonexistent config file path is reported as not existing
        EXPECT_FALSE(doesConfigDataExists(path));
        auto configData = loadConfigData(path);
        // Ensure loading a missing config file fails to produce a value
        EXPECT_FALSE(configData.has_value());
    }
}

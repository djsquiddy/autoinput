/**
 * @file validateConfigCliTest.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include <chrono>
#ifndef _WIN32
#include <sys/wait.h>
#endif
#include "testUtils.h"

#ifndef AUTOINPUT_EXE_PATH
#error "AUTOINPUT_EXE_PATH must be defined"
#endif

namespace autoinput::test
{
    TEST(ValidateConfigCliTest, ValidConfigReturnsSuccess)
    {
        TemporaryDirectory tempDir("cli_valid");
        std::filesystem::path configPath = tempDir.path() / "valid.toml";
        std::ofstream file(configPath);
        file << "end = \"f3\"\n[[command]]\naction = \"click\"\nbutton = \"left\"\nstart = \"f2\"\n";
        file.close();

        std::string command = quotePath(AUTOINPUT_EXE_PATH) + " config validate " + quotePath(configPath);
        auto result = runCommand(command);

        // Verify CLI exit code indicates success for a valid configuration file
        EXPECT_EQ(result.exitCode, 0);
        // Verify CLI output contains validation success message
        EXPECT_NE(result.output.find("Configuration is valid"), std::string::npos);
    }

    TEST(ValidateConfigCliTest, InvalidConfigReturnsFailure)
    {
        TemporaryDirectory tempDir("cli_invalid");
        std::filesystem::path configPath = tempDir.path() / "invalid.toml";
        std::ofstream file(configPath);
        file << "end = \"f3\"\n[[command]]\naction = \"invalid\"\nbutton = \"left\"\nstart = \"f2\"\n";
        file.close();

        std::string command = quotePath(AUTOINPUT_EXE_PATH) + " config validate " + quotePath(configPath);
        auto result = runCommand(command);

        // Verify CLI exit code indicates error for an invalid configuration file
        EXPECT_NE(result.exitCode, 0);
        // Verify CLI output indicates validation failure
        EXPECT_NE(result.output.find("Configuration validation failed"), std::string::npos) << "Output: " << result.output;
        // Verify specific validation error details are reported in output
        EXPECT_NE(result.output.find("Invalid action: 'invalid'"), std::string::npos);
    }

    TEST(ValidateConfigCliTest, MissingConfigReturnsFailure)
    {
        TemporaryDirectory tempDir("cli_missing");
        std::filesystem::path configPath = tempDir.path() / "non_existent.toml";

        std::string command = quotePath(AUTOINPUT_EXE_PATH) + " config validate " + quotePath(configPath);
        auto result = runCommand(command);

        // Verify CLI exit code indicates error when configuration file does not exist
        EXPECT_NE(result.exitCode, 0);
        // Verify CLI output indicates configuration file was not found
        EXPECT_NE(result.output.find("Configuration file not found"), std::string::npos);
    }

    TEST(ValidateConfigCliTest, ValidationModeExitsQuickly)
    {
        TemporaryDirectory tempDir("cli_quick");
        std::filesystem::path configPath = tempDir.path() / "valid.toml";
        std::ofstream file(configPath);
        file << "end = \"f3\"\n[[command]]\naction = \"click\"\nbutton = \"left\"\nstart = \"f2\"\n";
        file.close();

        auto start = std::chrono::steady_clock::now();
        
        std::string command = quotePath(AUTOINPUT_EXE_PATH) + " config validate " + quotePath(configPath);
        auto result = runCommand(command);
        
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);

        // Verify validation execution succeeds
        EXPECT_EQ(result.exitCode, 0);
        // Validation should take much less than 5 seconds.
        // Verify config validation completes quickly without hanging
        EXPECT_LT(duration.count(), 5);
    }

    TEST(ValidateConfigCliTest, ValidConfigWithJsonReturnsJson)
    {
        TemporaryDirectory tempDir("cli_valid_json");
        std::filesystem::path configPath = tempDir.path() / "valid.toml";
        std::ofstream file(configPath);
        file << "end = \"f3\"\n[[command]]\naction = \"click\"\nbutton = \"left\"\nstart = \"f2\"\n";
        file.close();

        std::string command = quotePath(AUTOINPUT_EXE_PATH) + " config validate " + quotePath(configPath) + " --json";
        auto [exitCode, output] = runCommand(command);
        
        // Verify CLI exit code indicates success when validating valid config with --json flag
        EXPECT_EQ(exitCode, 0);
        // Verify JSON response indicates valid configuration
        EXPECT_NE(output.find("\"valid\": true"), std::string::npos);
        // Verify JSON response contains an empty errors array
        EXPECT_NE(output.find("\"errors\": []"), std::string::npos) << "Output: " << output;
        // Verify JSON response includes the validated configuration filename
        EXPECT_NE(output.find(configPath.filename().string()), std::string::npos);
    }

    TEST(ValidateConfigCliTest, InvalidConfigWithJsonReturnsJson)
    {
        TemporaryDirectory tempDir("cli_invalid_json");
        std::filesystem::path configPath = tempDir.path() / "invalid.toml";
        std::ofstream file(configPath);
        file << "end = \"f3\"\n[[command]]\naction = \"invalid\"\nbutton = \"left\"\nstart = \"f2\"\n";
        file.close();

        std::string command = quotePath(AUTOINPUT_EXE_PATH) + " config validate " + quotePath(configPath) + " --json";
        auto [exitCode, output] = runCommand(command);

        // Verify CLI exit code indicates failure when validating invalid config with --json flag
        EXPECT_NE(exitCode, 0);
        // Verify JSON response indicates invalid configuration
        EXPECT_NE(output.find("\"valid\": false"), std::string::npos);
        // Verify specific validation error message is present in JSON output
        EXPECT_NE(output.find("Invalid action: 'invalid'"), std::string::npos);
    }

    TEST(ValidateConfigCliTest, MissingConfigWithJsonReturnsJson)
    {
        TemporaryDirectory tempDir("cli_missing_json");
        std::filesystem::path configPath = tempDir.path() / "non_existent.toml";

        std::string command = quotePath(AUTOINPUT_EXE_PATH) + " --json config validate " + quotePath(configPath);
        auto [exitCode, output] = runCommand(command);

        // Verify CLI exit code indicates failure when config file is missing in JSON mode
        EXPECT_NE(exitCode, 0);
        // Verify JSON response indicates invalid status
        EXPECT_NE(output.find("\"valid\": false"), std::string::npos);
        // Verify JSON response contains missing file error message
        EXPECT_NE(output.find("Configuration file not found"), std::string::npos);
    }

    TEST(ValidateConfigCliTest, JsonOutputIsClean)
    {
        TemporaryDirectory tempDir("cli_clean_json");
        std::filesystem::path configPath = tempDir.path() / "valid.toml";
        std::ofstream file(configPath);
        file << "end = \"f3\"\n[[command]]\naction = \"click\"\nbutton = \"left\"\nstart = \"f2\"\n";
        file.close();

        // Try both orderings
        const std::vector<std::string> commands = {
            quotePath(AUTOINPUT_EXE_PATH) + " config validate " + quotePath(configPath) + " --json",
            quotePath(AUTOINPUT_EXE_PATH) + " --json config validate " + quotePath(configPath)
        };

        for (const auto& command : commands)
        {
            auto [exitCode, output] = runCommand(command);
            // Verify command execution succeeds for both flag positions
            EXPECT_EQ(exitCode, 0);
            
            // The output should not contain any warnings
            // Verify output does not contain any warning logs
            EXPECT_EQ(output.find("[WARNING]"), std::string::npos) << "Found warning in command: " << command;
            // Verify output does not complain about unknown --json argument
            EXPECT_EQ(output.find("Unknown argument: --json"), std::string::npos) << "Found unknown argument warning in command: " << command;
            
            // The output should start with a JSON object (ignoring whitespace)
            size_t firstBrace = output.find('{');
            // Ensure output contains an opening JSON brace
            ASSERT_NE(firstBrace, std::string::npos) << "Could not find '{' in command: " << command;
            std::string prefix = output.substr(0, firstBrace);
            
            // Check that prefix only contains whitespace
            for (char c : prefix)
            {
                // Verify all characters preceding the JSON opening brace are whitespace
                EXPECT_TRUE(std::isspace(static_cast<unsigned char>(c))) << "Non-whitespace character found before '{': '" << c << "' in command: " << command << " prefix: " <<prefix;
            }
        }
    }
}

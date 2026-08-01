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
    struct CommandResult
    {
        int exitCode;
        std::string output;
    };

    std::string quotePath(std::filesystem::path path)
    {
        return "\"" + path.make_preferred().string() + "\"";
    }

    CommandResult runCommand(const std::string& command)
    {
        // Use a temporary file to capture the output
        TemporaryDirectory tempDir("run_command_tmp");
        std::filesystem::path outputPath = tempDir.path() / "output.txt";
        
        // Redirect stdout and stderr to the temporary file
        std::string fullCommand = command + " > " + quotePath(outputPath) + " 2>&1";
        
#ifdef _WIN32
        // On Windows, if there are multiple quotes in the command, cmd /c needs the whole thing quoted.
        std::string cmdWrapper = "cmd /c \"" + fullCommand + "\"";
        int rawExitCode = std::system(cmdWrapper.c_str());
#else
        int rawExitCode = std::system(fullCommand.c_str());
#endif
        
        // On Windows, std::system returns the exit code directly.
        // On POSIX, we would need WEXITSTATUS(rawExitCode).
        int exitCode = rawExitCode;
#ifndef _WIN32
        if (WIFEXITED(rawExitCode)) {
            exitCode = WEXITSTATUS(rawExitCode);
        }
#endif
        
        std::string output;
        if (std::filesystem::exists(outputPath))
        {
            std::ifstream file(outputPath);
            output.assign((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        }
        
        return { exitCode, output };
    }

    TEST(ValidateConfigCliTest, ValidConfigReturnsSuccess)
    {
        TemporaryDirectory tempDir("cli_valid");
        std::filesystem::path configPath = tempDir.path() / "valid.toml";
        std::ofstream file(configPath);
        file << "end = \"f3\"\n[[command]]\naction = \"click\"\nbutton = \"left\"\nstart = \"f2\"\n";
        file.close();

        std::string command = quotePath(AUTOINPUT_EXE_PATH) + " --validate-config " + quotePath(configPath);
        auto result = runCommand(command);

        EXPECT_EQ(result.exitCode, 0);
        EXPECT_NE(result.output.find("Configuration is valid"), std::string::npos);
    }

    TEST(ValidateConfigCliTest, InvalidConfigReturnsFailure)
    {
        TemporaryDirectory tempDir("cli_invalid");
        std::filesystem::path configPath = tempDir.path() / "invalid.toml";
        std::ofstream file(configPath);
        file << "end = \"f3\"\n[[command]]\naction = \"invalid\"\nbutton = \"left\"\nstart = \"f2\"\n";
        file.close();

        std::string command = quotePath(AUTOINPUT_EXE_PATH) + " --validate-config " + quotePath(configPath);
        auto result = runCommand(command);

        EXPECT_NE(result.exitCode, 0);
        EXPECT_NE(result.output.find("Configuration validation failed"), std::string::npos);
        EXPECT_NE(result.output.find("Invalid action: 'invalid'"), std::string::npos);
    }

    TEST(ValidateConfigCliTest, MissingConfigReturnsFailure)
    {
        TemporaryDirectory tempDir("cli_missing");
        std::filesystem::path configPath = tempDir.path() / "non_existent.toml";

        std::string command = quotePath(AUTOINPUT_EXE_PATH) + " --validate-config " + quotePath(configPath);
        auto result = runCommand(command);

        EXPECT_NE(result.exitCode, 0);
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
        
        std::string command = quotePath(AUTOINPUT_EXE_PATH) + " --validate-config " + quotePath(configPath);
        auto result = runCommand(command);
        
        auto end = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);

        EXPECT_EQ(result.exitCode, 0);
        // Validation should take much less than 5 seconds.
        EXPECT_LT(duration.count(), 5);
    }

    TEST(ValidateConfigCliTest, ValidConfigWithJsonReturnsJson)
    {
        TemporaryDirectory tempDir("cli_valid_json");
        std::filesystem::path configPath = tempDir.path() / "valid.toml";
        std::ofstream file(configPath);
        file << "end = \"f3\"\n[[command]]\naction = \"click\"\nbutton = \"left\"\nstart = \"f2\"\n";
        file.close();

        std::string command = quotePath(AUTOINPUT_EXE_PATH) + " --validate-config " + quotePath(configPath) + " --json";
        auto result = runCommand(command);

        EXPECT_EQ(result.exitCode, 0);
        EXPECT_NE(result.output.find("\"valid\": true"), std::string::npos);
        EXPECT_NE(result.output.find("\"errors\": []"), std::string::npos);
        EXPECT_NE(result.output.find(configPath.filename().string()), std::string::npos);
    }

    TEST(ValidateConfigCliTest, InvalidConfigWithJsonReturnsJson)
    {
        TemporaryDirectory tempDir("cli_invalid_json");
        std::filesystem::path configPath = tempDir.path() / "invalid.toml";
        std::ofstream file(configPath);
        file << "end = \"f3\"\n[[command]]\naction = \"invalid\"\nbutton = \"left\"\nstart = \"f2\"\n";
        file.close();

        std::string command = quotePath(AUTOINPUT_EXE_PATH) + " --validate-config " + quotePath(configPath) + " --json";
        auto result = runCommand(command);

        EXPECT_NE(result.exitCode, 0);
        EXPECT_NE(result.output.find("\"valid\": false"), std::string::npos);
        EXPECT_NE(result.output.find("Invalid action: 'invalid'"), std::string::npos);
    }

    TEST(ValidateConfigCliTest, MissingConfigWithJsonReturnsJson)
    {
        TemporaryDirectory tempDir("cli_missing_json");
        std::filesystem::path configPath = tempDir.path() / "non_existent.toml";

        std::string command = quotePath(AUTOINPUT_EXE_PATH) + " --json --validate-config " + quotePath(configPath);
        auto result = runCommand(command);

        EXPECT_NE(result.exitCode, 0);
        EXPECT_NE(result.output.find("\"valid\": false"), std::string::npos);
        EXPECT_NE(result.output.find("Configuration file not found"), std::string::npos);
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
            quotePath(AUTOINPUT_EXE_PATH) + " --validate-config " + quotePath(configPath) + " --json",
            quotePath(AUTOINPUT_EXE_PATH) + " --json --validate-config " + quotePath(configPath)
        };

        for (const auto& command : commands)
        {
            auto result = runCommand(command);
            EXPECT_EQ(result.exitCode, 0);
            
            // The output should not contain any warnings
            EXPECT_EQ(result.output.find("[WARNING]"), std::string::npos) << "Found warning in command: " << command;
            EXPECT_EQ(result.output.find("Unknown argument: --json"), std::string::npos) << "Found unknown argument warning in command: " << command;
            
            // The output should start with a JSON object (ignoring whitespace)
            size_t firstBrace = result.output.find('{');
            ASSERT_NE(firstBrace, std::string::npos) << "Could not find '{' in command: " << command;
            std::string prefix = result.output.substr(0, firstBrace);
            
            // Check that prefix only contains whitespace
            for (char c : prefix)
            {
                EXPECT_TRUE(std::isspace(static_cast<unsigned char>(c))) << "Non-whitespace character found before '{': '" << c << "' in command: " << command;
            }
        }
    }
}

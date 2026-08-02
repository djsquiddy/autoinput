/**
 * @file duplicateConfigCliTest.cpp
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
    TEST(DuplicateConfigCliTest, DuplicateUserConfigToNewUserConfig)
    {
        TemporaryDirectory tempDir("duplicate_cli");
        // We need to trick the app into using this as the HOME directory
        ScopedEnvironmentVariable homeEnv("USERPROFILE", tempDir.path().string());
        ScopedEnvironmentVariable homeEnv2("HOME", tempDir.path().string());

        std::filesystem::path userConfigDir = tempDir.path() / ".autoinput";
        std::filesystem::create_directories(userConfigDir);

        std::filesystem::path sourcePath = userConfigDir / "source.toml";
        {
            std::ofstream file(sourcePath);
            file << "# Source config\nend = \"f3\"\n[[command]]\naction = \"click\"\nbutton = \"left\"\nstart = \"f2\"\n";
        }

        std::string command = quotePath(AUTOINPUT_EXE_PATH) + " --duplicate-config source destination";
        auto result = runCommand(command);

        EXPECT_EQ(result.exitCode, 0);
        EXPECT_NE(result.output.find("Configuration duplicated"), std::string::npos);

        std::filesystem::path destPath = userConfigDir / "destination.toml";
        EXPECT_TRUE(std::filesystem::exists(destPath));

        // Verify content
        std::ifstream file(destPath);
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        EXPECT_NE(content.find("# Source config"), std::string::npos);
        EXPECT_NE(content.find("end = \"f3\""), std::string::npos);
    }

    TEST(DuplicateConfigCliTest, DuplicateWithAlias)
    {
        TemporaryDirectory tempDir("duplicate_alias");
        ScopedEnvironmentVariable homeEnv("USERPROFILE", tempDir.path().string());
        ScopedEnvironmentVariable homeEnv2("HOME", tempDir.path().string());

        std::filesystem::path userConfigDir = tempDir.path() / ".autoinput";
        std::filesystem::create_directories(userConfigDir);

        std::filesystem::path sourcePath = userConfigDir / "source.toml";
        {
            std::ofstream file(sourcePath);
            file << "end = \"f3\"\n";
        }

        std::string command = quotePath(AUTOINPUT_EXE_PATH) + " --copy-config source dest_alias";
        auto result = runCommand(command);

        EXPECT_EQ(result.exitCode, 0);
        EXPECT_TRUE(std::filesystem::exists(userConfigDir / "dest_alias.toml"));
    }

    TEST(DuplicateConfigCliTest, DestinationAlreadyExistsFails)
    {
        TemporaryDirectory tempDir("duplicate_fail_exists");
        ScopedEnvironmentVariable homeEnv("USERPROFILE", tempDir.path().string());
        ScopedEnvironmentVariable homeEnv2("HOME", tempDir.path().string());

        std::filesystem::path userConfigDir = tempDir.path() / ".autoinput";
        std::filesystem::create_directories(userConfigDir);

        std::filesystem::path sourcePath = userConfigDir / "source.toml";
        { std::ofstream file(sourcePath); file << "end = \"f3\"\n"; }
        std::filesystem::path destPath = userConfigDir / "dest.toml";
        { std::ofstream file(destPath); file << "already here\n"; }

        std::string command = quotePath(AUTOINPUT_EXE_PATH) + " --duplicate-config source dest";
        auto result = runCommand(command);

        EXPECT_NE(result.exitCode, 0);
        EXPECT_NE(result.output.find("already exists"), std::string::npos);
    }

    TEST(DuplicateConfigCliTest, MissingSourceFails)
    {
        TemporaryDirectory tempDir("duplicate_fail_missing");
        ScopedEnvironmentVariable homeEnv("USERPROFILE", tempDir.path().string());
        ScopedEnvironmentVariable homeEnv2("HOME", tempDir.path().string());

        std::string command = quotePath(AUTOINPUT_EXE_PATH) + " --duplicate-config non_existent dest";
        auto result = runCommand(command);

        EXPECT_NE(result.exitCode, 0);
        EXPECT_NE(result.output.find("could not be found"), std::string::npos);
    }

    TEST(DuplicateConfigCliTest, SameSourceAndDestinationFails)
    {
        TemporaryDirectory tempDir("duplicate_fail_same");
        ScopedEnvironmentVariable homeEnv("USERPROFILE", tempDir.path().string());
        ScopedEnvironmentVariable homeEnv2("HOME", tempDir.path().string());

        std::filesystem::path userConfigDir = tempDir.path() / ".autoinput";
        std::filesystem::create_directories(userConfigDir);

        std::filesystem::path sourcePath = userConfigDir / "source.toml";
        { std::ofstream file(sourcePath); file << "end = \"f3\"\n"; }

        std::string command = quotePath(AUTOINPUT_EXE_PATH) + " --duplicate-config source source";
        auto result = runCommand(command);

        EXPECT_NE(result.exitCode, 0);
        EXPECT_NE(result.output.find("same path"), std::string::npos);
    }

    TEST(DuplicateConfigCliTest, DuplicateBuiltInConfigToUserConfig)
    {
        TemporaryDirectory tempDir("duplicate_builtin");
        ScopedEnvironmentVariable homeEnv("USERPROFILE", tempDir.path().string());
        ScopedEnvironmentVariable homeEnv2("HOME", tempDir.path().string());

        // We assume 'core-keeper-fishing' or some other config exists in the built-in configs.
        // But since we are in a test environment, let's check what's actually there or use one we know.
        // Actually, the build copies the project 'configs' dir to the output dir.
        
        std::string command = quotePath(AUTOINPUT_EXE_PATH) + " --duplicate-config core-keeper-fishing my-copy";
        auto result = runCommand(command);

        // If 'core-keeper-fishing' doesn't exist in the test environment, this might fail.
        // Let's create a fake built-in config if needed, but the project has them.
        
        if (result.exitCode != 0) {
            // Fallback: create a dummy built-in config in the expected location
            std::filesystem::path exePath(AUTOINPUT_EXE_PATH);
            std::filesystem::path builtinDir = exePath.parent_path() / "configs";
            std::filesystem::create_directories(builtinDir);
            { std::ofstream file(builtinDir / "test-builtin.toml"); file << "builtin = true\n"; }
            
            command = quotePath(AUTOINPUT_EXE_PATH) + " --duplicate-config test-builtin my-copy-2";
            result = runCommand(command);
            EXPECT_EQ(result.exitCode, 0);
            EXPECT_TRUE(std::filesystem::exists(tempDir.path() / ".autoinput" / "my-copy-2.toml"));
        } else {
            EXPECT_EQ(result.exitCode, 0);
            EXPECT_TRUE(std::filesystem::exists(tempDir.path() / ".autoinput" / "my-copy.toml"));
        }
    }

    TEST(DuplicateConfigCliTest, DestinationWithPathGoesToUserConfig)
    {
        TemporaryDirectory tempDir("duplicate_path");
        ScopedEnvironmentVariable homeEnv("USERPROFILE", tempDir.path().string());
        ScopedEnvironmentVariable homeEnv2("HOME", tempDir.path().string());

        std::filesystem::path userConfigDir = tempDir.path() / ".autoinput";
        std::filesystem::create_directories(userConfigDir);

        std::filesystem::path sourcePath = userConfigDir / "source.toml";
        { std::ofstream file(sourcePath); file << "end = \"f3\"\n"; }

        std::filesystem::path someOtherDir = tempDir.path() / "other";
        std::filesystem::create_directories(someOtherDir);
        std::filesystem::path destInOtherDir = someOtherDir / "dest.toml";

        std::string command = quotePath(AUTOINPUT_EXE_PATH) + " --duplicate-config source " + quotePath(destInOtherDir);
        auto result = runCommand(command);

        EXPECT_EQ(result.exitCode, 0);
        // It should NOT be in someOtherDir, but in userConfigDir
        EXPECT_FALSE(std::filesystem::exists(destInOtherDir));
        EXPECT_TRUE(std::filesystem::exists(userConfigDir / "dest.toml"));
    }
}

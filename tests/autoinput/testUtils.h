/**
 * @file testUtils.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_TEST_UTILS_H
#define INCLUDE_AUTOINPUT_TEST_UTILS_H
#pragma once

#include <string>
#include <filesystem>
#include <memory>
#include <optional>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <random>
#include <fstream>

#include "autoinput/platform.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <stdlib.h>
#include <sys/wait.h>
#endif

#include "autoinput/backend.h"
#include "autoinput/environment.h"
#include <map>

namespace autoinput::test
{
    /**
     * @brief A fake environment for testing.
     */
    class FakeEnvironment : public IEnvironment
    {
    public:
        std::filesystem::path executablePath() const override { return m_executablePath; }
            std::filesystem::path executableDirectoryPath() const override
            {
                if (!m_executableDirectoryPath.empty())
                {
                    return m_executableDirectoryPath;
                }

                return m_executablePath.has_filename()
                    ? m_executablePath.parent_path()
                    : m_executablePath;
            }

        std::filesystem::path userHomePath() const override { return m_userHomePath; }
        std::optional<std::string> environmentVariable(std::string_view name) const override
        {
            auto it = m_variables.find(std::string(name));
            if (it != m_variables.end())
            {
                return it->second;
            }
            return std::nullopt;
        }

        bool openPath(const std::filesystem::path& path) const override { return true; }

        void setExecutablePath(std::filesystem::path path) { m_executablePath = std::move(path); }
        void setExecutableDirectoryPath(std::filesystem::path path) { m_executableDirectoryPath = std::move(path); }
        void setUserHomePath(std::filesystem::path path) { m_userHomePath = std::move(path); }
        void setEnvironmentVariable(std::string name, std::string value) { m_variables[std::move(name)] = std::move(value); }

    private:
        std::filesystem::path m_executablePath;
        std::filesystem::path m_executableDirectoryPath;
        std::filesystem::path m_userHomePath;
        std::map<std::string, std::string, std::less<>> m_variables;
    };

    /**
     * @brief Helper to temporarily set an environment variable and restore it on destruction.
     */
    class ScopedEnvironmentVariable
    {
    public:
        ScopedEnvironmentVariable(const std::string& name, std::optional<std::string> value)
            : m_name(name)
        {
            const char* oldValue = std::getenv(name.c_str());
            if (oldValue)
            {
                m_oldValue = oldValue;
            }

            if (value)
            {
                setEnv(name, *value);
            }
            else
            {
                unsetEnv(name);
            }
        }

        ~ScopedEnvironmentVariable()
        {
            if (m_oldValue)
            {
                setEnv(m_name, *m_oldValue);
            }
            else
            {
                unsetEnv(m_name);
            }
        }

    private:
        static void setEnv(const std::string& name, const std::string& value)
        {
#ifdef _WIN32
            SetEnvironmentVariableA(name.c_str(), value.c_str());
            _putenv((name + "=" + value).c_str());
#else
            setenv(name.c_str(), value.c_str(), 1);
#endif
        }

        static void unsetEnv(const std::string& name)
        {
#ifdef _WIN32
            SetEnvironmentVariableA(name.c_str(), nullptr);
            _putenv((name + "=").c_str());
#else
            unsetenv(name.c_str());
#endif
        }

        std::string m_name;
        std::optional<std::string> m_oldValue;
    };

    /**
     * @brief Helper to create a unique temporary directory and remove it on destruction.
     */
    class TemporaryDirectory
    {
    public:
        explicit TemporaryDirectory(const std::string& prefix = "autoinput_test_")
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<unsigned int> dis(0, 0xFFFFFFFF);

            for (int attempt = 0; attempt < 10; ++attempt)
            {
                auto now = std::chrono::system_clock::now().time_since_epoch().count();
                auto pid = 0;
#ifdef _WIN32
                pid = GetCurrentProcessId();
#else
                pid = getpid();
#endif
                auto tid = std::hash<std::thread::id>{}(std::this_thread::get_id());
                auto randVal = dis(gen);

                std::string uniqueName = prefix + 
                                       std::to_string(now) + "_" + 
                                       std::to_string(pid) + "_" + 
                                       std::to_string(tid) + "_" + 
                                       std::to_string(randVal);
                
                m_path = std::filesystem::temp_directory_path() / uniqueName;

                std::error_code ec;
                if (std::filesystem::create_directories(m_path, ec))
                {
                    return;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }

            // Fallback or throw if all attempts fail
            throw std::runtime_error("Failed to create unique temporary directory after 10 attempts: " + m_path.string());
        }

        ~TemporaryDirectory()
        {
            std::error_code ec;
            std::filesystem::remove_all(m_path, ec);
        }

        const std::filesystem::path& path() const { return m_path; }
        
        // Helper to get a path inside the temporary directory
        std::filesystem::path operator/(const std::string& subPath) const
        {
            return m_path / subPath;
        }

        operator std::filesystem::path() const
        {
            return m_path;
        }

    private:
        std::filesystem::path m_path;
    };

    struct CommandResult
    {
        int exitCode;
        std::string output;
    };

    inline std::string quotePath(std::filesystem::path path)
    {
        return "\"" + path.make_preferred().string() + "\"";
    }

    inline CommandResult runCommand(const std::string& command)
    {
        TemporaryDirectory tempDir("run_command_tmp");
        std::filesystem::path outputPath = tempDir.path() / "output.txt";
        
        std::string fullCommand = command + " > " + quotePath(outputPath) + " 2>&1";
        
#ifdef _WIN32
        // On Windows, std::system calls cmd.exe /c.
        // cmd.exe /c has a special rule: if the command starts and ends with quotes, they are removed.
        // To ensure our nested quotes are preserved, we MUST wrap the entire command in another set of quotes.
        std::string cmdWrapper = "\"" + fullCommand + "\"";
        int rawExitCode = std::system(cmdWrapper.c_str());
#else
        int rawExitCode = std::system(fullCommand.c_str());
#endif
        
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
}

#endif // INCLUDE_AUTOINPUT_TEST_UTILS_H

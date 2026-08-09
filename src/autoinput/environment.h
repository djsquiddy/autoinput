/**
 * @file environment.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_ENVIRONMENT_H
#define INCLUDE_AUTOINPUT_ENVIRONMENT_H
#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace autoinput
{
    /**
     * @brief Interface for environment-related operations.
     */
    class IEnvironment
    {
    public:
        virtual ~IEnvironment() = default;

        /**
         * @brief Gets the path to the current executable.
         * @return The path to the executable.
         */
        [[nodiscard]] virtual std::filesystem::path executablePath() const = 0;

        /**
         * @brief Gets the path to the current executable directory.
         * @return The path to the executable directory.
         */
        [[nodiscard]] virtual std::filesystem::path executableDirectoryPath() const = 0;

        /**
         * @brief Gets the path to the user's home directory.
         * @return The path to the user's home directory.
         */
        [[nodiscard]] virtual std::filesystem::path userHomePath() const = 0;

        /**
         * @brief Gets the value of an environment variable.
         * @param name The name of the environment variable.
         * @return The value of the environment variable, or std::nullopt if not found.
         */
        [[nodiscard]] virtual std::optional<std::string> environmentVariable(std::string_view name) const = 0;

        /**
         * @brief Gets the display name of the current platform.
         * @return The platform name.
         */
        [[nodiscard]] virtual std::string platformName() const = 0;

        /**
         * @brief Opens a file or folder using the platform's default application.
         * @param path The path to open.
         * @return True if successful.
         */
        virtual bool openPath(const std::filesystem::path& path) const = 0;
    };

    /**
     * @brief Implementation of IEnvironment that uses actual system calls.
     */
    class SystemEnvironment : public IEnvironment
    {
    public:
        [[nodiscard]] std::filesystem::path executablePath() const override;
        [[nodiscard]] std::filesystem::path executableDirectoryPath() const override;
        [[nodiscard]] std::filesystem::path userHomePath() const override;
        [[nodiscard]] std::optional<std::string> environmentVariable(std::string_view name) const override;
        [[nodiscard]] std::string platformName() const override;
        bool openPath(const std::filesystem::path& path) const override;

        /**
         * @brief Gets the singleton instance of SystemEnvironment.
         * @return The SystemEnvironment instance.
         */
        static const SystemEnvironment& instance();
    };
}

#endif // INCLUDE_AUTOINPUT_ENVIRONMENT_H

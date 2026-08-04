/**
 * @file platform.h
 * @author djsquiddy
 * @date April 2026
 */
#ifndef INCLUDE_AUTOINPUT_PLATFORM_H
#define INCLUDE_AUTOINPUT_PLATFORM_H
#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>

namespace autoinput
{
    struct Key;
    class INotificationSink;

    namespace platform
    {
        /**
         * @brief Signals the program to end execution.
         */
        void signalEnd();

        /**
         * @brief Sets up handlers for system signals (e.g. SIGINT).
         */
        void setupSignalHandler();

        /**
         * @brief Translates a Key object to a platform-specific virtual key code.
         * @param key The key to translate.
         * @return The virtual key code.
         */
        int32_t getVirtualKey(const Key& key);

        /**
         * @brief Gets the name of the application that currently has focus.
         * @return The application name.
         */
        std::string getActiveApplicationName();

        /**
         * @brief Gets a list of names of all currently running applications.
         * @return A vector of application names.
         */
        std::vector<std::string> getRunningApplicationNames();

        /**
         * @brief Gets the path to the current executable.
         * @return The executable path.
         */
        std::filesystem::path getExecutablePath();

        /**
         * @brief Gets the path to the user's home directory.
         * @return The home directory path.
         */
        std::filesystem::path getUserHomePath();

        /**
         * @brief Creates a platform-specific desktop notification sink.
         * @return A unique pointer to the notification sink.
         */
        std::unique_ptr<INotificationSink> createDesktopNotificationSink();
    }
}

#endif // INCLUDE_AUTOINPUT_PLATFORM_H

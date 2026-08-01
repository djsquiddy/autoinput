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

    namespace platform
    {
        void signalEnd();
        void setupSignalHandler();
        int32_t getVirtualKey(const Key& key);
        std::string getActiveApplicationName();
        std::vector<std::string> getRunningApplicationNames();
        std::filesystem::path getExecutablePath();
        std::filesystem::path getUserHomePath();
    }
}

#endif // INCLUDE_AUTOINPUT_PLATFORM_H

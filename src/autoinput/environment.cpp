/**
 * @file environment.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/environment.h"
#include "autoinput/platform.h"
#include <cstdlib>

namespace autoinput
{
    std::filesystem::path SystemEnvironment::executablePath() const
    {
        return platform::getExecutablePath();
    }

    std::filesystem::path SystemEnvironment::executableDirectoryPath() const
    {
        return platform::getExecutableDirectoryPath();
    }

    std::filesystem::path SystemEnvironment::userHomePath() const
    {
        return platform::getUserHomePath();
    }

    std::optional<std::string> SystemEnvironment::environmentVariable(std::string_view name) const
    {
        if (const char* val = std::getenv(name.data()))
        {
            return std::string(val);
        }
        return std::nullopt;
    }

    const SystemEnvironment& SystemEnvironment::instance()
    {
        static SystemEnvironment instance;
        return instance;
    }
}

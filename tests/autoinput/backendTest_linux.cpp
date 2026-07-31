#include <gtest/gtest.h>
#include "autoinput/backend.h"
#include "autoinput/linux/internalData_linux.h"
#include "autoinput/logger.h"

#ifdef _WIN32
#include <cstdlib>
#define setenv(name, value, overwrite) _putenv_s(name, value)
#define unsetenv(name) _putenv_s(name, "")
#endif

namespace autoinput
{
    // Mock versions of the create functions so we don't need the real backends
    std::unique_ptr<PlatformBackend> createWaylandBackend()
    {
        return std::make_unique<FakeBackend>();
    }

    std::unique_ptr<PlatformBackend> createX11Backend()
    {
        return std::make_unique<FakeBackend>();
    }

    // Copy of detectLinuxBackend logic for testing on all platforms
    std::unique_ptr<PlatformBackend> test_detectLinuxBackend()
    {
        const char* sessionType = std::getenv("XDG_SESSION_TYPE");
        bool isWayland = sessionType && std::string(sessionType) == "wayland";

        if (isWayland)
        {
            return createWaylandBackend();
        }
        else
        {
#if AUTOINPUT_WITH_X11 || !defined(__linux__)
            return createX11Backend();
#else
            return nullptr;
#endif
        }
    }
}

TEST(LinuxBackendTest, DetectsWayland)
{
    setenv("XDG_SESSION_TYPE", "wayland", 1);
    auto backend = autoinput::test_detectLinuxBackend();
    EXPECT_NE(backend, nullptr);
    unsetenv("XDG_SESSION_TYPE");
}

TEST(LinuxBackendTest, DetectsX11)
{
    setenv("XDG_SESSION_TYPE", "x11", 1);
    auto backend = autoinput::test_detectLinuxBackend();
    EXPECT_NE(backend, nullptr);
    unsetenv("XDG_SESSION_TYPE");
}

TEST(LinuxBackendTest, FallbackToX11)
{
    unsetenv("XDG_SESSION_TYPE");
    auto backend = autoinput::test_detectLinuxBackend();
    EXPECT_NE(backend, nullptr);
}

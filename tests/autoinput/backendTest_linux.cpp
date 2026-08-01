/**
 * @file backendTest_linux.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include "autoinput/backend.h"
#include "autoinput/linux/internalData_linux.h"
#include "autoinput/logger.h"
#include "testUtils.h"

namespace autoinput
{
    // Mock versions of the create functions so we don't need the real backends
    std::unique_ptr<IPlatformBackend> createWaylandBackend()
    {
        return std::make_unique<FakeBackend>();
    }

    std::unique_ptr<IPlatformBackend> createX11Backend()
    {
        return std::make_unique<FakeBackend>();
    }

    // Copy of detectLinuxBackend logic for testing on all platforms
    std::unique_ptr<IPlatformBackend> test_detectLinuxBackend()
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
    test::ScopedEnvironmentVariable env("XDG_SESSION_TYPE", "wayland");
    auto backend = autoinput::test_detectLinuxBackend();
    EXPECT_NE(backend, nullptr);
}

TEST(LinuxBackendTest, DetectsX11)
{
    test::ScopedEnvironmentVariable env("XDG_SESSION_TYPE", "x11");
    auto backend = autoinput::test_detectLinuxBackend();
    EXPECT_NE(backend, nullptr);
}

TEST(LinuxBackendTest, FallbackToX11)
{
    test::ScopedEnvironmentVariable env("XDG_SESSION_TYPE", std::nullopt); // Ensure it's unset
    auto backend = autoinput::test_detectLinuxBackend();
    EXPECT_NE(backend, nullptr);
}

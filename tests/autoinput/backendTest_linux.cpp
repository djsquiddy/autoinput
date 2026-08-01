/**
 * @file backendTest_linux.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include "autoinput/backend.h"
#include "autoinput/backendFactory.h"
#include "autoinput/logger.h"
#include "testUtils.h"

namespace autoinput
{
}

TEST(LinuxBackendTest, DetectsWayland)
{
    autoinput::test::ScopedEnvironmentVariable env("XDG_SESSION_TYPE", "wayland");
    auto backend = autoinput::BackendFactory::detectLinuxBackend();
    EXPECT_NE(backend, nullptr);
}

TEST(LinuxBackendTest, DetectsX11)
{
    autoinput::test::ScopedEnvironmentVariable env("XDG_SESSION_TYPE", "x11");
    auto backend = autoinput::BackendFactory::detectLinuxBackend();
    EXPECT_NE(backend, nullptr);
}

TEST(LinuxBackendTest, FallbackToX11)
{
    autoinput::test::ScopedEnvironmentVariable env("XDG_SESSION_TYPE", std::nullopt); // Ensure it's unset
    auto backend = autoinput::BackendFactory::detectLinuxBackend();
    EXPECT_NE(backend, nullptr);
}

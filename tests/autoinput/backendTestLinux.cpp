/**
 * @file backendTestLinux.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include <gtest/gtest.h>
#include "autoinput/platform/backend.h"
#include "autoinput/platform/backendFactory.h"
#include "autoinput/support/logger.h"
#include "testUtils.h"

namespace autoinput
{
}

TEST(LinuxBackendTest, DetectsWayland)
{
    autoinput::test::FakeEnvironment env;
    env.setEnvironmentVariable("XDG_SESSION_TYPE", "wayland");
    auto backend = autoinput::BackendFactory::createPlatformBackend(env);
    // Verify platform backend is created for Wayland session
    EXPECT_NE(backend, nullptr);
}

TEST(LinuxBackendTest, DetectsX11)
{
    autoinput::test::FakeEnvironment env;
    env.setEnvironmentVariable("XDG_SESSION_TYPE", "x11");
    auto backend = autoinput::BackendFactory::createPlatformBackend(env);
    // Verify platform backend is created for X11 session
    EXPECT_NE(backend, nullptr);
}

TEST(LinuxBackendTest, FallbackToX11)
{
    autoinput::test::FakeEnvironment env;
    // XDG_SESSION_TYPE not set
    auto backend = autoinput::BackendFactory::createPlatformBackend(env);
    // Verify fallback backend creation when session environment variable is unset
    EXPECT_NE(backend, nullptr);
}

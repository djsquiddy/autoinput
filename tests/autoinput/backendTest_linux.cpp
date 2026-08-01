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
    autoinput::test::FakeEnvironment env;
    env.setEnvironmentVariable("XDG_SESSION_TYPE", "wayland");
    auto backend = autoinput::BackendFactory::createPlatformBackend(env);
    EXPECT_NE(backend, nullptr);
}

TEST(LinuxBackendTest, DetectsX11)
{
    autoinput::test::FakeEnvironment env;
    env.setEnvironmentVariable("XDG_SESSION_TYPE", "x11");
    auto backend = autoinput::BackendFactory::createPlatformBackend(env);
    EXPECT_NE(backend, nullptr);
}

TEST(LinuxBackendTest, FallbackToX11)
{
    autoinput::test::FakeEnvironment env;
    // XDG_SESSION_TYPE not set
    auto backend = autoinput::BackendFactory::createPlatformBackend(env);
    EXPECT_NE(backend, nullptr);
}

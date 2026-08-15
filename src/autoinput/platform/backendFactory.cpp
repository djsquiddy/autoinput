/**
 * @file backendFactory.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/platform/backendFactory.h"
#include "autoinput/support/logger.h"
#include "autoinput/platform/environment.h"
#include <cstdlib>
#include <string>

#ifdef _WIN32
#include "autoinput/platform/backend.h"
#else
#include "autoinput/platform/linux/internalDataLinux.h"
#endif

namespace autoinput
{
    std::unique_ptr<IPlatformBackend> BackendFactory::createPlatformBackend()
    {
        return createPlatformBackend(SystemEnvironment::instance());
    }

    std::unique_ptr<IPlatformBackend> BackendFactory::createPlatformBackend(const IEnvironment& environment)
    {
#if AUTOINPUT_FAKE_HOOK
        Logger::info("Fake hook enabled, actions will be logged but not performed.\n");
        return std::make_unique<FakeBackend>();
#else
#ifdef _WIN32
        return createWindowsBackend();
#else
        return detectLinuxBackend(environment);
#endif
#endif
    }

    std::unique_ptr<IPlatformBackend> BackendFactory::detectLinuxBackend(const IEnvironment& environment)
    {
        auto sessionType = environment.environmentVariable("XDG_SESSION_TYPE");
        bool isWayland = sessionType && *sessionType == "wayland";

        if (isWayland)
        {
#ifdef __linux__
            Logger::info("Detected Wayland session, using Wayland/uinput backend\n");
            return createWaylandBackend();
#else
            return std::make_unique<FakeBackend>();
#endif
        }
        else
        {
#ifdef __linux__
#if AUTOINPUT_WITH_X11
            Logger::info("Detected X11 session or unknown, using X11 backend\n");
            return createX11Backend();
#else
            Logger::error("X11 support was not compiled in and this is not a Wayland session.\n");
            return nullptr;
#endif
#else
            return std::make_unique<FakeBackend>();
#endif
        }
    }
}

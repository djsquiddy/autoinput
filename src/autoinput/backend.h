/**
 * @file backend.h
 * @author djsquiddy
 * @date July 2026
 */
#ifndef INCLUDE_AUTOINPUT_BACKEND_H
#define INCLUDE_AUTOINPUT_BACKEND_H
#pragma once

#include "types.h"
#include "logger.h"

namespace autoinput
{
    class PlatformBackend
    {
    public:
        virtual ~PlatformBackend() = default;
        virtual bool installHooks() = 0;
        virtual void runListener() = 0;
        virtual void cleanup() = 0;
        // TODO: add shutdown that signals to the platform to cleanup.
        
        virtual void keyPress(const Key& key) = 0;
        virtual void keyRelease(const Key& key) = 0;
        virtual void mousePress(MouseButton button) = 0;
        virtual void mouseRelease(MouseButton button) = 0;
    };

    extern std::unique_ptr<PlatformBackend> g_backend;

    class FakeBackend : public PlatformBackend
    {
    public:
        bool installHooks() override { return true; }
        void runListener() override { while (true) std::this_thread::sleep_for(std::chrono::hours(1)); }
        void cleanup() override {}
        
        void keyPress(const Key& key) override { Logger::info("[FAKE] Pressing key: {}\n", key.toString()); }
        void keyRelease(const Key& key) override { Logger::info("[FAKE] Releasing key: {}\n", key.toString()); }
        void mousePress(MouseButton button) override { Logger::info("[FAKE] Pressing mouse button: {}\n", button); }
        void mouseRelease(MouseButton button) override { Logger::info("[FAKE] Releasing mouse button: {}\n", button); }
    };

#ifdef _WIN32
    std::unique_ptr<PlatformBackend> createWindowsBackend();
#else
    std::unique_ptr<PlatformBackend> createLinuxBackend();
#endif
}

#endif // INCLUDE_AUTOINPUT_BACKEND_H

/**
 * @file backend.h
 * @author djsquiddy
 * @date July 2026
 */
#ifndef INCLUDE_AUTOINPUT_BACKEND_H
#define INCLUDE_AUTOINPUT_BACKEND_H
#pragma once

#include <memory>
#include <thread>
#include <chrono>

#include "autoinput/types.h"
#include "autoinput/logger.h"

namespace autoinput
{
    class IPlatformBackend
    {
    public:
        virtual ~IPlatformBackend() = default;
        virtual bool installHooks() = 0;
        virtual void runListener() = 0;
        virtual void cleanup() = 0;

        virtual void keyPress(const Key& key) = 0;
        virtual void keyRelease(const Key& key) = 0;
        virtual void mousePress(const Mouse& mouse) = 0;
        virtual void mouseRelease(const Mouse& mouse) = 0;
    };

    extern std::unique_ptr<IPlatformBackend> g_backend;

    class FakeBackend : public IPlatformBackend
    {
    public:
        bool installHooks() override { return true; }
        void runListener() override { while (true) std::this_thread::sleep_for(std::chrono::hours(1)); }
        void cleanup() override {}
        
        void keyPress(const Key& key) override { Logger::info("[FAKE] Pressing key: {}\n", key.toString()); }
        void keyRelease(const Key& key) override { Logger::info("[FAKE] Releasing key: {}\n", key.toString()); }
        void mousePress(const Mouse& mouse) override { Logger::info("[FAKE] Pressing mouse: {}\n", mouse.toString()); }
        void mouseRelease(const Mouse& mouse) override { Logger::info("[FAKE] Releasing mouse: {}\n", mouse.toString()); }
    };

#ifdef _WIN32
    std::unique_ptr<IPlatformBackend> createWindowsBackend();
#endif
}

#endif // INCLUDE_AUTOINPUT_BACKEND_H

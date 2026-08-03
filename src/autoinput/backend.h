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
    struct BackendCapabilities
    {
        bool keyboardHooks = false;
        bool mouseHooks = false;
        bool focusDetection = false;
        bool listApplications = false;
        bool syntheticKeyboardInput = false;
        bool syntheticMouseInput = false;
        bool absoluteMouseMovement = false;
        bool getCursorPosition = false;
    };

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

        virtual void keyDown(const Key& key) = 0;
        virtual void keyUp(const Key& key) = 0;
        virtual void mouseDown(const Mouse& mouse) = 0;
        virtual void mouseUp(const Mouse& mouse) = 0;
        virtual void moveMouseTo(int32_t x, int32_t y) = 0;
        virtual std::pair<int32_t, int32_t> getCursorPosition() = 0;

        virtual BackendCapabilities capabilities() const = 0;
    };

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

        void keyDown(const Key& key) override { Logger::info("[FAKE] Key down: {}\n", key.toString()); }
        void keyUp(const Key& key) override { Logger::info("[FAKE] Key up: {}\n", key.toString()); }
        void mouseDown(const Mouse& mouse) override { Logger::info("[FAKE] Mouse down: {}\n", mouse.toString()); }
        void mouseUp(const Mouse& mouse) override { Logger::info("[FAKE] Mouse up: {}\n", mouse.toString()); }
        void moveMouseTo(int32_t x, int32_t y) override { Logger::info("[FAKE] Move mouse to: {}, {}\n", x, y); }
        std::pair<int32_t, int32_t> getCursorPosition() override { return { 0, 0 }; }

        BackendCapabilities capabilities() const override
        {
            return {
                .keyboardHooks = true,
                .mouseHooks = true,
                .focusDetection = true,
                .listApplications = true,
                .syntheticKeyboardInput = true,
                .syntheticMouseInput = true,
                .absoluteMouseMovement = true,
                .getCursorPosition = true
            };
        }
    };

#ifdef _WIN32
    std::unique_ptr<IPlatformBackend> createWindowsBackend();
#endif
}

#endif // INCLUDE_AUTOINPUT_BACKEND_H

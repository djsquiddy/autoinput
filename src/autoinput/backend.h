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
#include <atomic>

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
        /**
         * @brief Virtual destructor for IPlatformBackend.
         */
        virtual ~IPlatformBackend() = default;

        /**
         * @brief Installs platform-specific input hooks.
         * @return True if successful.
         */
        virtual bool installHooks() = 0;

        /**
         * @brief Runs the backend's event listener loop.
         */
        virtual void runListener() = 0;

        /**
         * @brief Cleans up backend resources.
         */
        virtual void cleanup() = 0;

        /**
         * @brief Requests the backend to stop its listener loop.
         */
        virtual void requestStop() = 0;

        /**
         * @brief Simulates a full key press (down and up).
         * @param key The key to press.
         */
        virtual void keyPress(const Key& key) = 0;

        /**
         * @brief Simulates a full key release (not usually needed if keyPress exists, but here for completeness).
         * @param key The key to release.
         */
        virtual void keyRelease(const Key& key) = 0;

        /**
         * @brief Simulates a full mouse button press (down and up).
         * @param mouse The mouse button to press.
         */
        virtual void mousePress(const Mouse& mouse) = 0;

        /**
         * @brief Simulates a full mouse button release.
         * @param mouse The mouse button to release.
         */
        virtual void mouseRelease(const Mouse& mouse) = 0;

        /**
         * @brief Simulates a key down event.
         * @param key The key to press down.
         */
        virtual void keyDown(const Key& key) = 0;

        /**
         * @brief Simulates a key up event.
         * @param key The key to release.
         */
        virtual void keyUp(const Key& key) = 0;

        /**
         * @brief Simulates a mouse button down event.
         * @param mouse The mouse button to press down.
         */
        virtual void mouseDown(const Mouse& mouse) = 0;

        /**
         * @brief Simulates a mouse button up event.
         * @param mouse The mouse button to release.
         */
        virtual void mouseUp(const Mouse& mouse) = 0;

        /**
         * @brief Moves the mouse cursor to absolute coordinates.
         * @param x The x-coordinate.
         * @param y The y-coordinate.
         */
        virtual void moveMouseTo(int32_t x, int32_t y) = 0;

        /**
         * @brief Gets the current cursor position.
         * @return A pair of (x, y) coordinates.
         */
        virtual std::pair<int32_t, int32_t> getCursorPosition() = 0;

        /**
         * @brief Enumerates all visible windows.
         * @return A vector of AppWindowInfo.
         */
        virtual std::vector<AppWindowInfo> enumerateWindows() = 0;

        /**
         * @brief Gets the information of the current foreground window.
         * @return An AppWindowInfo struct, or std::nullopt if not available.
         */
        virtual std::optional<AppWindowInfo> getForegroundWindow() = 0;

        /**
         * @brief Gets the capabilities of this backend.
         * @return A BackendCapabilities struct.
         */
        virtual BackendCapabilities capabilities() const = 0;

        /**
         * @brief Gets the display name of this backend.
         * @return The name of the backend.
         */
        virtual std::string getName() const = 0;
    };

    class FakeBackend : public IPlatformBackend
    {
    public:
        bool installHooks() override { return true; }
        void runListener() override { m_stop = false; while (!m_stop) std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
        void cleanup() override {}
        void requestStop() override { m_stop = true; }
        
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
        std::vector<AppWindowInfo> enumerateWindows() override { return {}; }
        std::optional<AppWindowInfo> getForegroundWindow() override { return std::nullopt; }

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

        std::string getName() const override { return "Fake Backend"; }
    private:
        std::atomic<bool> m_stop{ false };
    };

#ifdef _WIN32
    std::unique_ptr<IPlatformBackend> createWindowsBackend();
#endif
}

#endif // INCLUDE_AUTOINPUT_BACKEND_H

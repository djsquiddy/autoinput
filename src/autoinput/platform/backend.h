/**
 * @file backend.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_PLATFORM_BACKEND_H
#define INCLUDE_AUTOINPUT_PLATFORM_BACKEND_H
#pragma once

#include <memory>
#include <thread>
#include <chrono>
#include <atomic>

#include "autoinput/support/types.h"
#include "autoinput/support/logger.h"
#include "autoinput/platform/foregroundWindowListener.h"

namespace autoinput
{
    /**
     * @brief Describes the features supported by a platform backend.
     */
    struct BackendCapabilities
    {
        bool keyboardHooks = false;       /**< Support for listening to keyboard events. */
        bool mouseHooks = false;          /**< Support for listening to mouse events. */
        bool focusDetection = false;      /**< Support for detecting window focus changes. */
        bool listApplications = false;    /**< Support for enumerating running applications. */
        bool syntheticKeyboardInput = false; /**< Support for injecting keyboard events. */
        bool syntheticMouseInput = false;    /**< Support for injecting mouse events. */
        bool absoluteMouseMovement = false;  /**< Support for absolute cursor positioning. */
        bool getCursorPosition = false;      /**< Support for reading current cursor position. */
    };

    /**
     * @brief Interface for platform-specific input and windowing operations.
     */
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

        /**
         * @brief Sets the callback for foreground window change notifications.
         * @param callback The callback to invoke on focus change.
         */
        virtual void setForegroundWindowCallback(ForegroundWindowCallback callback)
        {
            m_foregroundCallback = std::move(callback);
        }

        /**
         * @brief Notifies the registered callback of a foreground window change.
         * @param info Information about the new foreground window.
         */
        void notifyForegroundWindowChanged(const AppWindowInfo& info)
        {
            if (m_foregroundCallback)
            {
                m_foregroundCallback(info);
            }
        }

    protected:
        ForegroundWindowCallback m_foregroundCallback;
    };

    /**
     * @brief A mock implementation of IPlatformBackend for testing and development.
     */
    class FakeBackend : public IPlatformBackend
    {
    public:
        /** @brief Mock implementation of installHooks. */
        bool installHooks() override
        {
            if (m_mockForegroundWindow.has_value())
            {
                notifyForegroundWindowChanged(*m_mockForegroundWindow);
            }
            return true;
        }
        /** @brief Mock implementation of runListener. */
        void runListener() override { m_stop = false; while (!m_stop) std::this_thread::sleep_for(std::chrono::milliseconds(100)); }
        /** @brief Mock implementation of cleanup. */
        void cleanup() override {}
        /** @brief Mock implementation of requestStop. */
        void requestStop() override { m_stop = true; }
        
        /** @brief Mock implementation of keyPress. */
        void keyPress(const Key& key) override { Logger::info("[FAKE] Pressing key: {}\n", key.toString()); }
        /** @brief Mock implementation of keyRelease. */
        void keyRelease(const Key& key) override { Logger::info("[FAKE] Releasing key: {}\n", key.toString()); }
        /** @brief Mock implementation of mousePress. */
        void mousePress(const Mouse& mouse) override { Logger::info("[FAKE] Pressing mouse: {}\n", mouse.toString()); }
        /** @brief Mock implementation of mouseRelease. */
        void mouseRelease(const Mouse& mouse) override { Logger::info("[FAKE] Releasing mouse: {}\n", mouse.toString()); }

        /** @brief Mock implementation of keyDown. */
        void keyDown(const Key& key) override { Logger::info("[FAKE] Key down: {}\n", key.toString()); }
        /** @brief Mock implementation of keyUp. */
        void keyUp(const Key& key) override { Logger::info("[FAKE] Key up: {}\n", key.toString()); }
        /** @brief Mock implementation of mouseDown. */
        void mouseDown(const Mouse& mouse) override { Logger::info("[FAKE] Mouse down: {}\n", mouse.toString()); }
        /** @brief Mock implementation of mouseUp. */
        void mouseUp(const Mouse& mouse) override { Logger::info("[FAKE] Mouse up: {}\n", mouse.toString()); }
        /** @brief Mock implementation of moveMouseTo. */
        void moveMouseTo(int32_t x, int32_t y) override { Logger::info("[FAKE] Move mouse to: {}, {}\n", x, y); }
        /** @brief Mock implementation of getCursorPosition. */
        std::pair<int32_t, int32_t> getCursorPosition() override { return { 0, 0 }; }
        /** @brief Mock implementation of enumerateWindows. */
        std::vector<AppWindowInfo> enumerateWindows() override { return {}; }
        /** @brief Mock implementation of getForegroundWindow. */
        std::optional<AppWindowInfo> getForegroundWindow() override { return m_mockForegroundWindow; }

        /** @brief Sets a mock foreground window for testing and notifies listeners. */
        void setMockForegroundWindow(AppWindowInfo info)
        {
            m_mockForegroundWindow = std::move(info);
            notifyForegroundWindowChanged(*m_mockForegroundWindow);
        }

        /** @brief Clears the mock foreground window. */
        void clearMockForegroundWindow()
        {
            m_mockForegroundWindow.reset();
        }

        /** @brief Mock implementation of capabilities. */
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

        /** @brief Mock implementation of getName. */
        std::string getName() const override { return "Fake Backend"; }
    private:
        std::atomic<bool> m_stop{ false };
        std::optional<AppWindowInfo> m_mockForegroundWindow;
    };

#ifdef _WIN32
    std::unique_ptr<IPlatformBackend> createWindowsBackend();
#endif
}

#endif // INCLUDE_AUTOINPUT_PLATFORM_BACKEND_H

/**
 * @file foregroundWindowListener.h
 * @brief Platform-neutral foreground window listener interface.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_PLATFORM_FOREGROUNDWINDOWLISTENER_H
#define INCLUDE_AUTOINPUT_PLATFORM_FOREGROUNDWINDOWLISTENER_H
#pragma once

#include <functional>
#include <optional>
#include "autoinput/support/types.h"

namespace autoinput
{
    using ForegroundWindowCallback = std::function<void(const AppWindowInfo&)>;

    /**
     * @class IForegroundWindowListener
     * @brief Interface for event-driven foreground window change listeners.
     */
    class IForegroundWindowListener
    {
    public:
        virtual ~IForegroundWindowListener() = default;

        /**
         * @brief Starts listening for foreground window change events.
         * @param callback Callback invoked when the foreground window changes.
         * @return True if listener was successfully initialized, false otherwise.
         */
        virtual bool start(ForegroundWindowCallback callback) = 0;

        /**
         * @brief Stops listening for foreground window change events.
         */
        virtual void stop() = 0;

        /**
         * @brief Queries the currently active foreground window information.
         * @return Optional AppWindowInfo if available.
         */
        [[nodiscard]] virtual std::optional<AppWindowInfo> getForegroundWindow() = 0;

        /**
         * @brief Checks if event-driven foreground detection is supported on this backend.
         * @return True if supported, false otherwise.
         */
        [[nodiscard]] virtual bool isSupported() const = 0;
    };
}

#endif // INCLUDE_AUTOINPUT_PLATFORM_FOREGROUNDWINDOWLISTENER_H

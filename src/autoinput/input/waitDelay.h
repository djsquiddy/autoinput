/**
 * @file waitDelay.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_INPUT_WAIT_DELAY_H
#define INCLUDE_AUTOINPUT_INPUT_WAIT_DELAY_H
#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include "autoinput/config/defaults.h"

namespace autoinput
{
    struct WaitDelayData
    {
        std::chrono::milliseconds minWaitPressDelay{ defaults::DefaultDelay };
        std::chrono::milliseconds maxWaitPressDelay{ defaults::DefaultDelay };
        std::chrono::milliseconds minWaitReleaseDelay{ defaults::DefaultDelay };
        std::chrono::milliseconds maxWaitReleaseDelay{ defaults::DefaultDelay };
        bool usePressRange{ false };
        bool hasPress{ false };
        bool useReleaseRange{ false };
        bool hasRelease{ false };

        /**
         * @brief Gets the current press delay, potentially randomized within a range.
         * @return The press delay duration.
         */
        [[nodiscard]] std::chrono::milliseconds getPressDelay() const;

        /**
         * @brief Gets the current release delay, potentially randomized within a range.
         * @return The release delay duration.
         */
        [[nodiscard]] std::chrono::milliseconds getReleaseDelay() const;

        /**
         * @brief Converts the wait delay data to a string representation.
         * @param isPressWait Whether to stringify the press or release delay.
         * @return The string representation.
         */
        [[nodiscard]] std::string toString(bool isPressWait) const;

        /**
         * @brief Parses a wait delay argument string into the struct.
         * @param waitTimeDelayArg The argument string (e.g. "100ms" or "100-200ms").
         * @param isPressWait Whether this is for the press or release delay.
         * @return True if parsing was successful.
         */
        bool parseWaitTimeDelay(std::string_view waitTimeDelayArg, bool isPressWait);
    };

    /**
     * @brief Validates if a string is a valid wait delay representation.
     * @param wait The wait string to validate.
     * @return True if valid, false otherwise.
     */
    bool isValidWaitDelay(std::string_view wait);

    /**
     * @brief Parses a wait delay string into milliseconds.
     * @param delayStr The delay string to parse.
     * @return The duration in milliseconds.
     */
    std::chrono::milliseconds parseWaitDelay(std::string_view delayStr);

    struct WaitDelayInput
    {
        double minValue{ 0.0 };
        double maxValue{ 0.0 };
        bool useRange{ false };
        bool hasValue{ false };
        std::string durationType{ "ms" };
    };

    std::optional<WaitDelayInput> parseWaitDelayInput(std::string_view value);
    std::string formatWaitDelayInput(const WaitDelayInput& input);
    std::chrono::milliseconds waitDelayInputToMilliseconds(double value, std::string_view durationType);
}

#endif // INCLUDE_AUTOINPUT_INPUT_WAIT_DELAY_H

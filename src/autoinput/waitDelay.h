/**
 * @file waitDelay.h
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_WAIT_DELAY_H
#define INCLUDE_AUTOINPUT_WAIT_DELAY_H
#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include "autoinput/defaults.h"

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

        [[nodiscard]] std::chrono::milliseconds getPressDelay() const;
        [[nodiscard]] std::chrono::milliseconds getReleaseDelay() const;
        [[nodiscard]] std::string toString(bool isPressWait) const;
        bool parseWaitTimeDelay(std::string_view waitTimeDelayArg, bool isPressWait);
    };

    /**
     * @brief Validates if a string is a valid wait delay representation.
     * @param wait The wait string to validate.
     * @return True if valid, false otherwise.
     */
    bool isValidWaitDelay(std::string_view wait);
}

#endif // INCLUDE_AUTOINPUT_WAIT_DELAY_H

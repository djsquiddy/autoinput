//
// Created by djsquiddy on 3/9/2026.
//

#ifndef INCLUDE_AUTOINPUT_PROGRAM_ARGUMENTS_H
#define INCLUDE_AUTOINPUT_PROGRAM_ARGUMENTS_H
#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include "types.h"

namespace autoinput
{
    struct WaitDelayData
    {
        std::chrono::milliseconds minWaitPressDelay{ DEFAULT_DELAY };
        std::chrono::milliseconds maxWaitPressDelay{ DEFAULT_DELAY };
        std::chrono::milliseconds minWaitReleaseDelay{ DEFAULT_DELAY };
        std::chrono::milliseconds maxWaitReleaseDelay{ DEFAULT_DELAY };
        bool usePressRange{ false };
        bool hasPress{ false };
        bool useReleaseRange{ false };
        bool hasRelease{ false };

        std::chrono::milliseconds getPressDelay() const;
        std::chrono::milliseconds getReleaseDelay() const;
        bool parseWaitTimeDelay(std::string_view waitTimeDelayArg, bool isPressWait);
    };

    struct ProgramArguments
    {
        std::string programName{};
        std::vector<MouseButton> buttons{};
        std::vector<std::string> startKeys{};
        std::string endKey{};
        ButtonState buttonState{ ButtonState::INVALID };
        WaitDelayData delayData{};

        [[nodiscard]] bool parseArguments(int argc, char** argv);

        [[nodiscard]] bool postParseArguments();
        void printUsage(bool verbose = false) const;

        /// A safe wrapper around testing the next string argument.
        ///
        /// @param i argument index
        /// @param argc arguments count
        /// @param argv arguments string
        /// @return If the string is not an option (starts with '-') and is within the bounds of the argument.
        static std::string_view safeGetNextArgument(int32_t i, int32_t argc, char** argv);
    };
}
#endif // INCLUDE_AUTOINPUT_PROGRAM_ARGUMENTS_H
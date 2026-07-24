//
// Created by djsquiddy on 3/9/2026.
//

#ifndef INCLUDE_AUTOINPUT_PROGRAM_ARGUMENTS_H
#define INCLUDE_AUTOINPUT_PROGRAM_ARGUMENTS_H
#pragma once

#include "utils.h"
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

        [[nodiscard]] std::chrono::milliseconds getPressDelay() const;
        [[nodiscard]] std::chrono::milliseconds getReleaseDelay() const;
        bool parseWaitTimeDelay(std::string_view waitTimeDelayArg, bool isPressWait);
    };

    class ProgramArguments : public NonCopyable
    {
    public:
        ProgramArguments();
        ~ProgramArguments() override = default;
        std::string programName{};
        std::vector<MouseButton> buttons{};
        std::vector<Key> keys{};
        std::vector<std::string> startKeys{};
        std::string endKey{};
        ActionState actionState{ ActionState::INVALID };
        WaitDelayData delayData{};

        [[nodiscard]] bool parseArguments(int argc, char** argv);

        [[nodiscard]] bool postParseArguments();
        void printUsage(bool verbose = false) const;

        bool parseConfigArguments(int argc, char** argv, int& i);
        bool parseActionState(int argc, char** argv, int& i);
        bool parseButton(int argc, char** argv, int& i);
#if AUTOINPUT_HOOK_KEYBOARD_ENABLED
        bool parseKey(int argc, char** argv, int& i);
#endif // AUTOINPUT_HOOK_KEYBOARD_ENABLED
        bool parseStartKey(int argc, char** argv, int& i);
        bool parseEndKey(int argc, char** argv, int& i);
        bool parsePressWaitTime(int argc, char** argv, int& i);
        bool parseReleaseTime(int argc, char** argv, int& i);
        bool parseWaitTIme(int argc, char** argv, int& i, bool isWaitPress);
        /// A safe wrapper around testing the next string argument.
        ///
        /// @param i argument index
        /// @param argc arguments count
        /// @param argv arguments string
        /// @return If the string is not an option (starts with '-') and is within the bounds of the argument.
        static std::string_view safeGetNextArgument(int32_t i, int32_t argc, char** argv);

        std::unordered_map<std::string, std::function<bool(int,char**,int&)>> argumentCallbacks;
    };
}
#endif // INCLUDE_AUTOINPUT_PROGRAM_ARGUMENTS_H
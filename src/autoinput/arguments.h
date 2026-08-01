/**
 * @file arguments.h
 * @author djsquiddy
 * @date March 2026
 */
#ifndef INCLUDE_AUTOINPUT_PROGRAM_ARGUMENTS_H
#define INCLUDE_AUTOINPUT_PROGRAM_ARGUMENTS_H
#pragma once

#include "autoinput/utils.h"
#include "autoinput/types.h"
#include "autoinput/settings.h"

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
        [[nodiscard]] std::string toString(bool isPressWait) const;
        bool parseWaitTimeDelay(std::string_view waitTimeDelayArg, bool isPressWait);
    };

    class ProgramArguments : public NonCopyable
    {
    public:
        ProgramArguments();
        ~ProgramArguments() override = default;
        std::string programName{};
        std::vector<Mouse> buttons{};
        std::vector<Key> keys{};
        std::vector<std::string> startKeys{};
        std::vector<ActionState> targetActions{};
        std::string endKey{};
        std::string applicationName{};
        std::string saveConfigName{};
        std::vector<std::string> blacklist{};
        bool listApplications{ false };
        bool listConfigs{ false };
        ActionState actionState{ ActionState::INVALID };
        WaitDelayData delayData{};

        [[nodiscard]] bool parseArguments(gsl::span<char*> args, bool loadSettings = false);

        [[nodiscard]] bool postParseArguments();
        void printUsage(bool verbose = false) const;
        [[nodiscard]] ConfigData toConfigData() const;

        bool parseConfigArguments(gsl::span<char*> args, int& i);
        bool parseActionState(gsl::span<char*> args, int& i);
        bool parseButton(gsl::span<char*> args, int& i);
#if AUTOINPUT_HOOK_KEYBOARD_ENABLED
        bool parseKey(gsl::span<char*> args, int& i);
#endif // AUTOINPUT_HOOK_KEYBOARD_ENABLED
        bool parseStartKey(gsl::span<char*> args, int& i);
        bool parseEndKey(gsl::span<char*> args, int& i);
        bool parsePressWaitTime(gsl::span<char*> args, int& i);
        bool parseReleaseTime(gsl::span<char*> args, int& i);
        bool parseWaitTIme(gsl::span<char*> args, int& i, bool isWaitPress);
        /// A safe wrapper around testing the next string argument.
        ///
        /// @param i argument index
        /// @param args arguments span
        /// @return If the string is not an option (starts with '-') and is within the bounds of the argument.
        static std::string_view safeGetNextArgument(int32_t i, gsl::span<char*> args);

    private:
        Settings m_settings;
    };
}
#endif // INCLUDE_AUTOINPUT_PROGRAM_ARGUMENTS_H
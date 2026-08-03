/**
 * @file arguments.h
 * @author djsquiddy
 * @date March 2026
 */
#ifndef INCLUDE_AUTOINPUT_PROGRAM_ARGUMENTS_H
#define INCLUDE_AUTOINPUT_PROGRAM_ARGUMENTS_H
#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <gsl/gsl>

#include "autoinput/waitDelay.h"
#include "autoinput/utils.h"
#include "autoinput/defaults.h"
#include "autoinput/types.h"
#include "autoinput/settings.h"
#include "autoinput/config.h"

namespace autoinput
{
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
        std::vector<std::string> commandNames{};
        std::vector<std::string> exclusiveGroups{};
        std::string endKey{};
        std::string applicationName{};
        std::string saveConfigName{};
        std::string validateConfigName{};
        std::string duplicateConfigSource{};
        std::string duplicateConfigDestination{};
        std::vector<std::string> blacklist{};
        std::vector<RecordedSequence> sequences{};
        bool listApplications{ false };
        bool listConfigs{ false };
        bool jsonOutput{ false };
        bool forceOverwrite{ false };
        ActionState actionState{ ActionState::INVALID };
        StatusNotificationMode statusNotificationMode{ StatusNotificationMode::Console };
        WaitDelayData delayData{};

        [[nodiscard]] bool parseArguments(gsl::span<char*> args, bool loadSettings = false);

        [[nodiscard]] bool postParseArguments();
        void printUsage(bool verbose = false) const;
        [[nodiscard]] ConfigData toConfigData() const;
        [[nodiscard]] const Settings& getSettings() const { return m_settings; }
        [[nodiscard]] Settings& getSettings() { return m_settings; }

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
        bool parseEarlyOptions(gsl::span<char*> args);
        bool parseCommandOptions(gsl::span<char*> args);
        bool parsePositionalArgument(std::string_view arg, gsl::span<char*> args, int& i);
        void applyDefaults();

        Settings m_settings;
    };
}
#endif // INCLUDE_AUTOINPUT_PROGRAM_ARGUMENTS_H
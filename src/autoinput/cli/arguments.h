/**
 * @file arguments.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_CLI_ARGUMENTS_H
#define INCLUDE_AUTOINPUT_CLI_ARGUMENTS_H
#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <gsl/gsl>

#include "autoinput/input/waitDelay.h"
#include "autoinput/support/utils.h"
#include "autoinput/config/defaults.h"
#include "autoinput/support/types.h"
#include "autoinput/config/settings.h"
#include "autoinput/config/config.h"

namespace autoinput
{
    class ProgramArguments
    {
    public:
        /**
         * @brief Default constructor for ProgramArguments.
         */
        ProgramArguments();

        /**
         * @brief Copy constructor.
         */
        ProgramArguments(const ProgramArguments&) = delete;

        /**
         * @brief Copy assignment operator.
         */
        ProgramArguments& operator=(const ProgramArguments&) = delete;

        /**
         * @brief Move constructor.
         */
        ProgramArguments(ProgramArguments&&) noexcept = default;

        /**
         * @brief Move assignment operator.
         */
        ProgramArguments& operator=(ProgramArguments&&) noexcept = default;

        /**
         * @brief Default destructor.
         */
        virtual ~ProgramArguments() = default;

        /** @brief The name of the program being executed. */
        std::string programName{};

        /** @brief List of mouse buttons to be automated. */
        std::vector<Mouse> buttons{};
        /** @brief List of keyboard keys to be automated. */
        std::vector<Key> keys{};
        /** @brief List of keys that trigger the automation start for corresponding commands. */
        std::vector<std::string> startKeys{};
        /** @brief The actions (CLICK, HOLD, etc.) to perform for each command. */
        std::vector<ActionState> targetActions{};
        /** @brief The human-readable names assigned to each command. */
        std::vector<std::string> commandNames{};
        /** @brief The exclusive group names for commands that shouldn't run simultaneously. */
        std::vector<std::string> exclusiveGroups{};
        /** @brief List of control bindings for each command. */
        std::vector<std::vector<CommandControlData>> commandControls{};

        /** @brief The key that stops all automation. */
        std::string endKey{};
        /** @brief The name of the target application (if restricted). */
        std::string applicationName{};
        /** @brief List of applications where automation should be disabled. */
        std::vector<std::string> blacklist{};
        /** @brief List of pre-recorded sequences to be available. */
        std::vector<RecordedSequence> sequences{};

        /** @brief The name for saving the configuration. */
        std::string saveConfigName{};

        /** @brief The name for a new recording. */
        std::string recordName{};
        /** @brief The key to start recording. */
        std::string recordStartKey{};
        /** @brief The key to stop recording. */
        std::string recordEndKey{};
        /** @brief The key to play back the recording. */
        std::string recordPlayStartKey{};
        /** @brief The interval for mouse sampling during recording. */
        std::string recordMouseSample{};
        /** @brief Whether to record mouse movements. */
        bool recordMouseMoves{ false };

        /** @brief Whether to output status information in JSON format. */
        bool jsonOutput{ false };
        /** @brief Whether to force overwrite existing files. */
        bool forceOverwrite{ false };

        /** @brief The default action state. */
        ActionState actionState{ ActionState::INVALID };
        /** @brief The mode for status notifications. */
        StatusNotificationMode statusNotificationMode{ StatusNotificationMode::Console };
        /** @brief Data for wait and delay timings. */
        WaitDelayData delayData{};

        /**
         * @brief Performs post-parsing validation and processing of arguments.
         * @return True if arguments are valid, false otherwise.
         */
        [[nodiscard]] bool postParseArguments();

        /**
         * @brief Converts the current arguments to ConfigData.
         * @return A ConfigData object containing the current argument values.
         */
        [[nodiscard]] ConfigData toConfigData() const;

        /**
         * @brief Gets the current settings (const).
         * @return Const reference to Settings.
         */
        [[nodiscard]] const Settings& getSettings() const { return m_settings; }

        /**
         * @brief Gets the current settings.
         * @return Reference to Settings.
         */
        [[nodiscard]] Settings& getSettings() { return m_settings; }

    private:
        void applyDefaults();

        Settings m_settings;
    };
}
#endif // INCLUDE_AUTOINPUT_CLI_ARGUMENTS_H

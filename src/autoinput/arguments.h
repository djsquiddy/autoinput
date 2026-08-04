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
        /**
         * @brief Default constructor for ProgramArguments.
         */
        ProgramArguments();

        /**
         * @brief Default destructor.
         */
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
        std::vector<std::string> blacklist{};
        std::vector<RecordedSequence> sequences{};

        std::string saveConfigName{};

        std::string recordName{};
        std::string recordStartKey{};
        std::string recordEndKey{};
        std::string recordPlayStartKey{};
        std::string recordMouseSample{};
        bool recordMouseMoves{ false };

        bool jsonOutput{ false };
        bool forceOverwrite{ false };

        ActionState actionState{ ActionState::INVALID };
        StatusNotificationMode statusNotificationMode{ StatusNotificationMode::Console };
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
#endif // INCLUDE_AUTOINPUT_PROGRAM_ARGUMENTS_H
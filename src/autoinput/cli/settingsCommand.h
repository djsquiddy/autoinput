/**
 * @file settingsCommand.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_CLI_SETTINGSCOMMAND_H
#define INCLUDE_AUTOINPUT_CLI_SETTINGSCOMMAND_H
#pragma once

#include "commandBase.h"
#include "autoinput/input/command.h"
#include "autoinput/cli/commandBase.h"
#include "autoinput/input/mouse.h"
#include "autoinput/support/types.h"

namespace autoinput::cli
{
    enum class SettingsAction : u8
    {
        None = 0,
        Print,
        Add,
        Update,
        Remove,
        Edit
    };

    [[nodiscard]] SettingsAction settingsActionFromString(const std::string_view& action);
    [[nodiscard]] std::string_view actionToString(SettingsAction action);
    [[nodiscard]] HelpEntry getActionHelpEntry(SettingsAction action);
    void printActionHelp(SettingsAction action, const CommandContext& context, const std::string_view& cmdPrefix);

    struct SettingsData : ICommandData
    {
        ~SettingsData() override = default;
        SettingsAction action{ SettingsAction::None };

        [[nodiscard]] bool validate() const override;
    };

    class SettingsCommand final : public MultiCommand
    {
    public:
        using MultiCommand::MultiCommand;

        SettingsData data{};

        /**
         * @brief Gets the command name.
         * @return "settings".
         */
        [[nodiscard]] std::string_view getName() const override { return "settings"; }

        /**
         * @brief Parses the settings command arguments.
         * @param args Arguments span.
         * @param index Current index.
         * @return True if successful.
         */
        [[nodiscard]] bool parse(gsl::span<char*> args, i32& index) override;

        /**
         * @brief Validates the settings command arguments.
         * @return True if valid.
         */
        [[nodiscard]] bool validate() const override;

        /**
         * @brief Executes the settings command (listing, validating, or duplicating configs).
         * @return Exit code.
         */
        [[nodiscard]] ErrorCode execute() override;

        /**
         * @brief Gets the help entry for the settings command.
         * @return The HelpEntry.
         */
        [[nodiscard]] HelpEntry getHelpEntry() const override;

        /**
         * @brief Prints help for the settings command.
         */
        void printHelp() const override;
    };
}
#endif // INCLUDE_AUTOINPUT_CLI_SETTINGSCOMMAND_H

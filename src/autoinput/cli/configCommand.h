/**
 * @file configCommand.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_CONFIG_COMMAND_H
#define INCLUDE_AUTOINPUT_CONFIG_COMMAND_H
#pragma once

#include "commandBase.h"
#include "autoinput/mouse.h"
#include "autoinput/types.h"

namespace autoinput::cli
{
    enum class ConfigAction : u8
    {
        None = 0,
        List,
        Validate,
        Duplicate,
        Copy
    };

    struct ConfigData
    {
        ConfigAction action{ ConfigAction::None };
        std::string source;
        std::string destination;
        bool force{ false };
    };

    class ConfigCommand final : public MultiCommand
    {
    public:
        using MultiCommand::MultiCommand;

        ConfigData data{};

        /**
         * @brief Gets the command name.
         * @return "config".
         */
        [[nodiscard]] std::string_view getName() const override { return "config"; }

        /**
         * @brief Gets the help entry for the config command.
         * @return The HelpEntry.
         */
        [[nodiscard]] HelpEntry getHelpEntry() const override;

        /**
         * @brief Parses the config command arguments.
         * @param args Arguments span.
         * @param index Current index.
         * @return True if successful.
         */
        [[nodiscard]] bool parse(gsl::span<char*> args, i32& index) override;

        /**
         * @brief Validates the config command arguments.
         * @return True if valid.
         */
        [[nodiscard]] bool validate() const override;

        /**
         * @brief Executes the config command (listing, validating, or duplicating configs).
         * @return Exit code.
         */
        [[nodiscard]] i32 execute() override;

        /**
         * @brief Prints help for the config command.
         */
        void printHelp() const override;
    };
}

#endif // INCLUDE_AUTOINPUT_CONFIG_COMMAND_H


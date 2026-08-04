/**
 * @file appsCommand.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_APPS_COMMAND_H
#define INCLUDE_AUTOINPUT_APPS_COMMAND_H
#pragma once

#include "autoinput/cli/commandBase.h"
#include "autoinput/types.h"

namespace autoinput::cli
{
    enum class AppsAction : u8
    {
        None = 0,
        List
    };

    struct AppsData
    {
        AppsAction action{ AppsAction::None };
    };

    class AppsCommand final : public CommandBase
    {
    public:
        using CommandBase::CommandBase;

        AppsData data{};

        /**
         * @brief Gets the command name.
         * @return "apps".
         */
        [[nodiscard]] std::string_view getName() const override { return "apps"; }

        /**
         * @brief Gets the help entry for the apps command.
         * @return The HelpEntry.
         */
        [[nodiscard]] HelpEntry getHelpEntry() const override;

        /**
         * @brief Parses the apps command arguments.
         * @param args Arguments span.
         * @param index Current index.
         * @return True if successful.
         */
        [[nodiscard]] bool parse(gsl::span<char*> args, i32& index) override;

        /**
         * @brief Validates the apps command arguments.
         * @return True if valid.
         */
        [[nodiscard]] bool validate() const override;

        /**
         * @brief Executes the apps command (listing applications).
         * @return Exit code.
         */
        [[nodiscard]] i32 execute() override;

        /**
         * @brief Prints help for the apps command.
         */
        void printHelp() const override;
    };

}

#endif // INCLUDE_AUTOINPUT_APPS_COMMAND_H

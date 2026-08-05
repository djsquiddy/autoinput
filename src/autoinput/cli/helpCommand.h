/**
* @file helpCommand.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_CLI_HELP_COMMAND_H
#define INCLUDE_AUTOINPUT_CLI_HELP_COMMAND_H
#pragma once

#include "autoinput/cli/commandBase.h"
#include <gsl/gsl>
#include <string>
#include <vector>
#include <memory>

namespace autoinput::cli
{
    /**
     * @brief Help command
     * HelpCommand prints Usage and Global options.
     */
    class HelpCommand final : public CommandBase
    {
    public:
        using CommandBase::CommandBase;

        /**
         * @brief Gets the command name.
         * @return "help".
         */
        [[nodiscard]] std::string_view getName() const override { return "help"; }

        /**
         * @brief Gets the help entry for the help command.
         * @return The HelpEntry.
         */
        [[nodiscard]] HelpEntry getHelpEntry() const override;

        /**
         * @brief Parses the help command arguments (topics).
         * @param args Arguments span.
         * @param index Current index.
         * @return True if successful.
         */
        [[nodiscard]] bool parse(gsl::span<char*> args, i32& index) override;

        /**
         * @brief Validates the help command arguments.
         * @return True if valid.
         */
        [[nodiscard]] bool validate() const override;

        /**
         * @brief Executes the help command (printing help for topics or main help).
         * @return Exit code.
         */
        [[nodiscard]] ErrorCode execute() override;

        /**
         * @brief Prints help for the help command itself.
         */
        void printHelp() const override;

        /**
         * @brief Sets the help topics for which this command should provide help.
         * @param topics The list of topics (subcommand names).
         */
        void setHelpTopic(const std::vector<std::string>& topics) { m_topics = topics; }

    private:
        std::vector<std::string> m_topics;

        [[nodiscard]] std::unique_ptr<CommandBase> createTopicCommand() const;

        void printUsage(const HelpEntry& entry) const;
        static void printGlobalOptions();
        void printMainHelp() const;
    };
}

#endif // INCLUDE_AUTOINPUT_CLI_HELP_COMMAND_H

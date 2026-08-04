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

        [[nodiscard]] std::string_view getName() const override { return "help"; }
        [[nodiscard]] HelpEntry getHelpEntry() const override;
        [[nodiscard]] bool parse(gsl::span<char*> args, i32& index) override;
        [[nodiscard]] bool validate() const override;
        [[nodiscard]] i32 execute() override;
        void printHelp() const override;

    private:
        std::vector<std::string> m_topics;

        [[nodiscard]] std::unique_ptr<CommandBase> createTopicCommand() const;

        void printUsage(const HelpEntry& entry) const;
        static void printGlobalOptions();
        void printMainHelp() const;
    };
}

#endif // INCLUDE_AUTOINPUT_CLI_HELP_COMMAND_H

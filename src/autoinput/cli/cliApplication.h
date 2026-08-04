/**
 * @file cliApplication.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_CLI_APPLICATION_H
#define INCLUDE_AUTOINPUT_CLI_APPLICATION_H
#pragma once

#include "commandBase.h"
#include "autoinput/mouse.h"
#include "autoinput/types.h"
#include "autoinput/keyboard.h"
#include "autoinput/waitDelay.h"

namespace autoinput
{
    class ProgramArguments;
}

namespace autoinput::cli
{
    class CliApplication
    {
    public:
        /**
         * @brief Parses the command-line arguments and determines the command to run.
         * @param args The command-line arguments.
         * @return True if parsing was successful.
         */
        [[nodiscard]] bool parse(gsl::span<char*> args);

        /**
         * @brief Executes the parsed command.
         * @return The exit code.
         */
        [[nodiscard]] i32 execute();

    private:
        CommandContext m_context;
        std::unique_ptr<CommandBase> m_command;

        [[nodiscard]] bool parseGlobalOptions(gsl::span<char*> args, i32& index);
        void printUsage();
    };

    /**
     * @brief High-level helper to run the program with configured arguments.
     * @param configureArguments Callback to configure the program arguments.
     * @return The exit code.
     */
    [[nodiscard]] i32 runProgramWithArguments(const std::function<bool(ProgramArguments&)>& configureArguments);

    /**
     * @brief Factory function to create a command instance by name.
     * @param commandName The name of the command to create.
     * @param context The command context to share.
     * @return A unique pointer to the created command, or nullptr if not found.
     */
    [[nodiscard]] std::unique_ptr<CommandBase> createCommand(std::string_view commandName, CommandContext& context);
}

#endif // INCLUDE_AUTOINPUT_CLI_APPLICATION_H


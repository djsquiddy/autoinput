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
        [[nodiscard]] bool parse(gsl::span<char*> args);
        [[nodiscard]] i32 execute();

    private:
        CommandContext m_context;
        std::unique_ptr<CommandBase> m_command;

        [[nodiscard]] bool parseGlobalOptions(gsl::span<char*> args, i32& index);
        void printUsage();
    };

    [[nodiscard]] i32 runProgramWithArguments(const std::function<bool(ProgramArguments&)>& configureArguments);
    [[nodiscard]] std::unique_ptr<CommandBase> createCommand(std::string_view commandName, CommandContext& context);
}

#endif // INCLUDE_AUTOINPUT_CLI_APPLICATION_H


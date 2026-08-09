/**
 * @file serveCommand.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_SERVE_COMMAND_H
#define INCLUDE_AUTOINPUT_SERVE_COMMAND_H
#pragma once

#include "autoinput/cli/commandBase.h"

namespace autoinput::cli
{
    class ServeCommand final : public CommandBase
    {
    public:
        using CommandBase::CommandBase;

        [[nodiscard]] std::string_view getName() const override { return "serve"; }
        [[nodiscard]] HelpEntry getHelpEntry() const override;
        [[nodiscard]] bool parse(gsl::span<char*> args, i32& index) override;
        [[nodiscard]] bool validate() const override;
        [[nodiscard]] ErrorCode execute() override;
        void printHelp() const override;

    private:
        bool m_stdio{ false };
        ErrorCode runStdioRuntimeServer();
    };
}

#endif // INCLUDE_AUTOINPUT_SERVE_COMMAND_H

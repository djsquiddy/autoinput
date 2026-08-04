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

        [[nodiscard]] std::string_view getName() const override { return "config"; }
        [[nodiscard]] HelpEntry getHelpEntry() const override;
        [[nodiscard]] bool parse(gsl::span<char*> args, i32& index) override;
        [[nodiscard]] bool validate() const override;
        [[nodiscard]] i32 execute() override;
        void printHelp() const override;
    };
}

#endif // INCLUDE_AUTOINPUT_CONFIG_COMMAND_H


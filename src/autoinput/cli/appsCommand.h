/**
 * @file appsCommand.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_APPS_COMMAND_H
#define INCLUDE_AUTOINPUT_APPS_COMMAND_H
#pragma once

#include "commandBase.h"
#include "autoinput/mouse.h"
#include "autoinput/types.h"
#include "autoinput/keyboard.h"
#include "autoinput/waitDelay.h"

namespace autoinput::cli
{
    enum class AppsAction : u8
    {
        None = 0,
        List
    };

    class AppsCommand final : public CommandBase
    {
    public:
        using CommandBase::CommandBase;

        AppsAction action{ AppsAction::None };

        [[nodiscard]] std::string_view getName() const override { return "apps"; }
        [[nodiscard]] HelpEntry getHelpEntry() const override;
        [[nodiscard]] bool parse(gsl::span<char*> args, i32& index) override;
        [[nodiscard]] bool validate() const override;
        [[nodiscard]] i32 execute() override;
        void printHelp() const override;
    };

}

#endif // INCLUDE_AUTOINPUT_APPS_COMMAND_H

/**
 * @file runCommand.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_RUN_COMMAND_H
#define INCLUDE_AUTOINPUT_RUN_COMMAND_H
#pragma once

#include "commandBase.h"
#include "autoinput/mouse.h"
#include "autoinput/types.h"
#include "autoinput/keyboard.h"
#include "autoinput/waitDelay.h"

namespace autoinput::cli
{
    struct RunConfig
    {
        std::string configName;
        std::vector<Mouse> buttons;
        std::vector<Key> keys;
        std::vector<std::string> startKeys;
        std::vector<ActionState> targetActions;
        std::string endKey;
        std::string applicationName;
        std::vector<std::string> blacklist;
        std::string saveConfigName;
        WaitDelayData delayData;
        StatusNotificationMode statusNotificationMode{ StatusNotificationMode::Console };
    };

    class RunCommand final : public CommandBase
    {
    public:
        using CommandBase::CommandBase;

        [[nodiscard]] std::string_view getName() const override { return "run"; }
        [[nodiscard]] HelpEntry getHelpEntry() const override;
        [[nodiscard]] bool parse(gsl::span<char*> args, i32& index) override;
        [[nodiscard]] bool validate() const override;
        [[nodiscard]] i32 execute() override;
        void printHelp() const override;

    private:
        RunConfig m_config;
    };
}

#endif // INCLUDE_AUTOINPUT_RUN_COMMAND_H

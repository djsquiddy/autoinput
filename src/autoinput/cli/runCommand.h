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
#include <optional>
#include <vector>

namespace autoinput::cli
{
    struct RunTarget
    {
        ActionState action{ ActionState::INVALID };
        std::optional<Mouse> mouse;
        std::optional<Key> key;
        std::string startKey;
    };

    struct RunConfig
    {
        std::string configName;
        std::vector<RunTarget> targets;
        ActionState pendingAction{ ActionState::INVALID };
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

/**
 * @file recordCommand.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_RECORD_COMMAND_H
#define INCLUDE_AUTOINPUT_RECORD_COMMAND_H
#pragma once

#include "commandBase.h"
#include "autoinput/mouse.h"
#include "autoinput/types.h"

namespace autoinput::cli
{
    struct RecordConfig
    {
        std::string name;
        std::string startKey;
        std::string endKey;
        std::string playStartKey;
        std::string mouseSample;
        bool mouseMoves{ false };
        bool force{ false };
    };

    class RecordCommand final : public CommandBase
    {
    public:
        using CommandBase::CommandBase;

        [[nodiscard]] std::string_view getName() const override { return "record"; }
        [[nodiscard]] HelpEntry getHelpEntry() const override;
        [[nodiscard]] bool parse(gsl::span<char*> args, i32& index) override;
        [[nodiscard]] bool validate() const override;
        [[nodiscard]] i32 execute() override;
        void printHelp() const override;

    private:
        RecordConfig m_config;
    };
}

#endif // INCLUDE_AUTOINPUT_RECORD_COMMAND_H

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

        /**
         * @brief Gets the command name.
         * @return "record".
         */
        [[nodiscard]] std::string_view getName() const override { return "record"; }

        /**
         * @brief Gets the help entry for the record command.
         * @return The HelpEntry.
         */
        [[nodiscard]] HelpEntry getHelpEntry() const override;

        /**
         * @brief Parses the record command arguments.
         * @param args Arguments span.
         * @param index Current index.
         * @return True if successful.
         */
        [[nodiscard]] bool parse(gsl::span<char*> args, i32& index) override;

        /**
         * @brief Validates the record command arguments.
         * @return True if valid.
         */
        [[nodiscard]] bool validate() const override;

        /**
         * @brief Executes the record command (starting recording session).
         * @return Exit code.
         */
        [[nodiscard]] i32 execute() override;

        /**
         * @brief Prints help for the record command.
         */
        void printHelp() const override;

    private:
        RecordConfig m_config;
    };
}

#endif // INCLUDE_AUTOINPUT_RECORD_COMMAND_H

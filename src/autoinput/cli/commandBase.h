/**
 * @file commandBase.h
 * @author djsquiddy
 * @date August 2026
 */

#ifndef INCLUDE_AUTOINPUT_COMMAND_BASE_H
#define INCLUDE_AUTOINPUT_COMMAND_BASE_H
#pragma once

#include "autoinput/types.h"
#include "autoinput/logger.h"
#include "autoinput/settings.h"
#include <string_view>

namespace autoinput::cli
{
    struct GlobalCliOptions
    {
        std::string programName;
        bool jsonOutput{ false };
        bool showExamples{ false };
        LogLevel logLevel{ LogLevel::Info };
    };

    struct CommandContext
    {
        GlobalCliOptions global;
        Settings settings;
    };

    struct HelpEntry
    {
        std::string usage;
        std::string description;
    };
    void logHelpEntries(std::string_view heading, const std::vector<HelpEntry>& entries);
    void logHelpStrings(std::string_view heading, const std::vector<std::string>& entries);

    struct HelpMessage
    {
        const CommandContext& context;
        std::vector<HelpEntry> commands;
        std::vector<HelpEntry> options;
        std::vector<std::string> examples;
        std::vector<std::string> notes;
    };
    void logHelpMessage(const HelpMessage& message);

    class CommandBase
    {
    public:
        explicit CommandBase(CommandContext& context) : m_context(context) {}
        virtual ~CommandBase() = default;

        [[nodiscard]] virtual std::string_view getName() const = 0;
        [[nodiscard]] virtual bool parse(gsl::span<char*> args, i32& index) = 0;
        [[nodiscard]] virtual bool validate() const = 0;
        [[nodiscard]] virtual i32 execute() = 0;
        /**
         * Individual command printHelp() prints only:
         * subcommands
        *  local options
         * examples
         * notes
         */
        virtual void printHelp() const = 0;
        [[nodiscard]] virtual HelpEntry getHelpEntry() const = 0;

    protected:
        CommandContext& m_context;
    };

    class MultiCommand : public CommandBase
    {
    public:
        using CommandBase::CommandBase;
        ~MultiCommand() override = default;

        void setHelpTopic(const std::vector<std::string>& topics)  { m_helpTopics = topics; }

    protected:
        std::vector<std::string> m_helpTopics;
    };

    /// A safe wrapper around testing the next string argument.
    ///
    /// @param i argument index
    /// @param args arguments span
    /// @return If the string is not an option (starts with '-') and is within the bounds of the argument.
    std::string_view safeGetNextArgument(i32 i, gsl::span<char*> args);
}

#endif // INCLUDE_AUTOINPUT_COMMAND_BASE_H

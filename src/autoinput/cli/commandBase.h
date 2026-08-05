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

namespace autoinput
{
    enum class ErrorCode : i32;
}

namespace autoinput::cli
{
    struct GlobalCliOptions
    {
        std::string programName{};
        std::string programPath{};
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
    /**
     * @brief Logs a list of help entries with a heading.
     * @param heading The heading to display.
     * @param entries The list of help entries (usage and description).
     */
    void logHelpEntries(std::string_view heading, const std::vector<HelpEntry>& entries);

    /**
     * @brief Logs a list of help strings with a heading.
     * @param heading The heading to display.
     * @param entries The list of strings.
     */
    void logHelpStrings(std::string_view heading, const std::vector<std::string>& entries);

    struct HelpMessage
    {
        const CommandContext& context;
        std::vector<HelpEntry> commands;
        std::vector<HelpEntry> options;
        std::vector<std::string> examples;
        std::vector<std::string> notes;
    };
    /**
     * @brief Logs a full help message.
     * @param message The help message data.
     */
    void logHelpMessage(const HelpMessage& message);

    class CommandBase
    {
    public:
        /**
         * @brief Constructs a CommandBase with a shared context.
         * @param context The command context.
         */
        explicit CommandBase(CommandContext& context) : m_context(context) {}

        /**
         * @brief Virtual destructor.
         */
        virtual ~CommandBase() = default;

        /**
         * @brief Gets the unique name of the command.
         * @return The command name.
         */
        [[nodiscard]] virtual std::string_view getName() const = 0;

        /**
         * @brief Parses the command-line arguments starting at the given index.
         * @param args The full span of command-line arguments.
         * @param index The current index in the arguments span, updated by the command.
         * @return True if parsing was successful.
         */
        [[nodiscard]] virtual bool parse(gsl::span<char*> args, i32& index) = 0;

        /**
         * @brief Validates the parsed command-line arguments.
         * @return True if arguments are valid.
         */
        [[nodiscard]] virtual bool validate() const = 0;

        /**
         * @brief Executes the command's logic.
         * @return The exit code (0 for success).
         */
        [[nodiscard]] virtual ErrorCode execute() = 0;

        /**
         * @brief Prints help information for this specific command.
         *
         * Individual command printHelp() prints only:
         * subcommands
         * local options
         * examples
         * notes
         */
        virtual void printHelp() const = 0;

        /**
         * @brief Gets the help entry for this command (usage and short description).
         * @return The HelpEntry.
         */
        [[nodiscard]] virtual HelpEntry getHelpEntry() const = 0;

    protected:
        CommandContext& m_context;
    };

    class MultiCommand : public CommandBase
    {
    public:
        using CommandBase::CommandBase;

        /**
         * @brief Virtual destructor for MultiCommand.
         */
        ~MultiCommand() override = default;

        /**
         * @brief Sets the help topics for which this command should provide help.
         * @param topics The list of topics (subcommand names).
         */
        void setHelpTopic(const std::vector<std::string>& topics) { m_helpTopics = topics; }

    protected:
        std::vector<std::string> m_helpTopics;
    };

    /// A safe wrapper around testing the next string argument.
    ///
    /// @param i argument index
    /// @param args arguments span
    /// @return If the string is not an option (starts with '-') and is within the bounds of the argument.
    std::string_view safeGetNextArgument(i32 i, gsl::span<char*> args);
    /// A safe wrapper around testing the next string argument.
    ///
    /// @param i argument index
    /// @param args arguments span
    /// @return If the string is not an option (starts with '-') and is within the bounds of the argument.
    std::string_view safeGetNextArgument(size_t i, gsl::span<char*> args);
}

#endif // INCLUDE_AUTOINPUT_COMMAND_BASE_H

/**
 * @file commandBase.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/cli/commandBase.h"
#include "autoinput/utils.h"

namespace autoinput::cli
{
    namespace
    {
        [[nodiscard]] size_t maxOptionUsageWidth(const std::vector<HelpEntry>& options)
        {
            size_t width = 0;
            for (const HelpEntry& option : options)
            {
                width = std::max(width, option.usage.size());
            }
            return width;
        }

        void logHelpExamples(const CommandContext& ctx, const std::vector<std::string>& examples)
        {
            if (examples.empty())
            {
                return;
            }

            Logger::print("\nExamples:\n");
            for (const std::string& example : examples)
            {
                Logger::print("  {} {}\n", ctx.global.programName, example);
            }
        }
    }

    std::string_view safeGetNextArgument(const i32 i, const gsl::span<char*> args)
    {
        if (i >= args.size() || args[i][0] == '-')
        {
            return std::string_view{};
        }

        Logger::debug("processing argument[{}]: {}\n", i, args[i]);
        return args[i] != nullptr ? std::string_view{args[i]} : std::string_view{};
    }

    std::string_view safeGetNextArgument(const size_t i, const gsl::span<char*> args)
    {
        if (i >= args.size() || args[i][0] == '-')
        {
            return std::string_view{};
        }

        Logger::debug("processing argument[{}]: {}\n", i, args[i]);
        return args[i] != nullptr ? std::string_view{args[i]} : std::string_view{};
    }

    void logHelpEntries(std::string_view heading, const std::vector<HelpEntry>& entries)
    {
        if (entries.empty())
        {
            return;
        }

        const size_t usageWidth = maxOptionUsageWidth(entries);

        Logger::print("\n{}:\n", heading);
        for (const auto& [usage, description] : entries)
        {
            Logger::print(
                "  {:<{}}  {}\n",
                usage,
                usageWidth,
                description
            );
        }
    }

    void logHelpStrings(std::string_view heading, const std::vector<std::string>& entries)
    {
        if (entries.empty())
        {
            return;
        }

        Logger::print("\n{}:\n", heading);
        for (const auto& entry : entries)
        {
            Logger::print("  {}\n", entry);
        }
    }

    void logHelpMessage(const HelpMessage& message)
    {
        logHelpEntries("Commands", message.commands);
        logHelpEntries("Options", message.options);
        logHelpExamples(message.context, message.examples);
        logHelpStrings("Note", message.notes);
        Logger::print("\n");
    }
}

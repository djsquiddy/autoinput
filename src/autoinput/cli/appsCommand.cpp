/**
 * @file appsCommand.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/cli/appsCommand.h"
#include "autoinput/errorCode.h"
#include "autoinput/platform.h"

namespace autoinput::cli
{
    bool AppsCommand::parse(const gsl::span<char*> args, i32& index)
    {
        if (index >= args.size())
        {
            Logger::fatal("The apps command needs a subcommand.\n");
            return false;
        }

        if (const std::string_view subcommand = args[index++]; subcommand == "list")
        {
            data.action = AppsAction::List;
        }
        else
        {
            Logger::fatal("Unknown apps subcommand: {}\n", subcommand);
            return false;
        }

        if (index < args.size())
        {
            Logger::fatal("Unexpected apps argument: {}\n", args[index]);
            return false;
        }

        return true;
    }

    bool AppsCommand::validate() const
    {
        if (data.action == AppsAction::None)
        {
            Logger::fatal("The apps command needs a subcommand.\n");
            return false;
        }

        return true;
    }

    ErrorCode AppsCommand::execute()
    {
        switch (data.action)
        {
        case AppsAction::List:
        {
            Logger::debug("Listing running applications\n");
            const auto apps = platform::getRunningApplicationNames();
            if (apps.empty())
            {
                Logger::print("No running applications found or listing not supported on this platform.\n");
                return ErrorCode::Success;
            }

            Logger::print("Currently running applications:\n");
            for (const std::string& app : apps)
            {
                Logger::print("  - {}\n", app);
            }

            return ErrorCode::Success;
        }

        case AppsAction::None:
        default:
            Logger::fatal("The apps command needs a subcommand.\n");
            return ErrorCode::InvalidParam;
        }
    }

    void AppsCommand::printHelp() const
    {
        logHelpMessage({
            .context = m_context,
            .commands = {
                { .usage = "list", .description = "List currently running application names" },
            },
            .examples = {
                "apps list",
            },
            .notes = {
                "Application listing depends on platform support.",
            },
        });
    }

    HelpEntry AppsCommand::getHelpEntry() const
    {
        return {
            .usage = "apps <command>",
            .description = "Inspect running applications.",
        };
    }
}

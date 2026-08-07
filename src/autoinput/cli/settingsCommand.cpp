/**
 * @file settingsCommand.cpp
 * @author djsquiddy
 * @date August 2026
 */

#include "settingsCommand.h"

#include "autoinput/errorCode.h"

namespace autoinput::cli
{
    SettingsAction settingsActionFromString(const std::string_view& action)
    {
        if (action == "print")
        {
            return SettingsAction::Print;
        }
        if (action == "add")
        {
            return SettingsAction::Add;
        }
        if (action == "update")
        {
            return SettingsAction::Update;
        }
        if (action == "remove")
        {
            return SettingsAction::Remove;
        }
        if (action == "edit")
        {
            return SettingsAction::Edit;
        }

        return SettingsAction::None;
    }

    HelpEntry getActionHelpEntry(const SettingsAction action)
    {
        switch (action)
        {
        case SettingsAction::Print:
            return {
                .usage = "print",
                .description = "Prints the current settings",
            };
            break;
        case SettingsAction::Add:
            return {
                .usage = "add KEY VALUE",
                .description = "Adds a new setting",
            };
            break;
        case SettingsAction::Update:
            return {
                .usage = "update KEY VALUE",
                .description = "Update a setting key with the new value",
            };
            break;

        case SettingsAction::Remove:
            return {
                .usage = "remove KEY",
                .description = "Removes a setting",
            };
            break;
        case SettingsAction::Edit:
            return {
                .usage = "edit KEY VALUE",
                .description = "Edits a setting",
            };
            break;
        case SettingsAction::None:
        default:
            break;
        }

        return {};
    }

    std::string_view actionToString(const SettingsAction action)
    {
        switch (action)
        {
        case SettingsAction::Print:
            return "print";
        case SettingsAction::Add:
            return "add";
        case SettingsAction::Update:
            return "update";
        case SettingsAction::Remove:
            return "remove";
        case SettingsAction::Edit:
            return "edit";
        case SettingsAction::None:
        default:
            return "";
        }
    }

    void printActionHelp(SettingsAction action, const CommandContext& context, const std::string_view& cmdPrefix)
    {
        switch (action)
        {
        case SettingsAction::Print:
            logHelpMessage({
                .context = context,
                .examples = {
                    std::format("{} print", cmdPrefix)
                },
            });
            break;
        case SettingsAction::Add:
            logHelpMessage({
                .context = context,
                .examples = {
                    std::format("{} add editor vscode", cmdPrefix),
                },
            });
            break;
        case SettingsAction::Update:
            logHelpMessage({
                .context = context,
                .options = {
                    {.usage = "-a, --append", .description = "Append value to existing setting"}
                },
                .examples = {
                    std::format("{} add editor zed", cmdPrefix),
                    std::format("{} update blacklist explorer --append", cmdPrefix),
                    std::format("{} update editor zed", cmdPrefix),
                    std::format("{} update statusNotificationMode console", cmdPrefix),
                    std::format("{} update blacklist explorer -a", cmdPrefix)
                }
            });
            break;
        case SettingsAction::Remove:
            logHelpMessage({
                .context = context,
                .examples = {
                    std::format("{} remove editor", cmdPrefix),
                },
            });
            break;
        case SettingsAction::Edit:
            logHelpMessage({
                .context = context,
                .examples = {
                    std::format("{} edit editor vscode", cmdPrefix),
                },
            });
            break;
        case SettingsAction::None:
        default:
            break;
        }
    }

    bool SettingsData::validate() const
    {
        switch (action)
        {
        case SettingsAction::None:
        default:
            Logger::fatalError({
                .code = ErrorCode::MissingCommandLineArgument,
                .message = "The settings command needs a subcommand."}
            );
            return false;
        }
    }

    bool SettingsCommand::parse(gsl::span<char*> args, i32& index)
    {
        return true;
    }

    bool SettingsCommand::validate() const
    {
        return true;
    }

    ErrorCode SettingsCommand::execute()
    {
        return ErrorCode::Success;
    }

    HelpEntry SettingsCommand::getHelpEntry() const
    {
        if (m_helpTopics.size() >= 2)
        {
            const std::string& topic = m_helpTopics[1];
            if (const SettingsAction action = settingsActionFromString(topic); action != SettingsAction::None)
            {
                return getActionHelpEntry(action);
            }
        }

        return {
            .usage = "settings <command> [options]",
            .description = "Manages autoinput settings.",
        };
    }

    void SettingsCommand::printHelp() const
    {
    }
}

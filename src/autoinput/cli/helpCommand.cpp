/**
 * @file helpCommand.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "helpCommand.h"

#include "appsCommand.h"
#include "cliApplication.h"
#include "configCommand.h"
#include "recordCommand.h"
#include "runCommand.h"
#include "serveCommand.h"
#include "autoinput/support/errorCode.h"
#include <format>

namespace autoinput::cli
{
    bool HelpCommand::parse(const gsl::span<char*> args, int& index)
    {
        while (index < args.size())
        {
            const std::string_view arg = args[index];
            if (arg.starts_with('-'))
            {
                Logger::fatal("Unknown help option: {}\n", arg);
                return false;
            }

            m_topics.emplace_back(arg);
            ++index;
        }

        return true;
    }

    bool HelpCommand::validate() const
    {
        if (m_topics.empty())
        {
            return true;
        }

        if (m_topics.size() == 1)
        {
            return m_topics[0] == "run" ||
                   m_topics[0] == "record" ||
                   m_topics[0] == "config" ||
                   m_topics[0] == "apps" ||
                   m_topics[0] == "serve";
        }

        if (m_topics.size() == 2 && m_topics[0] == "config")
        {
            return m_topics[1] == "list" ||
                   m_topics[1] == "validate" ||
                   m_topics[1] == "duplicate" ||
                   m_topics[1] == "copy";
        }

        return false;
    }

    ErrorCode HelpCommand::execute()
    {
        if (!validate())
        {
            Logger::fatal("Unknown help topic.\n\n");
            printUsage(getHelpEntry());
            printGlobalOptions();
            printMainHelp();
            return ErrorCode::InvalidParam;
        }

        printHelp();
        return ErrorCode::Success;
    }

    void HelpCommand::printHelp() const
    {
        const std::unique_ptr<CommandBase> command = createTopicCommand();
        if (!command)
        {
            printMainHelp();
            return;
        }

        const HelpEntry entry = command ? command->getHelpEntry() : getHelpEntry();

        printUsage(entry);
        printGlobalOptions();

        if (command)
        {
            command->printHelp();
            return;
        }

        printMainHelp();
    }

    std::unique_ptr<CommandBase> HelpCommand::createTopicCommand() const
    {
        if (m_topics.empty())
        {
            return nullptr;
        }

        std::unique_ptr<CommandBase> command = createCommand(m_topics[0], m_context);
        if (!command)
        {
            return nullptr;
        }

        if (auto* multiCommand = dynamic_cast<MultiCommand*>(command.get()); multiCommand != nullptr)
        {
            multiCommand->setHelpTopic(m_topics);
        }

        return command;
    }

    void HelpCommand::printUsage(const HelpEntry& entry) const
    {
        Logger::print(
            "Usage:\n"
            "  {} {}\n",
            m_context.global.programName,
            entry.usage
        );
    }

    void HelpCommand::printGlobalOptions()
    {
        std::vector<HelpEntry> entries;
        entries.reserve(HelpMetadata::GLOBAL_OPTIONS.size());
        for (const auto& option : HelpMetadata::GLOBAL_OPTIONS)
        {
            entries.push_back(optionToHelpEntry(option));
        }

        logHelpEntries("Global options", entries);
    }

    void HelpCommand::printMainHelp() const
    {
        std::vector<HelpEntry> entries;
        entries.reserve(HelpMetadata::ALL_COMMANDS.size());
        for (const auto& command : HelpMetadata::ALL_COMMANDS)
        {
            entries.push_back({ .usage = std::string(command.usage), .description = std::string(command.description) });
        }

        logHelpEntries("Commands", entries);

        logHelpStrings("Examples", {
            std::format("{} help run", m_context.global.programName),
            std::format("{} help config", m_context.global.programName),
            std::format("{} help config validate", m_context.global.programName),
            std::format("{} help serve", m_context.global.programName),
            std::format("{} serve --stdio", m_context.global.programName),
        });

        Logger::print("\n");
    }

    HelpEntry HelpCommand::getHelpEntry() const
    {
        if (const HelpMetadata::CliCommandMetadata* metadata = HelpMetadata::findCommand(getName()))
        {
            return { .usage = std::string(metadata->usage), .description = std::string(metadata->description) };
        }
        return {};
    }
}

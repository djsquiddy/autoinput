/**
 * @file configCommand.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/cli/configCommand.h"

#include "autoinput/config/config.h"
#include "autoinput/config/configValidator.h"
#include "autoinput/support/errorCode.h"
#include <filesystem>

#include "autoinput/services/configService.h"

namespace autoinput::cli
{
    namespace
    {
        void listConfigsFromDirectory(const std::filesystem::path& path, const std::string_view label)
        {
            Logger::print("{} configurations", label);

            if (!path.empty())
            {
                Logger::print(" in {}", path.string());
            }

            Logger::print(":\n");

            if (!std::filesystem::exists(path) || !std::filesystem::is_directory(path))
            {
                Logger::print("  (none)\n\n");
                return;
            }

            bool found = false;
            for (const auto& entry : std::filesystem::directory_iterator(path))
            {
                if (!entry.is_regular_file() || entry.path().extension() != ".toml")
                {
                    continue;
                }

                const std::string name = entry.path().stem().string();
                if (name == "settings")
                {
                    continue;
                }

                Logger::print("  - {}\n", name);
                found = true;
            }

            if (!found)
            {
                Logger::print("  (none)\n");
            }

            Logger::print("\n");
        }

        [[nodiscard]] ErrorCode executeListConfigs()
        {
            Logger::debug("Listing configurations\n");
            listConfigsFromDirectory(getConfigsPath(), "Global");
            listConfigsFromDirectory(getUserConfigsPath(), "User");
            return ErrorCode::Success;
        }

        [[nodiscard]] ErrorCode executeValidateConfig(const services::ConfigService& configService, const std::string& source)
        {
            Logger::info("Validating configuration: {}\n", source);
            const auto result = configService.validateConfig(source);
            printValidationJson(result);
            if (result.isValid)
            {
                Logger::info("Configuration is valid: {}\n", source);
            }
            else
            {
                Logger::error("Configuration is invalid: {}\n", source);
            }
            return result.isValid ? ErrorCode::Success : ErrorCode::FailedToLoadConfig;
        }

        [[nodiscard]] ErrorCode executeDuplicateConfig(
            const std::string& source,
            const std::string& destination,
            const bool force)
        {
            Logger::info("Duplicating configuration: {} -> {}\n", source, destination);
            if (duplicateConfig(source, destination, force))
            {
                Logger::info("Successfully duplicated configuration to {}\n", destination);
                return ErrorCode::Success;
            }

            Logger::error("Failed to duplicate configuration: {} -> {}\n", source, destination);
            return ErrorCode::FailedToLoadConfig;
        }

        [[nodiscard]] ErrorCode executePrintConfigPath(const std::string& nameOrPath)
        {
            const auto configPath = getConfigFilePath(nameOrPath);
            Logger::debug("Resolving config path for {}: {}\n", nameOrPath, configPath.string());
            Logger::print("{}\n", configPath.string());
            return ErrorCode::Success;
        }
    }

    ConfigAction configActionFromString(const std::string_view& action)
    {
        if (action == "list")
        {
            return ConfigAction::List;
        }
        if (action == "validate")
        {
            return ConfigAction::Validate;
        }
        if (action == "duplicate")
        {
            return ConfigAction::Duplicate;
        }
        if (action == "copy")
        {
            return ConfigAction::Copy;
        }
        if (action == "path")
        {
            return ConfigAction::Path;
        }

        return ConfigAction::None;
    }

    std::string_view actionToString(const ConfigAction action)
    {
        switch (action)
        {
        case ConfigAction::List:
            return "list";
        case ConfigAction::Validate:
            return "validate";
        case ConfigAction::Duplicate:
            return "duplicate";
        case ConfigAction::Copy:
            return "copy";
        case ConfigAction::Path:
            return "path";
        case ConfigAction::None:
        default:
            return "";
        }
    }

    HelpEntry getActionHelpEntry(const ConfigAction action)
    {
        const HelpMetadata::CliCommandMetadata* configMetadata = HelpMetadata::findCommand("config");
        if (!configMetadata)
        {
            return {};
        }

        const HelpMetadata::CliCommandMetadata* subMetadata = HelpMetadata::findSubcommand(*configMetadata, actionToString(action));
        if (!subMetadata)
        {
            return {};
        }

        return {
            .usage = std::format("config {}", subMetadata->usage),
            .description = std::string(subMetadata->description),
        };
    }

    void printActionHelp(const ConfigAction action, const CommandContext& context)
    {
        const HelpMetadata::CliCommandMetadata* configMetadata = HelpMetadata::findCommand("config");
        if (!configMetadata)
        {
            return;
        }

        const std::vector<std::string> topics{ "config", std::string(actionToString(action)) };
        renderCommandHelp(*configMetadata, context, topics);
    }

    bool ConfigCliData::validate() const
    {
        switch (action)
        {
        case ConfigAction::List:
            return true;
        case ConfigAction::Validate:
            if (source.empty())
            {
                Logger::fatalError({
                    .code = ErrorCode::MissingCommandLineArgument,
                    .message = std::format("The config {} command needs a NAME_OR_PATH argument.", actionToString(action))
                });
                return false;
            }
            return true;
        case ConfigAction::Duplicate:
        case ConfigAction::Copy:
            if (source.empty())
            {
                Logger::fatalError({
                    .code = ErrorCode::MissingCommandLineArgument,
                    .message = std::format("The config {} command needs a SOURCE argument.", actionToString(action))
                });
                return false;
            }
            if (destination.empty())
            {
                Logger::fatalError({
                    .code = ErrorCode::MissingCommandLineArgument,
                    .message = std::format("The config {} command needs a DESTINATION argument.", actionToString(action))
                });
                return false;
            }
            return true;
        case ConfigAction::Path:
            if (source.empty())
            {
                Logger::fatalError({
                    .code = ErrorCode::MissingCommandLineArgument,
                    .message = std::format("The config {} command needs a NAME_OR_PATH argument.", actionToString(action))
                });
                return false;
            }
            return true;
        case ConfigAction::None:
        default:
            Logger::fatalError({
                .code = ErrorCode::MissingCommandLineArgument,
                .message = "The config command needs a subcommand."}
            );
            return false;
        }
    }

    bool ConfigCommand::parse(const gsl::span<char*> args, i32& index)
    {
        if (index >= args.size())
        {
            Logger::fatal("The config command needs a subcommand.\n");
            return false;
        }

        if (const std::string_view subcommand = args[index++]; subcommand == "list")
        {
            data.action = ConfigAction::List;
        }
        else if (subcommand == "validate")
        {
            data.action = ConfigAction::Validate;

            const std::string_view value = safeGetNextArgument(index, args);
            if (value.empty())
            {
                Logger::fatal("The config validate command needs a NAME_OR_PATH argument.\n");
                return false;
            }

            data.source = value;
            ++index;
        }
        else if (subcommand == "duplicate" || subcommand == "copy")
        {
            data.action = subcommand == "duplicate" ? ConfigAction::Duplicate : ConfigAction::Copy;

            const std::string_view sourceValue = safeGetNextArgument(index, args);
            if (sourceValue.empty())
            {
                Logger::fatalError({
                    .code = ErrorCode::MissingCommandLineArgument,
                    .message = std::format("The config {} command needs a SOURCE argument.", subcommand)
                });
                return false;
            }

            data.source = sourceValue;
            ++index;

            const std::string_view destinationValue = safeGetNextArgument(index, args);
            if (destinationValue.empty())
            {
                Logger::fatalError({
                    .code = ErrorCode::MissingCommandLineArgument,
                    .message = std::format("The config {} command needs a DESTINATION argument.", subcommand)
                });
                return false;
            }

            data.destination = destinationValue;
            ++index;
        }
        else if (subcommand == "path")
        {
            data.action = ConfigAction::Path;
            const std::string_view nameOrPath = safeGetNextArgument(index, args);
            if (nameOrPath.empty())
            {
                Logger::fatalError({
                    .code = ErrorCode::MissingCommandLineArgument,
                    .message = std::format("The config path command needs a NAME_OR_PATH argument.")
                });
                return false;
            }

            data.source = nameOrPath;
            ++index;
        }
        else
        {
            Logger::fatalError({
                .code = ErrorCode::UnknownCommand,
                .message = std::format("Unknown config subcommand: {}", subcommand)
            });
            return false;
        }

        while (index < args.size())
        {
            const std::string_view arg = args[index];

            if (arg == "--force")
            {
                if (data.action != ConfigAction::Duplicate && data.action != ConfigAction::Copy)
                {
                    Logger::fatal("The --force option only applies to config duplicate/copy.\n");
                    return false;
                }

                data.force = true;
                ++index;
                continue;
            }

            Logger::fatal("Unknown config option: {}\n", arg);
            return false;
        }

        return true;
    }

    bool ConfigCommand::validate() const
    {
        return data.validate();
    }

    ErrorCode ConfigCommand::execute()
    {
        switch (data.action)
        {
        case ConfigAction::List:
            return executeListConfigs();
        case ConfigAction::Validate:
            AUTOINPUT_ASSERT(m_context.configService != nullptr,
                "Config Service is not initialized when it was expected to be."
            );
            return executeValidateConfig(*m_context.configService, data.source);
        case ConfigAction::Duplicate:
        case ConfigAction::Copy:
            return executeDuplicateConfig(data.source, data.destination, data.force);
        case ConfigAction::Path:
            return executePrintConfigPath(data.source);
        case ConfigAction::None:
        default:
            Logger::fatal("The config command needs a subcommand.\n");
            return ErrorCode::InvalidParam;
        }
    }

    void ConfigCommand::printHelp() const
    {
        const HelpMetadata::CliCommandMetadata* metadata = HelpMetadata::findCommand(getName());
        if (!metadata)
        {
            return;
        }

        const std::vector<std::string> topics = getHelpTopicsSize() > 0
            ? getHelpTopics()
            : std::vector<std::string>{ std::string(getName()) };
        renderCommandHelp(*metadata, m_context, topics);
    }

    HelpEntry ConfigCommand::getHelpEntry() const
    {
        if (getHelpTopicsSize() >= 2)
        {
            const std::string& topic = getHelpTopicEntry(1);
            if (const ConfigAction action = configActionFromString(topic); action != ConfigAction::None)
            {
                return getActionHelpEntry(action);
            }
        }

        const HelpMetadata::CliCommandMetadata* metadata = HelpMetadata::findCommand(getName());
        if (!metadata)
        {
            return {};
        }

        return { .usage = std::string(metadata->usage), .description = std::string(metadata->description) };
    }
}

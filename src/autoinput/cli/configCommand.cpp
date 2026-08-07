/**
 * @file configCommand.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/cli/configCommand.h"

#include "autoinput/config.h"
#include "autoinput/configValidator.h"
#include "autoinput/errorCode.h"
#include <filesystem>

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

        [[nodiscard]] ErrorCode executeValidateConfig(const ConfigService& configService, const std::string& source)
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
        switch (action)
        {
        case ConfigAction::List:
            return {
            .usage = "config list",
            .description = "List available configurations.",
        };
        case ConfigAction::Validate:
            return {
            .usage = "config validate NAME_OR_PATH",
            .description = "Validate a configuration file.",
        };
        case ConfigAction::Duplicate:
            return {
            .usage = "config duplicate SOURCE DESTINATION [options]",
            .description = "Duplicate a configuration into the user config directory.",
        };
        case ConfigAction::Copy:
            return {
            .usage = "config copy SOURCE DESTINATION [options]",
            .description = "Alias for config duplicate.",
        };
        case ConfigAction::Path:
            return {
            .usage = "print NAME_OR_PATH",
            .description = "Print the path to the configuration.",
        };
        case ConfigAction::None:
        default:
            return {};
        }
    }

    void printActionHelp(const ConfigAction action, const CommandContext& context)
    {
        switch (action)
        {
        case ConfigAction::List:
            logHelpMessage({
                .context = context,
                .examples = {
                    "config list",
                },
            });
            break;
        case ConfigAction::Validate:
            logHelpMessage({
                .context = context,
                .examples = {
                    "config validate my-config",
                    "--json config validate my-config",
                },
                .notes = {
                    "Use the global --json option before the command for machine-readable output.",
                },
            });
            break;
        case ConfigAction::Duplicate:
        case ConfigAction::Copy:
            logHelpMessage({
                .context = context,
                .options = {
                    { .usage = "--force", .description = "Overwrite destination if it already exists" },
                },
                .examples = {
                    "config duplicate old-config my-copy",
                    "config duplicate old-config my-copy --force",
                    "config copy old-config new-config",
                },
            });
            break;
        case ConfigAction::Path:
            logHelpMessage({
                .context = context,
                .examples = {
                    "config path my-config"
                },
            });
            break;
        case ConfigAction::None:
        default:
            break;
        }
    }

    bool ConfigData::validate() const
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
        if (m_helpTopics.size() >= 2)
        {
            const std::string& topic = m_helpTopics[1];
            if (const ConfigAction action = configActionFromString(topic); action != ConfigAction::None)
            {
                printActionHelp(action, m_context);
            }
        }

        logHelpMessage({
            .context = m_context,
            .commands = {
                { .usage = "list", .description = "List available configurations" },
                { .usage = "validate NAME_OR_PATH", .description = "Validate a configuration file" },
                { .usage = "duplicate SOURCE DESTINATION", .description = "Duplicate a configuration into the user config directory" },
                { .usage = "copy SOURCE DESTINATION", .description = "Alias for duplicate" },
                { .usage = "path NAME_OR_PATH", .description = "Print the path to the configuration." },
            },
            .examples = {
                "config list",
                "config validate my-config",
                "config duplicate old-config new-config",
                "config copy old-config new-config",
                "config path my-config",
                "help config validate",
                "help config duplicate",
            },
        });
    }

    HelpEntry ConfigCommand::getHelpEntry() const
    {
        if (m_helpTopics.size() >= 2)
        {
            const std::string& topic = m_helpTopics[1];
            if (const ConfigAction action = configActionFromString(topic); action != ConfigAction::None)
            {
                return getActionHelpEntry(action);
            }
        }

        return {
            .usage = "config <command> [options]",
            .description = "Manage autoinput configuration files.",
        };
    }
}

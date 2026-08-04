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
        [[nodiscard]] std::string_view configActionToString(const ConfigAction action)
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
            case ConfigAction::None:
            default:
                return "";
            }
        }

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

        [[nodiscard]] i32 executeListConfigs()
        {
            listConfigsFromDirectory(getConfigsPath(), "Global");
            listConfigsFromDirectory(getUserConfigsPath(), "User");
            return static_cast<i32>(ErrorCode::Success);
        }

        [[nodiscard]] i32 executeValidateConfig(const std::string& source, const bool jsonOutput)
        {
            const auto configPath = getConfigFilePath(source);

            if (!doesConfigDataExists(configPath))
            {
                if (jsonOutput)
                {
                    printValidationJson(false, configPath.string(), { ValidationError{ "Configuration file not found" } });
                }
                else
                {
                    Logger::error("Configuration file not found: {}\n", source);
                }

                return static_cast<i32>(ErrorCode::FailedToLoadConfig);
            }

            const auto configData = loadConfigData(configPath);
            if (!configData.has_value())
            {
                if (jsonOutput)
                {
                    printValidationJson(false, configPath.string(), { ValidationError{ "Failed to load configuration file" } });
                }
                else
                {
                    Logger::error("Failed to load configuration file: {}\n", source);
                }

                return static_cast<i32>(ErrorCode::FailedToLoadConfig);
            }

            const auto errors = validateConfigData(*configData);
            if (errors.empty())
            {
                if (jsonOutput)
                {
                    printValidationJson(true, configPath.string(), {});
                }
                else
                {
                    Logger::print("Configuration is valid: {}\n", source);
                }

                return static_cast<i32>(ErrorCode::Success);
            }

            if (jsonOutput)
            {
                printValidationJson(false, configPath.string(), errors);
            }
            else
            {
                Logger::error("Configuration validation failed for {}:\n", source);
                for (const ValidationError& error : errors)
                {
                    Logger::error("  - {}\n", error.message);
                }
            }

            return static_cast<i32>(ErrorCode::FailedToLoadConfig);
        }

        [[nodiscard]] i32 executeDuplicateConfig(
            const std::string& source,
            const std::string& destination,
            const bool force)
        {
            if (duplicateConfig(source, destination, force))
            {
                return static_cast<i32>(ErrorCode::Success);
            }

            return static_cast<i32>(ErrorCode::FailedToLoadConfig);
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
                Logger::fatal("The config {} command needs a SOURCE argument.\n", subcommand);
                return false;
            }

            data.source = sourceValue;
            ++index;

            const std::string_view destinationValue = safeGetNextArgument(index, args);
            if (destinationValue.empty())
            {
                Logger::fatal("The config {} command needs a DESTINATION argument.\n", subcommand);
                return false;
            }

            data.destination = destinationValue;
            ++index;
        }
        else
        {
            Logger::fatal("Unknown config subcommand: {}\n", subcommand);
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

            if (arg == "--json" || arg == "--examples")
            {
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
        switch (data.action)
        {
        case ConfigAction::List:
            return true;

        case ConfigAction::Validate:
            if (data.source.empty())
            {
                Logger::fatal("The config validate command needs a NAME_OR_PATH argument.\n");
                return false;
            }
            return true;

        case ConfigAction::Duplicate:
        case ConfigAction::Copy:
            if (data.source.empty())
            {
                Logger::fatal("The config {} command needs a SOURCE argument.\n", configActionToString(data.action));
                return false;
            }

            if (data.destination.empty())
            {
                Logger::fatal("The config {} command needs a DESTINATION argument.\n", configActionToString(data.action));
                return false;
            }

            return true;

        case ConfigAction::None:
        default:
            Logger::fatal("The config command needs a subcommand.\n");
            return false;
        }
    }

    i32 ConfigCommand::execute()
    {
        switch (data.action)
        {
        case ConfigAction::List:
            return executeListConfigs();

        case ConfigAction::Validate:
            return executeValidateConfig(data.source, m_context.global.jsonOutput);

        case ConfigAction::Duplicate:
        case ConfigAction::Copy:
            return executeDuplicateConfig(data.source, data.destination, data.force);

        case ConfigAction::None:
        default:
            Logger::fatal("The config command needs a subcommand.\n");
            return static_cast<i32>(ErrorCode::InvalidParam);
        }
    }

    void ConfigCommand::printHelp() const
    {
        if (m_helpTopics.size() >= 2)
        {
            const std::string& topic = m_helpTopics[1];

            if (topic == "list")
            {
                logHelpMessage({
                    .context = m_context,
                    .examples = {
                        "config list",
                    },
                });
                return;
            }

            if (topic == "validate")
            {
                logHelpMessage({
                    .context = m_context,
                    .examples = {
                        "config validate my-config",
                        "--json config validate my-config",
                    },
                    .notes = {
                        "Use the global --json option before the command for machine-readable output.",
                    },
                });
                return;
            }

            if (topic == "duplicate" || topic == "copy")
            {
                logHelpMessage({
                    .context = m_context,
                    .options = {
                        { .usage = "--force", .description = "Overwrite destination if it already exists" },
                    },
                    .examples = {
                        "config duplicate core-keeper-fishing my-copy",
                        "config duplicate core-keeper-fishing my-copy --force",
                        "config copy old-config new-config",
                    },
                });
                return;
            }
        }

        logHelpMessage({
            .context = m_context,
            .commands = {
                { .usage = "list", .description = "List available configurations" },
                { .usage = "validate NAME_OR_PATH", .description = "Validate a configuration file" },
                { .usage = "duplicate SOURCE DESTINATION", .description = "Duplicate a configuration into the user config directory" },
                { .usage = "copy SOURCE DESTINATION", .description = "Alias for duplicate" },
            },
            .examples = {
                "config list",
                "config validate my-config",
                "config duplicate old-config new-config",
                "config copy old-config new-config",
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

            if (topic == "list")
            {
                return {
                    .usage = "config list",
                    .description = "List available configurations.",
                };
            }

            if (topic == "validate")
            {
                return {
                    .usage = "config validate NAME_OR_PATH",
                    .description = "Validate a configuration file.",
                };
            }

            if (topic == "duplicate")
            {
                return {
                    .usage = "config duplicate SOURCE DESTINATION [options]",
                    .description = "Duplicate a configuration into the user config directory.",
                };
            }

            if (topic == "copy")
            {
                return {
                    .usage = "config copy SOURCE DESTINATION [options]",
                    .description = "Alias for config duplicate.",
                };
            }
        }

        return {
            .usage = "config <command> [options]",
            .description = "Manage autoinput configuration files.",
        };
    }
}
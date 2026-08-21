/**
 * @file runCommand.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/cli/runCommand.h"

#include "autoinput/cli/arguments.h"
#include "autoinput/cli/cliApplication.h"
#include "autoinput/config/config.h"
#include "autoinput/config/configMetadata.h"
#include "autoinput/support/errorCode.h"
#include "autoinput/support/logger.h"
#include <filesystem>

namespace autoinput::cli
{
    namespace
    {
        [[nodiscard]] bool applyConfigFileToArguments(const std::string& configName, ProgramArguments& arguments)
        {
            if (configName.empty())
            {
                return true;
            }

            const auto configPath = getConfigFilePath(configName);
            Logger::info("Loading configuration file: {}\n", configPath.string());

            if (!doesConfigDataExists(configPath))
            {
                Logger::error("Configuration file does not exist: {}\n", configPath.string());
                return false;
            }

            const auto foundConfigData = loadConfigData(configPath);
            if (!foundConfigData.has_value())
            {
                Logger::error("Failed to load configuration data from: {}\n", configPath.string());
                return false;
            }

            const ConfigData& configData = *foundConfigData;

            for (const CommandData& cmd : configData.commands)
            {
                const auto state = actionStateFromArguments(cmd.action);
                const auto action = state != ActionState::INVALID ? state : ActionState::CLICK;

                for (const std::string& button : cmd.buttons)
                {
                    const auto mouse = Mouse::fromString(button);
                    if (mouse.button == MouseButton::None)
                    {
                        Logger::fatalError({
                            .code = ErrorCode::InvalidParam,
                            .message = std::format("Invalid parameter {} for button type. Choices: {}", button, ConfigMetadata::validMouseButtonChoices())
                        });
                        return false;
                    }

                    arguments.buttons.push_back(mouse);
                    arguments.targetActions.push_back(action);
                    arguments.commandNames.push_back(cmd.name);
                    arguments.exclusiveGroups.push_back(cmd.exclusiveGroup);
                }

#if AUTOINPUT_HOOK_KEYBOARD_ENABLED
                for (const std::string& key : cmd.keys)
                {
                    arguments.keys.push_back(Key::fromString(key));
                    arguments.targetActions.push_back(action);
                    arguments.commandNames.push_back(cmd.name);
                    arguments.exclusiveGroups.push_back(cmd.exclusiveGroup);
                }
#endif // AUTOINPUT_HOOK_KEYBOARD_ENABLED

                for (const std::string& startKey : cmd.startKeys)
                {
                    arguments.startKeys.push_back(startKey);
                }

                if (!cmd.pressWait.empty())
                {
                    if (!arguments.delayData.parseWaitTimeDelay(cmd.pressWait, true))
                    {
                        return false;
                    }
                }

                if (!cmd.releaseWait.empty())
                {
                    if (!arguments.delayData.parseWaitTimeDelay(cmd.releaseWait, false))
                    {
                        return false;
                    }
                }
            }

            if (!configData.endKey.empty())
            {
                arguments.endKey = configData.endKey;
            }

            if (!configData.application.empty())
            {
                arguments.applicationName = configData.application;
            }

            if (!configData.blacklist.empty())
            {
                arguments.blacklist = configData.blacklist;
            }

            if (!configData.statusNotificationMode.empty())
            {
                arguments.statusNotificationMode = statusNotificationModeFromString(configData.statusNotificationMode);
            }

            if (!configData.sequences.empty())
            {
                arguments.sequences.insert(
                    arguments.sequences.end(),
                    configData.sequences.begin(),
                    configData.sequences.end()
                );
            }

            return true;
        }

        [[nodiscard]] bool applyRunConfigToArguments(
            const RunConfig& config,
            const CommandContext& context,
            ProgramArguments& arguments)
        {
            arguments.programName = context.global.programName;
            arguments.jsonOutput = context.global.jsonOutput;

            arguments.statusNotificationMode = config.statusNotificationMode;

            if (!applyConfigFileToArguments(config.configName, arguments))
            {
                return false;
            }

            for (const auto& target : config.targets)
            {
                ActionState action = target.action != ActionState::INVALID ? target.action : ActionState::CLICK;

                if (target.mouse.has_value())
                {
                    arguments.buttons.push_back(*target.mouse);
                    arguments.targetActions.push_back(action);
                }
                else if (target.key.has_value())
                {
                    arguments.keys.push_back(*target.key);
                    arguments.targetActions.push_back(action);
                }
                else
                {
                    // Default behavior for start-only target
                    arguments.buttons.emplace_back( MouseButton::Left );
                    arguments.targetActions.push_back(action);
                }

                if (!target.startKey.empty())
                {
                    arguments.startKeys.push_back(target.startKey);
                }
            }

            arguments.blacklist.insert(arguments.blacklist.end(), config.blacklist.begin(), config.blacklist.end());

            if (!config.endKey.empty())
            {
                arguments.endKey = config.endKey;
            }

            if (!config.applicationName.empty())
            {
                arguments.applicationName = config.applicationName;
            }

            if (!config.saveConfigName.empty())
            {
                arguments.saveConfigName = config.saveConfigName;
            }

            if (config.delayData.hasPress)
            {
                arguments.delayData.minWaitPressDelay = config.delayData.minWaitPressDelay;
                arguments.delayData.maxWaitPressDelay = config.delayData.maxWaitPressDelay;
                arguments.delayData.usePressRange = config.delayData.usePressRange;
                arguments.delayData.hasPress = true;
            }

            if (config.delayData.hasRelease)
            {
                arguments.delayData.minWaitReleaseDelay = config.delayData.minWaitReleaseDelay;
                arguments.delayData.maxWaitReleaseDelay = config.delayData.maxWaitReleaseDelay;
                arguments.delayData.useReleaseRange = config.delayData.useReleaseRange;
                arguments.delayData.hasRelease = true;
            }

            return arguments.postParseArguments();
        }
    }

    bool RunCommand::parse(const gsl::span<char*> args, i32& index)
    {
        while (index < args.size())
        {
            const std::string_view arg = args[index];

            if (arg == "-c" || arg == "--config")
            {
                const std::string_view value = safeGetNextArgument(++index, args);
                if (value.empty())
                {
                    Logger::fatalError({
                        .code = ErrorCode::MissingCommandLineArgument,
                        .message = std::format("The parameter {} needs an argument.", arg)
                    });
                    return false;
                }

                m_config.configName = value;
                ++index;
                continue;
            }

            if (arg == "-t" || arg == "--type")
            {
                const std::string_view value = safeGetNextArgument(++index, args);
                if (value.empty())
                {
                    Logger::fatalError({
                        .code = ErrorCode::MissingCommandLineArgument,
                        .message = std::format("The parameter {} needs an argument.", arg)
                    });
                    return false;
                }

                const ActionState action = actionStateFromArguments(value);
                if (action == ActionState::INVALID)
                {
                    Logger::fatalError({
                        .code = ErrorCode::InvalidParam,
                        .message = std::format("Invalid parameter {} for action type. Choices: {}", value, ConfigMetadata::validActionChoices())
                    });
                    return false;
                }

                m_config.pendingAction = action;
                ++index;
                continue;
            }

#if AUTOINPUT_HOOK_MOUSE_ENABLED
            if (arg == "-b" || arg == "--button" || arg == "--btn")
            {
                const std::string_view value = safeGetNextArgument(++index, args);
                if (value.empty())
                {
                    Logger::fatalError({
                    .code = ErrorCode::MissingCommandLineArgument,
                    .message = std::format("The parameter {} needs an argument. Choices: {}", arg, ConfigMetadata::validMouseButtonChoices())
                    });
                    return false;
                }

                const Mouse mouse = Mouse::fromString(value);
                if (mouse.button == MouseButton::None)
                {
                    Logger::fatalError({
                        .code = ErrorCode::InvalidParam,
                        .message = std::format("Invalid parameter {} for button type. Choices: {}", value, ConfigMetadata::validMouseButtonChoices())
                    });
                    return false;
                }

                RunTarget target;
                target.action = m_config.pendingAction != ActionState::INVALID ? m_config.pendingAction : ActionState::CLICK;
                target.mouse = mouse;
                m_config.targets.push_back(target);

                ++index;
                continue;
            }
#endif // AUTOINPUT_HOOK_MOUSE_ENABLED

#if AUTOINPUT_HOOK_KEYBOARD_ENABLED
            if (arg == "-k" || arg == "--key")
            {
                const std::string_view value = safeGetNextArgument(++index, args);
                if (value.empty())
                {
                    Logger::fatalError({
                        .code = ErrorCode::MissingCommandLineArgument,
                        .message = std::format("The parameter {} needs an argument.", arg)
                    });
                    return false;
                }

                RunTarget target;
                target.action = m_config.pendingAction != ActionState::INVALID ? m_config.pendingAction : ActionState::CLICK;
                target.key = Key::fromString(value);
                m_config.targets.push_back(target);

                ++index;
                continue;
            }
#endif // AUTOINPUT_HOOK_KEYBOARD_ENABLED

            if (arg == "-s" || arg == "--start")
            {
                const std::string_view value = safeGetNextArgument(++index, args);
                if (value.empty())
                {
                    Logger::fatalError({
                        .code = ErrorCode::MissingCommandLineArgument,
                        .message = std::format("The parameter {} needs an argument.", arg)
                    });
                    return false;
                }

                if (!m_config.targets.empty() && m_config.targets.back().startKey.empty())
                {
                    m_config.targets.back().startKey = std::string(value);
                }
                else
                {
                    RunTarget target;
                    target.action = m_config.pendingAction != ActionState::INVALID ? m_config.pendingAction : ActionState::CLICK;
                    target.startKey = std::string(value);
                    m_config.targets.push_back(target);
                }

                ++index;
                continue;
            }

            if (arg == "-e" || arg == "--end")
            {
                const std::string_view value = safeGetNextArgument(++index, args);
                if (value.empty())
                {
                    Logger::fatalError({
                        .code = ErrorCode::MissingCommandLineArgument,
                        .message = std::format("The parameter {} needs an argument.", arg)
                    });
                    return false;
                }

                m_config.endKey = value;
                ++index;
                continue;
            }

            if (arg == "-a" || arg == "--app" || arg == "--application")
            {
                const std::string_view value = safeGetNextArgument(++index, args);
                if (value.empty())
                {
                    Logger::fatalError({
                        .code = ErrorCode::MissingCommandLineArgument,
                        .message = std::format("The parameter {} needs an argument.", arg)
                    });
                    return false;
                }

                m_config.applicationName = value;
                ++index;
                continue;
            }

            if (arg == "-B" || arg == "--blacklist")
            {
                const std::string_view value = safeGetNextArgument(++index, args);
                if (value.empty())
                {
                    Logger::fatalError({
                        .code = ErrorCode::MissingCommandLineArgument,
                        .message = std::format("The parameter {} needs an argument.", arg)
                    });
                    return false;
                }

                m_config.blacklist.emplace_back(value);
                ++index;
                continue;
            }

            if (arg == "-S" || arg == "--save-config")
            {
                const std::string_view value = safeGetNextArgument(++index, args);
                if (value.empty())
                {
                    Logger::fatalError({
                        .code = ErrorCode::MissingCommandLineArgument,
                        .message = std::format("The parameter {} needs an argument.", arg)
                    });
                    return false;
                }

                m_config.saveConfigName = value;
                ++index;
                continue;
            }

            if (arg == "-w" || arg == "--wait" || arg == "--press-wait")
            {
                const std::string_view value = safeGetNextArgument(++index, args);
                if (value.empty())
                {
                    Logger::fatalError({
                        .code = ErrorCode::MissingCommandLineArgument,
                        .message = std::format("The parameter {} needs an argument.", arg)
                    });
                    return false;
                }

                if (!m_config.delayData.parseWaitTimeDelay(value, true))
                {
                    Logger::fatalError({
                        .code = ErrorCode::InvalidParam,
                        .message = std::format("The parameter {} needs a valid time range.", arg)
                    });
                    return false;
                }

                ++index;
                continue;
            }

            if (arg == "--release-wait")
            {
                const std::string_view value = safeGetNextArgument(++index, args);
                if (value.empty())
                {
                    Logger::fatalError({
                        .code = ErrorCode::MissingCommandLineArgument,
                        .message = std::format("The parameter {} needs an argument.", arg)
                    });
                    return false;
                }

                if (!m_config.delayData.parseWaitTimeDelay(value, false))
                {
                    Logger::fatalError({
                        .code = ErrorCode::InvalidParam,
                        .message = std::format("The parameter {} needs a valid time range.", arg)
                    });
                    return false;
                }

                ++index;
                continue;
            }

            if (arg == "--status-notification")
            {
                const std::string_view value = safeGetNextArgument(++index, args);
                if (value.empty())
                {
                    Logger::fatalError({
                        .code = ErrorCode::MissingCommandLineArgument,
                        .message = std::format("The parameter {} needs an argument. Choices: off, console, desktop, both", arg)
                    });
                    return false;
                }

                m_config.statusNotificationMode = statusNotificationModeFromString(value);
                ++index;
                continue;
            }

            Logger::fatalError({
                .code = ErrorCode::UnknownCommandOption,
                .message = std::format("Unknown run option: {}", arg)
            });
            return false;
        }

        return true;
    }


    bool RunCommand::validate() const
    {
        if (!m_config.configName.empty())
        {
            return true;
        }

        return true; // Defaults allow `autoinput run`
    }

    ErrorCode RunCommand::execute()
    {
        if (!m_config.saveConfigName.empty())
        {
            ProgramArguments arguments;
            if (!applyRunConfigToArguments(m_config, m_context, arguments))
            {
                return ErrorCode::InvalidParam;
            }

            std::filesystem::path dumpPath = m_config.saveConfigName;
            if (dumpPath.extension() != ".toml")
            {
                dumpPath += ".toml";
            }

            if (!dumpPath.is_absolute())
            {
                dumpPath = getUserConfigsPath() / dumpPath;
            }

            if (!dumpPath.parent_path().empty())
            {
                std::filesystem::create_directories(dumpPath.parent_path());
            }

            if (saveConfigData(arguments.toConfigData(), dumpPath, m_context.settings.getDefaults()))
            {
                Logger::info("Configuration saved to: {}\n", dumpPath.string());
                return ErrorCode::Success;
            }

            Logger::error("Failed to save configuration to: {}\n", dumpPath.string());
            return ErrorCode::FailedToLoadConfig;
        }

        return runProgramWithArguments([this](ProgramArguments& arguments)
        {
            return applyRunConfigToArguments(m_config, m_context, arguments);
        });
    }

    void RunCommand::printHelp() const
    {
        if (const HelpMetadata::CliCommandMetadata* metadata = HelpMetadata::findCommand(getName()))
        {
            const std::vector<std::string> topics{ std::string(getName()) };
            renderCommandHelp(*metadata, m_context, topics);
        }
    }

    HelpEntry RunCommand::getHelpEntry() const
    {
        if (const HelpMetadata::CliCommandMetadata* metadata = HelpMetadata::findCommand(getName()))
        {
            return { .usage = std::string(metadata->usage), .description = std::string(metadata->description) };
        }
        return {};
    }
}

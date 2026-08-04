/**
 * @file runCommand.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/cli/runCommand.h"

#include "autoinput/arguments.h"
#include "autoinput/cli/cliApplication.h"
#include "autoinput/config.h"
#include "autoinput/configMetadata.h"
#include "autoinput/errorCode.h"
#include "autoinput/logger.h"
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

            if (!doesConfigDataExists(configPath))
            {
                return false;
            }

            const auto foundConfigData = loadConfigData(configPath);
            if (!foundConfigData.has_value())
            {
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
                        Logger::fatal("Invalid parameter {} for button type. Choices: {}\n", button, ConfigMetadata::validMouseButtonChoices());
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
                if (!configData.appendBlacklist)
                {
                    arguments.blacklist.clear();
                }

                arguments.blacklist.insert(
                    arguments.blacklist.end(),
                    configData.blacklist.begin(),
                    configData.blacklist.end()
                );
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

            arguments.buttons.insert(arguments.buttons.end(), config.buttons.begin(), config.buttons.end());
            arguments.keys.insert(arguments.keys.end(), config.keys.begin(), config.keys.end());
            arguments.startKeys.insert(arguments.startKeys.end(), config.startKeys.begin(), config.startKeys.end());
            arguments.targetActions.insert(arguments.targetActions.end(), config.targetActions.begin(), config.targetActions.end());
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
                    Logger::fatal("The parameter {} needs an argument.\n", arg);
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
                    Logger::fatal("The parameter {} needs an argument. Choices: {}\n", arg, ConfigMetadata::validActionChoices());
                    return false;
                }

                const ActionState action = actionStateFromArguments(value);
                if (action == ActionState::INVALID)
                {
                    Logger::fatal("Invalid parameter {} for action type. Choices: {}\n", value, ConfigMetadata::validActionChoices());
                    return false;
                }

                m_config.targetActions.push_back(action);
                ++index;
                continue;
            }

#if AUTOINPUT_HOOK_MOUSE_ENABLED
            if (arg == "-b" || arg == "--button" || arg == "--btn")
            {
                const std::string_view value = safeGetNextArgument(++index, args);
                if (value.empty())
                {
                    Logger::fatal("The parameter {} needs an argument. Choices: {}\n", arg, ConfigMetadata::validMouseButtonChoices());
                    return false;
                }

                const Mouse mouse = Mouse::fromString(value);
                if (mouse.button == MouseButton::None)
                {
                    Logger::fatal("Invalid parameter {} for button type. Choices: {}\n", value, ConfigMetadata::validMouseButtonChoices());
                    return false;
                }

                m_config.buttons.push_back(mouse);
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
                    Logger::fatal("The parameter {} needs an argument.\n", arg);
                    return false;
                }

                m_config.keys.push_back(Key::fromString(value));
                ++index;
                continue;
            }
#endif // AUTOINPUT_HOOK_KEYBOARD_ENABLED

            if (arg == "-s" || arg == "--start")
            {
                const std::string_view value = safeGetNextArgument(++index, args);
                if (value.empty())
                {
                    Logger::fatal("The parameter {} needs an argument.\n", arg);
                    return false;
                }

                m_config.startKeys.emplace_back(value);
                ++index;
                continue;
            }

            if (arg == "-e" || arg == "--end")
            {
                const std::string_view value = safeGetNextArgument(++index, args);
                if (value.empty())
                {
                    Logger::fatal("The parameter {} needs an argument.\n", arg);
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
                    Logger::fatal("The parameter {} needs an argument.\n", arg);
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
                    Logger::fatal("The parameter {} needs an argument.\n", arg);
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
                    Logger::fatal("The parameter {} needs an argument.\n", arg);
                    return false;
                }

                m_config.saveConfigName = value;
                ++index;
                continue;
            }

            if (arg == "-w" || arg == "--wait" || arg == "--press-wait")
            {
                const std::string_view value = safeGetNextArgument(++index, args);
                if (value.empty() || !m_config.delayData.parseWaitTimeDelay(value, true))
                {
                    Logger::fatal("The parameter {} needs a valid time range.\n", arg);
                    return false;
                }

                ++index;
                continue;
            }

            if (arg == "--release-wait")
            {
                const std::string_view value = safeGetNextArgument(++index, args);
                if (value.empty() || !m_config.delayData.parseWaitTimeDelay(value, false))
                {
                    Logger::fatal("The parameter {} needs a valid time range.\n", arg);
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
                    Logger::fatal("The parameter {} needs an argument. Choices: off, console, desktop, both\n", arg);
                    return false;
                }

                m_config.statusNotificationMode = statusNotificationModeFromString(value);
                ++index;
                continue;
            }

            if (arg == "--json" || arg == "--examples")
            {
                ++index;
                continue;
            }

            Logger::fatal("Unknown run option: {}\n", arg);
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

    i32 RunCommand::execute()
    {
        if (!m_config.saveConfigName.empty())
        {
            ProgramArguments arguments;
            if (!applyRunConfigToArguments(m_config, m_context, arguments))
            {
                return static_cast<i32>(ErrorCode::InvalidParam);
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
                Logger::print("Configuration saved to: {}\n", dumpPath.string());
                return static_cast<i32>(ErrorCode::Success);
            }

            Logger::error("Failed to save configuration to: {}\n", dumpPath.string());
            return static_cast<i32>(ErrorCode::FailedToLoadConfig);
        }

        return runProgramWithArguments([this](ProgramArguments& arguments)
        {
            return applyRunConfigToArguments(m_config, m_context, arguments);
        });
    }

    void RunCommand::printHelp() const
    {
        logHelpMessage({
            .context = m_context,
            .options ={
            { .usage = "-c, --config NAME_OR_PATH", .description = "Load a TOML configuration" },
            { .usage = "-t, --type click|hold", .description = "Set action type" },
            { .usage = "-b, --button BUTTON", .description = "Mouse button target" },
            { .usage = "-k, --key KEY", .description = "Keyboard key target" },
            { .usage = "-s, --start KEY", .description = "Start/toggle trigger" },
            { .usage = "-e, --end KEY", .description = "Stop trigger" },
            { .usage = "-a, --app APPLICATION", .description = "Only run while application is focused" },
            { .usage = "-B, --blacklist APPLICATION", .description = "Pause while application is focused" },
            { .usage = "-w, --wait RANGE", .description = "Alias for --press-wait" },
            { .usage = "--press-wait RANGE", .description = "Delay while target is pressed" },
            { .usage = "--release-wait RANGE", .description = "Delay between repeated actions" },
            { .usage = "--status-notification MODE", .description = "off, console, desktop, both" },
            { .usage = "-S, --save-config NAME", .description = "Save current options as a user config" },
            },
            .examples = {
                "run --button left",
                "run --type hold --button left --start f2",
                "run --key space --start f6 --release-wait 1s",
                "run --config left-click-press",
            }
        }
        );
    }

    HelpEntry RunCommand::getHelpEntry() const
    {
        return {
            .usage = std::format("{} [options]", getName()),
            .description = "Run input automation from command options or a TOML configuration."
        };
    }
}

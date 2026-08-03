/**
 * @file arguments.cpp
 * @author djsquiddy
 * @date March 2026
 */
#include "autoinput/arguments.h"
#include "autoinput/waitDelay.h"
#include "autoinput/defaults.h"
#include "autoinput/cliHelpFormatter.h"
#include "autoinput/logger.h"
#include "autoinput/config.h"
#include "autoinput/configMetadata.h"

namespace autoinput
{
    ProgramArguments::ProgramArguments()
        : NonCopyable()
    {
    }

    bool ProgramArguments::parseArguments(const gsl::span<char*> args, const bool loadSettings)
    {
        if (loadSettings)
        {
            m_settings.load();
        }

        if (!args.empty())
        {
            programName = args[0];
        }

        if (args.size() <= 1)
        {
            // We only get the program name listed.
            printUsage();
            return false;
        }

        if (!parseEarlyOptions(args))
        {
            return false;
        }

        if (!parseCommandOptions(args))
        {
            return false;
        }

        return postParseArguments();
    }

    bool ProgramArguments::parseEarlyOptions(const gsl::span<char*> args)
    {
        showHelpExamples = contains(args, "--examples");

        for (int i = 1; i < args.size(); ++i)
        {
            const std::string_view arg = args[i];
            if (arg == "-h" || arg == "--help")
            {
                printUsage(showHelpExamples);
                return false;
            }
            if (arg == "--examples")
            {
                printUsage(true);
                return false;
            }
            if (arg == "-l" || arg == "--log")
            {
                const std::string_view logLevelStr = safeGetNextArgument(++i, args);
                if (logLevelStr.empty())
                {
                    printUsage();
                    Logger::fatal("The parameter {} needs an argument. Choices: {}\n", arg, ConfigMetadata::validLogLevelChoices());
                    return false;
                }
                const LogLevel logLevel = logLevelFromString(logLevelStr);
                if (logLevel == LogLevel::Unknown)
                {
                    printUsage();
                    Logger::fatal("Invalid parameter {} for log level. Choices: {}\n", logLevelStr, ConfigMetadata::validLogLevelChoices());
                    return false;
                }
                Logger::setLogLevel(logLevel);
                continue;
            }
            if (arg == "--json")
            {
                jsonOutput = true;
                continue;
            }
            if (arg == "--status-notification")
            {
                const std::string_view modeStr = safeGetNextArgument(++i, args);
                if (modeStr.empty())
                {
                    printUsage();
                    Logger::fatal("The parameter {} needs an argument. Choices: off, console, desktop, both\n", arg);
                    return false;
                }
                statusNotificationMode = statusNotificationModeFromString(modeStr);
                continue;
            }
            // We want to parse the configuration first so we can use cli arguments as overrides if need/wanted.
            if (arg == "-c" || arg == "--config")
            {
                if (!parseConfigArguments(args, i))
                {
                    return false;
                }
            }
            if (arg == "--validate-config")
            {
                validateConfigName = safeGetNextArgument(++i, args);
                if (validateConfigName.empty())
                {
                    printUsage();
                    Logger::fatal("The parameter {} needs an argument.\n", arg);
                    return false;
                }
            }
            if (arg == "--duplicate-config" || arg == "--copy-config")
            {
                duplicateConfigSource = safeGetNextArgument(++i, args);
                if (duplicateConfigSource.empty())
                {
                    printUsage();
                    Logger::fatal("The parameter {} needs a source argument.\n", arg);
                    return false;
                }
                duplicateConfigDestination = safeGetNextArgument(++i, args);
                if (duplicateConfigDestination.empty())
                {
                    printUsage();
                    Logger::fatal("The parameter {} needs a destination argument.\n", arg);
                    return false;
                }
            }
            if (arg == "--force")
            {
                forceOverwrite = true;
            }
            if (arg == "--record")
            {
                recordName = safeGetNextArgument(++i, args);
                if (recordName.empty())
                {
                    printUsage();
                    Logger::fatal("The parameter {} needs an argument.\n", arg);
                    return false;
                }
            }
            if (arg == "--record-start")
            {
                recordStartKey = safeGetNextArgument(++i, args);
                if (recordStartKey.empty())
                {
                    printUsage();
                    Logger::fatal("The parameter {} needs an argument.\n", arg);
                    return false;
                }
            }
            if (arg == "--record-end")
            {
                recordEndKey = safeGetNextArgument(++i, args);
                if (recordEndKey.empty())
                {
                    printUsage();
                    Logger::fatal("The parameter {} needs an argument.\n", arg);
                    return false;
                }
            }
            if (arg == "--record-play-start")
            {
                recordPlayStartKey = safeGetNextArgument(++i, args);
                if (recordPlayStartKey.empty())
                {
                    printUsage();
                    Logger::fatal("The parameter {} needs an argument.\n", arg);
                    return false;
                }
            }
            if (arg == "--record-mouse-moves")
            {
                recordMouseMoves = true;
            }
            if (arg == "--record-mouse-sample")
            {
                recordMouseSample = safeGetNextArgument(++i, args);
                if (recordMouseSample.empty())
                {
                    printUsage();
                    Logger::fatal("The parameter {} needs an argument.\n", arg);
                    return false;
                }
            }
        }
        return true;
    }

    bool ProgramArguments::parseCommandOptions(gsl::span<char*> args)
    {
        for (int i = 1; i < args.size(); ++i)
        {
            const std::string_view arg = args[i];

            Logger::debug("processing argument[{}]: {}\n", i, arg);

            if (arg == "-t" || arg == "--type")
            {
                if (!parseActionState(args, i))
                {
                    return false;
                }
            }
#if AUTOINPUT_HOOK_KEYBOARD_ENABLED
            else if (arg == "-k" || arg == "--key")
            {
                if (!parseKey(args, i))
                {
                    return false;
                }
                continue;
            }
#endif // AUTOINPUT_HOOK_KEYBOARD_ENABLED
#if AUTOINPUT_HOOK_MOUSE_ENABLED
            else if (arg == "-b" || arg == "--button" || arg == "--btn")
            {
                if (!parseButton(args, i))
                {
                    return false;
                }
            }
#endif // AUTOINPUT_HOOK_MOUSE_ENABLED
            else if (arg == "-s" || arg == "--start" || arg == "--start-key")
            {
                if (!parseStartKey(args, i))
                {
                    return false;
                }
            }
            else if (arg == "-e" || arg == "--end" || arg == "--end-key")
            {
                if (!parseEndKey(args, i))
                {
                    return false;
                }
            }
            else if (arg == "-a" || arg == "--app" || arg == "--application")
            {
                applicationName = safeGetNextArgument(++i, args);
                if (applicationName.empty())
                {
                    printUsage();
                    Logger::fatal("The parameter {} needs an argument.\n", arg);
                    return false;
                }
            }
            else if (arg == "-B" || arg == "--blacklist")
            {
                const std::string_view blacklistApp = safeGetNextArgument(++i, args);
                if (blacklistApp.empty())
                {
                    printUsage();
                    Logger::fatal("The parameter {} needs an argument.\n", arg);
                    return false;
                }
                blacklist.emplace_back(blacklistApp);
            }
            else if (arg == "-L" || arg == "--list-apps")
            {
                listApplications = true;
            }
            else if (arg == "-C" || arg == "--list-configs")
            {
                listConfigs = true;
            }
            else if (arg == "-S" || arg == "--save-config")
            {
                saveConfigName = safeGetNextArgument(++i, args);
                if (saveConfigName.empty())
                {
                    printUsage();
                    Logger::fatal("The parameter {} needs an argument.\n", arg);
                    return false;
                }
            }
            else if (arg == "-w" || arg == "--wait" || arg == "--press-wait" || arg == "--release-wait")
            {
                if (arg == "-w" || arg == "--wait" || arg.contains("press"))
                {
                    if (!parsePressWaitTime(args, i))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!parseReleaseTime(args, i))
                    {
                        return false;
                    }
                }
            }
            else if (arg == "-c" || arg == "--config")
            {
                // Already handled in first pass, but skip its value
                safeGetNextArgument(++i, args);
            }
            else if (arg == "--validate-config")
            {
                // Already handled in first pass, but skip its value
                safeGetNextArgument(++i, args);
            }
            else if (arg == "--duplicate-config" || arg == "--copy-config")
            {
                // Already handled in first pass, but skip its values
                safeGetNextArgument(++i, args);
                safeGetNextArgument(++i, args);
            }
            else if (arg == "--force")
            {
                // Already handled in first pass
            }
            else if (arg == "--record")
            {
                // Already handled in first pass, but skip its value
                safeGetNextArgument(++i, args);
            }
            else if (arg == "--record-start")
            {
                // Already handled in first pass, but skip its value
                safeGetNextArgument(++i, args);
            }
            else if (arg == "--record-end")
            {
                // Already handled in first pass, but skip its value
                safeGetNextArgument(++i, args);
            }
            else if (arg == "--record-play-start")
            {
                // Already handled in first pass, but skip its value
                safeGetNextArgument(++i, args);
            }
            else if (arg == "--record-mouse-moves")
            {
                // Already handled in first pass
            }
            else if (arg == "--record-mouse-sample")
            {
                // Already handled in first pass, but skip its value
                safeGetNextArgument(++i, args);
            }
            else if (arg == "--json")
            {
                // Already handled in first pass
            }
            else if (arg == "--status-notification")
            {
                // Already handled in first pass, but skip its value
                safeGetNextArgument(++i, args);
            }
            else if (arg == "-l" || arg == "--log")
            {
                // Already handled in first pass, but skip its value
                safeGetNextArgument(++i, args);
            }
            else if (arg.starts_with('-'))
            {
                Logger::warn("Unknown argument: {}\n", arg);
            }
            else
            {
                if (!parsePositionalArgument(arg, args, i))
                {
                    return false;
                }
            }
        }
        return true;
    }

    bool ProgramArguments::parsePositionalArgument(const std::string_view arg, const gsl::span<char*> args, int& i)
    {
        // Positional argument
        if (const auto state = actionStateFromArguments(arg); state != ActionState::INVALID)
        {
            actionState = state;
            return true;
        }

        auto registerTarget = [&]() {
            targetActions.emplace_back(actionState != ActionState::INVALID ? actionState : ActionState::CLICK);

            // Look ahead for potential start key
            if (const std::string_view nextArg = safeGetNextArgument(i + 1, args); !nextArg.empty() && !nextArg.starts_with('-'))
            {
                const bool isNextAction = actionStateFromArguments(nextArg) != ActionState::INVALID;
                const auto [character, modifier] = Key::fromString(nextArg);
                const bool isNextFunctionKey = modifier == KeyModifier::Function;

                // We treat it as a start key if it's a function key or if it's a trigger button (back/forward)
                // and it's not a standard action.
                if (!isNextAction)
                {
                    bool isPotentialStartKey = isNextFunctionKey;
                    if (const auto nextButton = mouseButtonFromArguments(nextArg); nextButton == MouseButton::Back || nextButton == MouseButton::Forward)
                    {
                        isPotentialStartKey = true;
                    }

                    if (isPotentialStartKey)
                    {
                        startKeys.emplace_back(nextArg);
                        ++i;
                    }
                }
            }
        };

        const auto mouse = Mouse::fromString(arg);
        if (mouse.button != MouseButton::None)
        {
            if (mouse.button == MouseButton::Back || mouse.button == MouseButton::Forward)
            {
                if ((buttons.size() + keys.size()) == startKeys.size())
                {
                    // Trigger for default target
                    buttons.emplace_back(MouseButton::Left);
                    targetActions.emplace_back(actionState != ActionState::INVALID ? actionState : ActionState::CLICK);
                    startKeys.emplace_back(arg);
                    return true;
                }
            }
            buttons.emplace_back(mouse);
            registerTarget();
            return true;
        }

#if AUTOINPUT_HOOK_KEYBOARD_ENABLED
        if (Key key = Key::fromString(arg); !key.character.empty() || key.modifier != KeyModifier::None)
        {
            if (key.modifier == KeyModifier::Function)
            {
                if ((buttons.size() + keys.size()) == startKeys.size())
                {
                    // Trigger for default target
                    buttons.emplace_back(MouseButton::Left);
                    targetActions.emplace_back(actionState != ActionState::INVALID ? actionState : ActionState::CLICK);
                    startKeys.emplace_back(arg);
                    return true;
                }
            }
            keys.emplace_back(key);
            registerTarget();
            return true;
        }
#endif
        Logger::warn("Unrecognized positional argument: {}\n", arg);
        return true;
    }

    bool ProgramArguments::postParseArguments()
    {
        applyDefaults();

        if (!recordName.empty() && saveConfigName.empty())
        {
            std::filesystem::path destPath = getUserConfigsPath() / (recordName + ".toml");
            saveConfigName = destPath.string();
        }
        
        return true;
    }

    void ProgramArguments::applyDefaults()
    {
        const auto& [start, end, press, release, action, button, settingsBlacklist, statusNotification, settingsLogLevel] = m_settings.getDefaults();

        if (!settingsBlacklist.empty())
        {
            blacklist.insert(blacklist.end(), settingsBlacklist.begin(), settingsBlacklist.end());
        }

        if (statusNotificationMode == StatusNotificationMode::Console)
        {
            if (!statusNotification.empty())
            {
                statusNotificationMode = statusNotificationModeFromString(statusNotification);
            }
        }

        if (Logger::getLogLevel() == LogLevel::Info) // Default
        {
            if (!settingsLogLevel.empty())
            {
                const LogLevel level = logLevelFromString(settingsLogLevel);
                if (level != LogLevel::Unknown)
                {
                    Logger::setLogLevel(level);
                }
            }
        }

        if (actionState == ActionState::INVALID)
        {
            if (!action.empty())
            {
                actionState = actionStateFromArguments(action);
            }

            if (actionState == ActionState::INVALID)
            {
                actionState = ActionState::CLICK;
            }
        }

        if (buttons.empty() && keys.empty())
        {
            if (!button.empty())
            {
                if (const auto mouse = Mouse::fromString(button); mouse.button != MouseButton::None)
                {
                    buttons.emplace_back(mouse);
                }
            }

            if (buttons.empty())
            {
                buttons.emplace_back(MouseButton::Left);
            }
        }

        const size_t targetCount = buttons.size() + keys.size();
        if (targetActions.empty())
        {
            targetActions.resize(targetCount, actionState);
        }
        else if (targetActions.size() < targetCount)
        {
            targetActions.resize(targetCount, targetActions.back());
        }

        if (startKeys.empty())
        {
            startKeys.emplace_back(!start.empty() ? start : defaults::StartKey);
        }
        if (endKey.empty())
        {
            endKey = !end.empty() ? end : defaults::EndKey;
        }

        if (!delayData.hasPress && !press.empty())
        {
            delayData.parseWaitTimeDelay(press, true);
        }
        if (!delayData.hasRelease && !release.empty())
        {
            delayData.parseWaitTimeDelay(release, false);
        }

        if (targetCount != startKeys.size())
        {
            if (startKeys.size() < targetCount)
            {
                startKeys.resize(targetCount, startKeys.back());
            }
        }

        if (recordStartKey.empty())
        {
            recordStartKey = defaults::RecordStartKey;
        }
        if (recordEndKey.empty())
        {
            recordEndKey = defaults::RecordEndKey;
        }
        if (recordPlayStartKey.empty())
        {
            recordPlayStartKey = defaults::RecordPlayStartKey;
        }
        if (recordMouseSample.empty())
        {
            recordMouseSample = defaults::DefaultRecordMouseSample;
        }
    }

    void ProgramArguments::printUsage(const bool verbose) const
    {
        CliHelpFormatter::printUsage(programName, verbose);
    }

    ConfigData ProgramArguments::toConfigData() const
    {
        ConfigData data;
        data.endKey = endKey;
        data.application = applicationName;
        data.blacklist = blacklist;
        data.appendBlacklist = true; // CLI arguments always append or we don't have enough info to know if they intended to replace.
        data.statusNotificationMode = statusNotificationModeToString(statusNotificationMode);
        data.logLevel = logLevelToString(Logger::getLogLevel());
        // Actually, we don't store appendBlacklist in ProgramArguments itself.
        // For now, let's just keep the default true.

        const size_t buttonCount = buttons.size();
        const size_t keyCount = keys.size();
        const size_t targetCount = buttonCount + keyCount;
        const size_t actionCount = targetActions.size();
        const size_t startKeyCount = startKeys.size();
        const size_t nameCount = commandNames.size();
        const size_t groupCount = exclusiveGroups.size();

        for (size_t i = 0; i < targetCount; ++i)
        {
            CommandData cmd;
            cmd.name = i < nameCount ? commandNames[i] : "";
            cmd.exclusiveGroup = i < groupCount ? exclusiveGroups[i] : "";
            cmd.action = actionStateToString(i < actionCount ? targetActions[i] : ActionState::CLICK);

            if (i < buttonCount)
            {
                cmd.buttons.push_back(buttons[i].toString());
            }
            else
            {
                cmd.keys.push_back(keys[i - buttonCount].toString());
            }

            if (i < startKeyCount)
            {
                cmd.startKeys.push_back(startKeys[i]);
            }

            cmd.pressWait = delayData.toString(true);
            cmd.releaseWait = delayData.toString(false);

            data.commands.push_back(std::move(cmd));
        }

        data.sequences = sequences;

        return data;
    }

    bool ProgramArguments::parseConfigArguments(gsl::span<char*> args, int& i)
    {
        // TODO: implement
        std::string_view fileName = safeGetNextArgument(++i, args);
        if (fileName.empty())
        {
            printUsage();
            Logger::error("No configuration file name given");
            return false;
        }
        const auto configPath = getConfigFilePath(std::string(fileName));

        if (!doesConfigDataExists(configPath))
        {
            printUsage();
            return false;
        }
        if (const auto foundConfigData = autoinput::loadConfigData(configPath); foundConfigData.has_value())
        {
            const auto& configData = *foundConfigData;
            for (const auto& cmd : configData.commands)
            {
                const auto state = actionStateFromArguments(cmd.action);
                const auto action = state != ActionState::INVALID ? state : ActionState::CLICK;

                for (const auto& button : cmd.buttons)
                {
                    if (const auto mouse = Mouse::fromString(button); mouse.button != MouseButton::None)
                    {
                        buttons.push_back(mouse);
                        targetActions.push_back(action);
                        commandNames.push_back(cmd.name);
                        exclusiveGroups.push_back(cmd.exclusiveGroup);
                    }
                    else
                    {
                        Logger::fatal("Invalid parameter {} for button type. Choices: {}\n", button, ConfigMetadata::validMouseButtonChoices());
                        printUsage();
                        return false;
                    }
                }

#if AUTOINPUT_HOOK_KEYBOARD_ENABLED
                for (const auto& key : cmd.keys)
                {
                    keys.push_back(Key::fromString(key));
                    targetActions.push_back(action);
                    commandNames.push_back(cmd.name);
                    exclusiveGroups.push_back(cmd.exclusiveGroup);
                }
#endif // AUTOINPUT_HOOK_KEYBOARD_ENABLED

                for (const auto& startKey : cmd.startKeys)
                {
                    startKeys.push_back(startKey);
                }

                if (!cmd.pressWait.empty())
                {
                    if (const auto isValidWaitDelay = delayData.parseWaitTimeDelay(cmd.pressWait, true); !isValidWaitDelay)
                    {
                        return false;
                    }
                }
                if (!cmd.releaseWait.empty())
                {
                    if (const auto isValidWaitDelay = delayData.parseWaitTimeDelay(cmd.releaseWait, false); !isValidWaitDelay)
                    {
                        return false;
                    }
                }
            }

            if (!configData.endKey.empty())
            {
                endKey = configData.endKey;
            }
            if (!configData.application.empty())
            {
                applicationName = configData.application;
            }
            if (!configData.blacklist.empty())
            {
                if (!configData.appendBlacklist)
                {
                    blacklist.clear();
                }
                blacklist.insert(blacklist.end(), configData.blacklist.begin(), configData.blacklist.end());
            }
            if (!configData.statusNotificationMode.empty())
            {
                statusNotificationMode = statusNotificationModeFromString(configData.statusNotificationMode);
            }
            if (!configData.sequences.empty())
            {
                sequences.insert(sequences.end(), configData.sequences.begin(), configData.sequences.end());
            }
        }

        return true;
    }

    bool ProgramArguments::parseActionState(const gsl::span<char*> args, int& i)
    {
        const std::string_view arg = args[i];

        std::string_view actionType = safeGetNextArgument(++i, args);
        if (actionType.empty())
        {
            printUsage();
            Logger::fatal("The parameter {} needs an argument. Choices: {}\n", arg, ConfigMetadata::validActionChoices());
            return false;
        }
        actionState = actionStateFromArguments(actionType);
        if (actionState == ActionState::INVALID)
        {
            printUsage();
            Logger::fatal("Invalid parameter {} for action type. Choices: {}\n", actionType, ConfigMetadata::validActionChoices());
            return false;
        }
        return true;
    }

    bool ProgramArguments::parseButton(const gsl::span<char*> args, int& i)
    {
        const std::string_view arg = args[i];
        int32_t j = i + 1;
        for (; j < args.size(); ++j)
        {
            std::string_view button = safeGetNextArgument(j, args);
            if (buttons.empty() && button.empty())
            {
                printUsage();
                Logger::fatal("The parameter {} needs an argument. Choices: {}\n", arg, ConfigMetadata::validMouseButtonChoices());
                return false;
            }
            if (button.empty() && j >= i + 2)
            {
                // We were able to process at least one argument.
                break;
            }
            const auto mouse = Mouse::fromString(button);
            if (mouse.button != MouseButton::None)
            {
                buttons.emplace_back(mouse);
            }
            else
            {
                printUsage();
                Logger::fatal("Invalid parameter {} for button type. Choices: {}\n", button, ConfigMetadata::validMouseButtonChoices());
                return false;
            }
        }
        i = j - 1;
        return true;
    }

#if AUTOINPUT_HOOK_KEYBOARD_ENABLED
    bool ProgramArguments::parseKey(const gsl::span<char*> args, int& i)
    {
        std::string_view keyValue = safeGetNextArgument(++i, args);
        if (keyValue.empty())
        {
            printUsage();
            Logger::fatal("The parameter -k/--key needs an argument.\n");
            return false;
        }
        Key key = Key::fromString(keyValue);
        keys.emplace_back(key);
        return true;
    }
#endif // AUTOINPUT_HOOK_KEYBOARD_ENABLED

    bool ProgramArguments::parseStartKey(const gsl::span<char*> args, int& i)
    {
        const std::string_view arg = args[i];

        int32_t j = i + 1;
        for (; j < args.size(); ++j)
        {
            const std::string_view startKeyArgument = safeGetNextArgument(j, args);
            if (startKeys.empty() && startKeyArgument.empty())
            {
                printUsage();
                Logger::fatal("The parameter {} needs an argument.\n", arg);
                return false;
            }
            if (startKeyArgument.empty() && j >= i + 2)
            {
                // We were able to process at least one argument.
                break;
            }
            startKeys.emplace_back(startKeyArgument);
        }
        i = j - 1;
        return true;
    }

    bool ProgramArguments::parseEndKey(const gsl::span<char*> args, int& i)
    {
        const std::string_view arg = args[i];
        endKey = safeGetNextArgument(++i, args);
        if (endKey.empty())
        {
            printUsage();
            Logger::fatal("The parameter {} needs an argument.\n", arg);
            return false;
        }
        return true;
    }

    bool ProgramArguments::parsePressWaitTime(const gsl::span<char*> args, int& i)
    {
        return parseWaitTIme(args, i, true);
    }

    bool ProgramArguments::parseReleaseTime(const gsl::span<char*> args, int& i)
    {
        return parseWaitTIme(args, i, false);
    }

    bool ProgramArguments::parseWaitTIme(const gsl::span<char*> args, int& i, const bool isWaitPress)
    {
        const std::string_view arg = args[i];
        const std::string_view waitArgument = safeGetNextArgument(++i, args);
        if (waitArgument.empty())
        {
            return false;
        }

        if (const auto isValidWaitDelay = delayData.parseWaitTimeDelay(waitArgument, isWaitPress); !isValidWaitDelay)
        {
            Logger::error("The parameter {} needs an argument.\n", arg);
            printUsage();
            return false;
        }
        return true;
    }

    std::string_view ProgramArguments::safeGetNextArgument(const int32_t i, const gsl::span<char*> args)
    {
        if (i >= args.size() || args[i][0] == '-')
        {
            return std::string_view{};
        }

        Logger::debug("processing argument[{}]: {}\n", i, args[i]);
        return std::string_view{args[i]};
    }
}

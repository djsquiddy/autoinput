/**
 * @file arguments.cpp
 * @author djsquiddy
 * @date March 2026
 */
#include "arguments.h"
#include "logger.h"
#include "config.h"

namespace autoinput
{
    std::chrono::milliseconds WaitDelayData::getPressDelay() const
    {
        if (!usePressRange || !hasPress)
        {
            return minWaitPressDelay;
        }
        thread_local std::random_device rd; 
        thread_local std::mt19937 gen(rd()); 
        std::uniform_int_distribution<> distribution(gsl::narrow_cast<int>(minWaitPressDelay.count()),
            gsl::narrow_cast<int>(maxWaitPressDelay.count())); 
        return std::chrono::milliseconds(distribution(gen));
    }

    std::chrono::milliseconds WaitDelayData::getReleaseDelay() const
    {
        if (!useReleaseRange || !hasRelease)
        {
            return minWaitReleaseDelay;
        }
        thread_local std::random_device rd; 
        thread_local std::mt19937 gen(rd()); 
        std::uniform_int_distribution<> distribution(gsl::narrow_cast<int>(minWaitReleaseDelay.count()),
            gsl::narrow_cast<int>(maxWaitReleaseDelay.count())); 
        return std::chrono::milliseconds(distribution(gen));
    }

    std::string WaitDelayData::toString(const bool isPressWait) const
    {
        auto formatDuration = [](const std::chrono::milliseconds ms) -> std::string {
            if (ms.count() == 0) return "0ms";
            if (ms.count() % 60000 == 0) return std::to_string(ms.count() / 60000) + "m";
            if (ms.count() % 1000 == 0) return std::to_string(ms.count() / 1000) + "s";
            return std::to_string(ms.count()) + "ms";
        };

        if (isPressWait)
        {
            if (!hasPress) return "";
            if (usePressRange) return formatDuration(minWaitPressDelay) + ".." + formatDuration(maxWaitPressDelay);
            return formatDuration(minWaitPressDelay);
        }
        
        if (!hasRelease) return "";
        if (useReleaseRange) return formatDuration(minWaitReleaseDelay) + ".." + formatDuration(maxWaitReleaseDelay);
        return formatDuration(minWaitReleaseDelay);
    }

    bool WaitDelayData::parseWaitTimeDelay(std::string_view waitTimeDelayArg, const bool isPressWait)
    {
        if (waitTimeDelayArg.empty())
        {
            return false;
        }

        struct NumberIndexData
        {
            std::string_view numberData;
            size_t startIndex = 0;
            size_t endIndex = 0;
            size_t durationStartIndex = 0;
            size_t durationEndIndex = 0;

            void init()
            {
                for (size_t i = 0; i < numberData.size(); ++i)
                {
                    if (!isdigit(numberData[i]))
                    {
                        endIndex = i;
                        durationStartIndex = i;
                        break;
                    }
                }

                if (endIndex == 0)
                {
                    // We don't have a duration type
                    endIndex = numberData.size();
                }
                else
                {
                    durationEndIndex = numberData.size();
                }

                Logger::debug("Wait Number data:\n");
                Logger::debug("Number   Start Index: {} End Index: {}\n", startIndex, endIndex);
                Logger::debug("Number              : {}\n", getNumber());
                Logger::debug("Duration Start Index: {} End Index: {}\n", durationStartIndex, durationEndIndex);
                Logger::debug("Duration            : {}\n", getMilliseconds());
            }

            [[nodiscard]] int32_t getNumber() const
            {
                return parseStringToInt(numberData.substr(startIndex, endIndex - startIndex));
            }

            [[nodiscard]] std::chrono::milliseconds getMilliseconds() const
            {
                const auto number = getNumber();
                if (durationStartIndex == 0)
                {
                    return std::chrono::milliseconds(number);
                }

                const auto durationType = numberData.substr(durationStartIndex, endIndex - durationEndIndex);
                if (durationType == "s")
                {
                    const std::chrono::seconds seconds{number};
                    return {seconds};
                }
                if (durationType == "m")
                {
                    const std::chrono::minutes minutes{number};
                    return {minutes};
                }
                if (durationType == "ms")
                {
                    return std::chrono::milliseconds(number);
                }

                Logger::error("Unrecognized duration type: {}\n", durationType);
                Logger::error("Recognized options: [s|m|ms]\n");
                return std::chrono::milliseconds(number);
            }
        };

        auto getNumberIndexData = [](const std::string_view numberData) -> NumberIndexData
        {
            NumberIndexData data{.numberData = numberData};
            data.init();
            return data;
        };

        if (waitTimeDelayArg.contains(".."))
        {
            const auto index = waitTimeDelayArg.find("..");
            const auto minWait = waitTimeDelayArg.substr(0, index);
            const auto maxWait = waitTimeDelayArg.substr(index + 2);
            const auto minWaitNumber = getNumberIndexData(minWait);
            const auto maxWaitNumber = getNumberIndexData(maxWait);
            if (isPressWait)
            {
                hasPress = usePressRange = true;
                minWaitPressDelay = minWaitNumber.getMilliseconds();
                maxWaitPressDelay = maxWaitNumber.getMilliseconds();
            }
            else
            {
                hasRelease = useReleaseRange = true;
                minWaitReleaseDelay = minWaitNumber.getMilliseconds();
                maxWaitReleaseDelay = maxWaitNumber.getMilliseconds();
            }
            return true;
        }

        const auto waitNumber = getNumberIndexData(waitTimeDelayArg);
        if (isPressWait)
        {
            hasPress = true;
            usePressRange = false;
            minWaitPressDelay = maxWaitPressDelay = waitNumber.getMilliseconds();
        }
        else
        {
            hasRelease = true;
            useReleaseRange = false;
            minWaitReleaseDelay = maxWaitReleaseDelay = waitNumber.getMilliseconds();
        }
        return true;
    }

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

        for (int i = 1; i < args.size(); ++i)
        {
            const std::string_view arg = args[i];
            if (arg == "-h" || arg == "--help")
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
                    Logger::fatal("The parameter {} needs an argument. Choices: {{d,debug,i,info,w,warn,warning,e,error,f,fatal}}\n", arg);
                    return false;
                }
                const LogLevel logLevel = logLevelFromString(logLevelStr);
                if (logLevel == LogLevel::Unknown)
                {
                    printUsage();
                    Logger::fatal("Invalid parameter {} for log level. Choices: {{d,debug,i,info,w,warn,warning,e,error,f,fatal}}\n", logLevelStr);
                    return false;
                }
                Logger::setLogLevel(logLevel);
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
        }

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
            else if (arg.starts_with('-'))
            {
                Logger::warn("Unknown argument: {}\n", arg);
            }
            else
            {
                // Positional argument
                const auto state = actionStateFromArguments(arg);
                if (state != ActionState::INVALID)
                {
                    actionState = state;
                    continue;
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
                            if (const auto nextButton = mouseButtonFromArguments(nextArg); nextButton == MouseButton::BACK || nextButton == MouseButton::FORWARD)
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
                if (mouse.button != MouseButton::NONE)
                {
                    if (mouse.button == MouseButton::BACK || mouse.button == MouseButton::FORWARD)
                    {
                        if ((buttons.size() + keys.size()) == startKeys.size())
                        {
                            // Trigger for default target
                            buttons.emplace_back(MouseButton::LEFT);
                            targetActions.emplace_back(actionState != ActionState::INVALID ? actionState : ActionState::CLICK);
                            startKeys.emplace_back(arg);
                            continue;
                        }
                    }
                    buttons.emplace_back(mouse);
                    registerTarget();
                    continue;
                }

#if AUTOINPUT_HOOK_KEYBOARD_ENABLED
                if (Key key = Key::fromString(arg); !key.character.empty() || key.modifier != KeyModifier::None)
                {
                    if (key.modifier == KeyModifier::Function)
                    {
                        if ((buttons.size() + keys.size()) == startKeys.size())
                        {
                            // Trigger for default target
                            buttons.emplace_back(MouseButton::LEFT);
                            targetActions.emplace_back(actionState != ActionState::INVALID ? actionState : ActionState::CLICK);
                            startKeys.emplace_back(arg);
                            continue;
                        }
                    }
                    keys.emplace_back(key);
                    registerTarget();
                    continue;
                }
#endif
                Logger::warn("Unrecognized positional argument: {}\n", arg);
            }
        }

        return postParseArguments();
    }

    bool ProgramArguments::postParseArguments()
    {
        const auto& [start, end, press, release, action, button, settingsBlacklist] = m_settings.getDefaults();

        if (!settingsBlacklist.empty())
        {
            blacklist.insert(blacklist.end(), settingsBlacklist.begin(), settingsBlacklist.end());
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
                if (const auto mouse = Mouse::fromString(button); mouse.button != MouseButton::NONE)
                {
                    buttons.emplace_back(mouse);
                }
            }

            if (buttons.empty())
            {
                buttons.emplace_back(MouseButton::LEFT);
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
            startKeys.emplace_back(!start.empty() ? start : "f2");
        }
        if (endKey.empty())
        {
            endKey = !end.empty() ? end : "f3";
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

        return true;
    }

    void ProgramArguments::printUsage(const bool verbose) const
    {
        Logger::print("usage {} [-h] [{{click,hold}}] [{{left,right,middle,key}} ...] [-s START_KEYS [START_KEYS ...]] [-e END_KEY] [-w WAIT_TIME] [-S SAVE_CONFIG_NAME]\n\n", programName);
        Logger::print("options\n");
        const auto optionPrefix = std::string(4, ' ');
        const auto optionUsagePrefix = std::string(10, ' ');

        Logger::print("{} -h, --help\n", optionPrefix);
        Logger::print("{} show this help message with examples and exits.\n", optionUsagePrefix);
        Logger::print("{} -l, --log [{{d,debug,i,info,w,warn,warning,e,error,f,fatal}}]\n", optionPrefix);
        Logger::print("{} set the log level. (Choices: debug, info, warning, warn, error, fatal)\n", optionUsagePrefix);
        Logger::print("{} -c --config\n", optionPrefix);
        Logger::print("{} Use the specified configuration found under {}. Extension can be omitted\n", optionUsagePrefix, getConfigsPath().string());
        Logger::print("{} -t, --type {{click,c,hold,h}}\n", optionPrefix);
        Logger::print("{} What kind of action event to use. (Can be positional)\n", optionUsagePrefix);
#if AUTOINPUT_HOOK_MOUSE_ENABLED
        Logger::print("{} -b, --btn, --button {{button}} [{{button}} ...]\n", optionPrefix);
        Logger::print("{} Which button to press. (Default: left) (Can be positional). Modifiers like shift+left are supported.\n", optionUsagePrefix);
#endif // AUTOINPUT_HOOK_MOUSE_ENABLED
#if AUTOINPUT_HOOK_KEYBOARD_ENABLED
        Logger::print("{} -k, --key {{key}} [{{key}} ...]\n", optionPrefix);
        Logger::print("{} Key that is to be pressed/simulated. (Can be positional)\n", optionUsagePrefix);
#endif
        Logger::print("{} -s, --start-key START_KEYS [START_KEYS ...]\n", optionPrefix);
        Logger::print("{} Key that is used to start the autoclicker. If button presses need separate start/stop binding the order matters here.\n", optionUsagePrefix);
        Logger::print("{} -e, --end-key END_KEY\n", optionPrefix);
        Logger::print("{} Key that is used to end the autoclicker.\n", optionUsagePrefix);
        Logger::print("{} -a, --app, --application APPLICATION_NAME\n", optionPrefix);
        Logger::print("{} Only listen for inputs when this application is in focus.\n", optionUsagePrefix);
        Logger::print("{} -B, --blacklist APPLICATION_NAME\n", optionPrefix);
        Logger::print("{} Do not run when this application is in focus.\n", optionUsagePrefix);
        Logger::print("{} -L, --list-apps\n", optionPrefix);
        Logger::print("{} List all currently running application names and exit.\n", optionUsagePrefix);
        Logger::print("{} -C, --list-configs\n", optionPrefix);
        Logger::print("{} List all valid configuration names and exit.\n", optionUsagePrefix);
        Logger::print("{} -S, --save-config SAVE_CONFIG_NAME\n", optionPrefix);
        Logger::print("{} Save the current configuration into a named configuration in the user configurations and exit.\n", optionUsagePrefix);
        Logger::print("{} -w, --wait WAIT_TIME\n", optionPrefix);
        Logger::print("{} Max duration to wait before auto clicking. {}\n", optionUsagePrefix, bold("Type must be set to click"));
        Logger::print("{} Can use a time range by using {{min}}..{{max}} with each click being random between the range. See examples for usage.\n", optionUsagePrefix);

        Logger::print("\n\n");
        if (verbose)
        {
            const auto examplePrefix = std::string(2, ' ');
            const auto exampleCmdPrefix = std::string(4, ' ') + "> ";
            const auto exampleNotePrefix = std::string(10, ' ') + "Note: ";
            int32_t index = 1;
            Logger::print("examples:\n");
            Logger::print("{}{}) hold the left click when pressing F2 and stopping on F3 (Defaults):\n", examplePrefix, index);
            Logger::print("{}{} hold left\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) hold the left click when pressing F2, or hold the right click when pressing F3 and stop on F4:\n", examplePrefix, index);
            Logger::print("{}{} hold left f2 right f3 -e f4\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) click the left button when pressing F2 and hold the left button when pressing F4:\n", examplePrefix, index);
            Logger::print("{}{} click f2 hold f4\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) hold the left button when pressing the mouse BACK button:\n", examplePrefix, index);
            Logger::print("{}{} hold left back\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) hold the left and right click when pressing F2 and stop on F3:\n", examplePrefix, index);
            Logger::print("{}{} hold left right f2\n", exampleCmdPrefix, programName);
            Logger::print("{} This only works if start key only has {} value.\n", exampleNotePrefix, bold("ONE"));
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) auto left click every 2 seconds:\n", examplePrefix, index);
            Logger::print("{}{} left -w 2s\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) auto left click every 1 to 2 seconds:\n", examplePrefix, index);
            Logger::print("{}{} left --press-wait 1s..2s\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) auto left click and hold for 500ms to 1s then wait 1s to 2s after to click again:\n", examplePrefix, index);
            Logger::print("{}{} left --press-wait 500ms..1s --release-wait 1s..2s\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) simulate pressing the 'a' key when F2 is pressed:\n", examplePrefix, index);
            Logger::print("{}{} a f2\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) hold the space bar when pressing F2 and stop on F3:\n", examplePrefix, index);
            Logger::print("{}{} space f2 -e f3\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) click the 'ctrl+v' key combination when pressing F2:\n", examplePrefix, index);
            Logger::print("{}{} click ctrl+v f2\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) click the 'shift+left' mouse button when pressing F2:\n", examplePrefix, index);
            Logger::print("{}{} click shift+left f2\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) use a configuration file named 'gaming':\n", examplePrefix, index);
            Logger::print("{}{} --config gaming\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) run with debug logging enabled:\n", examplePrefix, index);
            Logger::print("{}{} -l debug hold left\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) hold the left click but stop running if notepad is in focus:\n", examplePrefix, index);
            Logger::print("{}{} hold left --blacklist notepad.exe\n", exampleCmdPrefix, programName);
        }

        Logger::flush();
    }

    ConfigData ProgramArguments::toConfigData() const
    {
        ConfigData data;
        data.endKey = endKey;
        data.application = applicationName;
        data.blacklist = blacklist;
        data.appendBlacklist = true; // CLI arguments always append or we don't have enough info to know if they intended to replace.
        // Actually, we don't store appendBlacklist in ProgramArguments itself.
        // For now, let's just keep the default true.

        const size_t buttonCount = buttons.size();
        const size_t keyCount = keys.size();
        const size_t targetCount = buttonCount + keyCount;
        const size_t actionCount = targetActions.size();
        const size_t startKeyCount = startKeys.size();

        for (size_t i = 0; i < targetCount; ++i)
        {
            CommandData cmd;
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
            Logger::error("Configuration could not be found at: {}", configPath.string());
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
                    if (const auto mouse = Mouse::fromString(button); mouse.button != MouseButton::NONE)
                    {
                        buttons.push_back(mouse);
                        targetActions.push_back(action);
                    }
                    else
                    {
                        Logger::fatal("Invalid parameter {} for button type. Choices: {{left,right,middle,back,forward}}\n", button);
                        printUsage();
                        return false;
                    }
                }

#if AUTOINPUT_HOOK_KEYBOARD_ENABLED
                for (const auto& key : cmd.keys)
                {
                    keys.push_back(Key::fromString(key));
                    targetActions.push_back(action);
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
            Logger::fatal("The parameter {} needs an argument. Choices: {{click,c,hold,h}}\n", arg);
            return false;
        }
        actionState = actionStateFromArguments(actionType);
        if (actionState == ActionState::INVALID)
        {
            printUsage();
            Logger::fatal("Invalid parameter {} for action type. Choices: {{click,c,hold,h}}\n", actionType);
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
                Logger::fatal("The parameter {} needs an argument. Choices: {{left,l,right,r,middle,m,back,forward}}\n", arg);
                return false;
            }
            if (button.empty() && j >= i + 2)
            {
                // We were able to process at least one argument.
                break;
            }
            const auto mouse = Mouse::fromString(button);
            if (mouse.button != MouseButton::NONE)
            {
                buttons.emplace_back(mouse);
            }
            else
            {
                printUsage();
                Logger::fatal("Invalid parameter {} for button type. Choices: {{left,l,right,r,middle,m,back,forward}}\n", button);
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

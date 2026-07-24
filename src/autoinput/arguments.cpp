//
// Created by djsqu on 3/9/2026.
//

#include "arguments.h"

#include <algorithm>

#include "logger.h"
#include "config.h"

#include <iostream>
#include <random>
#include <ranges>

namespace autoinput
{
    std::chrono::milliseconds WaitDelayData::getPressDelay() const
    {
        if (!usePressRange || !hasPress)
        {
            return minWaitPressDelay;
        }
        static std::random_device rd; // obtain a random number from hardware
        static std::mt19937 gen(rd()); // seed the generator
        std::uniform_int_distribution<> distr(minWaitPressDelay.count(), maxWaitPressDelay.count()); // define the range
        return std::chrono::milliseconds(distr(gen));
    }

    std::chrono::milliseconds WaitDelayData::getReleaseDelay() const
    {
        if (!useReleaseRange || !hasRelease)
        {
            return minWaitReleaseDelay;
        }
        static std::random_device rd; // obtain a random number from hardware
        static std::mt19937 gen(rd()); // seed the generator
        std::uniform_int_distribution<> distr(minWaitReleaseDelay.count(), maxWaitReleaseDelay.count()); // define the range
        return std::chrono::milliseconds(distr(gen));
    }

    bool WaitDelayData::parseWaitTimeDelay(std::string_view waitTimeDelayArg, bool isPressWait)
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
            minWaitPressDelay = maxWaitPressDelay = waitNumber.getMilliseconds();
        }
        else
        {
            hasRelease = true;
            minWaitReleaseDelay = maxWaitReleaseDelay = waitNumber.getMilliseconds();
        }
        return true;
    }

    ProgramArguments::ProgramArguments()
        : NonCopyable()
    {
    }

    bool ProgramArguments::parseArguments(int argc, char** argv)
    {
        if (argc >= 1)
        {
            programName = argv[0];
        }
        if (argc <= 1)
        {
            // We only get the program name listed.
            printUsage();
            return false;
        }

        for (int i = 1; i < argc; ++i)
        {
            const std::string_view arg = argv[i];
            if (arg == "-h" || arg == "--help")
            {
                printUsage(true);
                return false;
            }
            if (arg == "-l" || arg == "--log")
            {
                const std::string_view logLevelStr = safeGetNextArgument(++i, argc, argv);
                const LogLevel logLevel = logLevelFromString(logLevelStr);
                Logger::setLogLevel(logLevel);
                continue;
            }
            // We want to parse the configuration first so we can use cli arguments as overrides if need/wanted.
            if (arg == "-c" || arg == "--config")
            {
                if (!parseConfigArguments(argc, argv, i))
                {
                    return false;
                }
            }
        }

        for (int i = 1; i < argc; ++i)
        {
            const std::string_view arg = argv[i];

            Logger::debug("processing argument[{}]: {}\n", i, arg);

#if AUTOINPUT_HOOK_KEYBOARD_ENABLED
            if (arg == "-k" || arg == "--key")
            {
                if (!parseKey(argc, argv, i))
                {
                    return false;
                }
                continue;
            }
#endif // AUTOINPUT_HOOK_KEYBOARD_ENABLED
            if (arg == "-t" || arg == "--type")
            {
                if (!parseActionState(argc, argv, i))
                {
                    return false;
                }
            }
            else if (arg == "-b" || arg == "--button" || arg == "--btn")
            {
                if (!parseButton(argc, argv, i))
                {
                    return false;
                }
            }
            else if (arg == "-s" || arg == "--start")
            {
                if (!parseStartKey(argc, argv, i))
                {
                    return false;
                }
            }
            else if (arg == "-e" || arg == "--end")
            {
                if (!parseEndKey(argc, argv, i))
                {
                    return false;
                }
            }
            else if (arg == "-w" || arg == "--wait" || arg=="--press-wait" || arg=="--release-wait")
            {
                if (arg.contains("press"))
                {
                    if (!parsePressWaitTime(argc, argv, i))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!parseReleaseTime(argc, argv, i))
                    {
                        return false;
                    }
                }
            }
        }

        return postParseArguments();
    }

    bool ProgramArguments::postParseArguments()
    {
        if (buttons.empty())
        {
            buttons.emplace_back(MouseButton::LEFT);
        }
        if (startKeys.empty())
        {
            printUsage();
            Logger::error("Start key is missing...\nAssign it using -s or --start\n");
            return false;
        }
        if (endKey.empty())
        {
            printUsage();
            Logger::error("End key is missing...\nAssign it using -e or --end");
            return false;
        }

        if (buttons.size() != startKeys.size())
        {
            startKeys.resize(buttons.size());
        }

        return true;
    }

    void ProgramArguments::printUsage(const bool verbose) const
    {
        Logger::print("usage {} [-h] [-t {{click,c,hold,h}}] [-b {{left,l,right,r,middle,m}} [{{left,l,right,r,middle,m}} ...]] [-s START_KEYS [START_KEYS ...]] [-e END_KEY] [-w WAIT_TIME]\n\n", programName);
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
        Logger::print("{} What kind of action event to use.\n", optionUsagePrefix);
        Logger::print("{} -b, --btn, --button {{left,l,right,r,middle,m}} [{{left,l,right,r,middle,m}} ...]\n", optionPrefix);
        Logger::print("{} Which button to press. (Default: left)\n", optionUsagePrefix);
#if AUTOINPUT_HOOK_KEYBOARD_ENABLED
        Logger::print("{} -k, --key {{key}} [{{key}} ...]\n", optionPrefix);
        Logger::print("{} Key that is used to start the autoclicker. If button presses need separate start/stop binding the order matters here.\n", optionUsagePrefix);
#endif
        Logger::print("{} -s, --start-key START_KEYS [START_KEYS ...]\n", optionPrefix);
        Logger::print("{} Key that is used to start the autoclicker. If button presses need separate start/stop binding the order matters here.\n", optionUsagePrefix);
        Logger::print("{} -e, --end-key END_KEY\n", optionPrefix);
        Logger::print("{} Key that is used to end the autoclicker.\n", optionUsagePrefix);
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
            Logger::print("{}{}) hold the left click when pressing F2 and stopping on F3:\n", examplePrefix, index);
            Logger::print("{}{} -t hold -b left -s f2 -e f3\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) hold the left click when pressing F2, or hold the right click when pressing F3 and stop on F4:\n", examplePrefix, index);
            Logger::print("{}{} -t hold -b left right -s f2 f3 -e f4\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) hold the left and right click when pressing F2 and stop on F3:\n", examplePrefix, index);
            Logger::print("{}{} -t hold -b left right -s f2 -e f3\n", exampleCmdPrefix, programName);
            Logger::print("{} This only works if start key only has {} value.\n", exampleNotePrefix, bold("ONE"));
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) auto left click every 2 seconds:\n", examplePrefix, index);
            Logger::print("{}{} -t click -s f2 -e f3 -w 2\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) auto left click every 1 to 2 seconds:\n", examplePrefix, index);
            Logger::print("{}{} -t click -s f2 -e f3 --press-wait 1s..2s\n", exampleCmdPrefix, programName);
            ++index;
            Logger::print("\n");
            Logger::print("{}{}) auto left click and hold for 500ms to 1s then wait 1s to 2s after to click again:\n", examplePrefix, index);
            Logger::print("{}{} -t click -s f2 -e f3 --press-wait 500ms..1s --release-wait 1s..2s\n", exampleCmdPrefix, programName);
        }

        Logger::flush();
    }

    bool ProgramArguments::parseConfigArguments(int argc, char** argv, int& i)
    {
        // TODO: implement
        std::string_view fileName = safeGetNextArgument(++i, argc, argv);
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
            if (!configData.action.empty())
            {
                actionState = actionStateFromArguments(configData.action);
                if (actionState == ActionState::INVALID)
                {
                    Logger::fatal("Invalid parameter {} for action type. Choices: {{click,hold}}\n", configData.action);
                    return false;
                }
            }
            if (!configData.buttons.empty())
            {
                for (auto& button : configData.buttons)
                {
                    if (const auto mouseButton = mouseButtonFromArguments(button); mouseButton != MouseButton::NONE)
                    {
                        buttons.emplace_back(mouseButton);
                    }
                    else
                    {
                        Logger::fatal("Invalid parameter {} for button type. Choices: {{left,right,middle}}\n", button);
                        printUsage();
                        return false;
                    }
                }
            }
#if AUTOINPUT_HOOK_KEYBOARD_ENABLED
            if (!configData.keys.empty())
            {
                for (auto& key : configData.keys)
                {
                    keys.emplace_back(Key::fromString(key));
                }
            }
#endif // AUTOINPUT_HOOK_KEYBOARD_ENABLED
            if (!configData.startKeys.empty())
            {
                for (auto& key : configData.startKeys)
                {
                    startKeys.emplace_back(key);
                }
            }
            if (!configData.endKey.empty())
            {
                endKey = configData.endKey;
            }
            if (!configData.pressWait.empty())
            {
                if (const auto isValidWaitDelay = delayData.parseWaitTimeDelay(configData.pressWait, true); !isValidWaitDelay)
                {
                    return false;
                }
            }
            if (!configData.releaseWait.empty())
            {
                if (const auto isValidWaitDelay = delayData.parseWaitTimeDelay(configData.releaseWait, false); !isValidWaitDelay)
                {
                    return false;
                }
            }
        }

        return true;
    }

    bool ProgramArguments::parseActionState(const int argc, char** argv, int& i)
    {
        const std::string_view arg = argv[i];

        std::string_view actionType = safeGetNextArgument(++i, argc, argv);
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

    bool ProgramArguments::parseButton(const int argc, char** argv, int& i)
    {
        const std::string_view arg = argv[i];
        int32_t j = i + 1;
        for (; j < argc; ++j)
        {
            std::string_view button = safeGetNextArgument(j, argc, argv);
            if (buttons.empty() && button.empty())
            {
                printUsage();
                Logger::fatal("The parameter {} needs an argument. Choices: {{left,l,right,r,middle,m}}\n", arg);
                return false;
            }
            if (button.empty() && j >= i + 2)
            {
                // We were able to process at least one argument.
                break;
            }
            if (const auto mouseButton = mouseButtonFromArguments(button); mouseButton != MouseButton::NONE)
            {
                buttons.emplace_back(mouseButton);
            }
            else
            {
                printUsage();
                Logger::fatal("Invalid parameter {} for button type. Choices: {{left,l,right,r,middle,m}}\n", button);
                return false;
            }
        }
        i = j - 1;
        return true;
    }

#if AUTOINPUT_HOOK_KEYBOARD_ENABLED
    bool ProgramArguments::parseKey(const int argc, char** argv, int& i)
    {
        std::string_view keyValue = safeGetNextArgument(++i, argc, argv);
        Key key = Key::fromString(keyValue);
        keys.emplace_back(key);
        return true;
    }
#endif // AUTOINPUT_HOOK_KEYBOARD_ENABLED

    bool ProgramArguments::parseStartKey(const int argc, char** argv, int& i)
    {
        const std::string_view arg = argv[i];

        int32_t j = i + 1;
        for (; j < argc; ++j)
        {
            const std::string_view startKeyArgument = safeGetNextArgument(j, argc, argv);
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

    bool ProgramArguments::parseEndKey(const int argc, char** argv, int& i)
    {
        endKey = safeGetNextArgument(++i, argc, argv);
        return true;
    }

    bool ProgramArguments::parsePressWaitTime(const int argc, char** argv, int& i)
    {
        return parseWaitTIme(argc, argv, i, true);
    }

    bool ProgramArguments::parseReleaseTime(const int argc, char** argv, int& i)
    {
        return parseWaitTIme(argc, argv, i, false);
    }

    bool ProgramArguments::parseWaitTIme(const int argc, char** argv, int& i, const bool isWaitPress)
    {
        const std::string_view arg = argv[i];
        const std::string_view waitArgument = safeGetNextArgument(++i, argc, argv);
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

    std::string_view ProgramArguments::safeGetNextArgument(const int32_t i, const int32_t argc, char** argv)
    {
        if (i >= argc || argv[i][0] == '-')
        {
            return std::string_view{};
        }

        std::cout << "processing argument[" << i << "]: " << argv[i] << "\n";
        return std::string_view{argv[i]};
    }
}

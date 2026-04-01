//
// Created by djsqu on 3/9/2026.
//

#include "arguments.h"

#include <iostream>
#include <random>

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
            int32_t startIndex = 0;
            int32_t endIndex = 0;
            int32_t durationStartIndex = 0;
            int32_t durationEndIndex = 0;

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

                std::cout << "Wait Number data:\n";
                std::cout << "Number   Start Index: " << startIndex << " End Index: " << endIndex << "\n";
                std::cout << "Number              : " << getNumber() << "\n";
                std::cout << "Duration Start Index: " << durationEndIndex << " End Index: " << durationEndIndex << "\n";
                std::cout << "Duration            : " << getMilliseconds() << "\n";
            }

            int32_t getNumber() const
            {
                return parseStringToInt(numberData.substr(startIndex, endIndex - startIndex));
            }

            std::chrono::milliseconds getMilliseconds() const
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
                    return std::chrono::milliseconds(seconds);
                }
                if (durationType == "m")
                {
                    const std::chrono::minutes minutes{number};
                    return std::chrono::milliseconds(minutes);
                }
                if (durationType == "ms")
                {
                    return std::chrono::milliseconds(number);
                }

                std::cerr << "Unrecognized duration type: " << durationType << "\n";
                std::cerr << "Recognized options: [s|m|ms]" << std::endl;
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

            std::cout << "processing argument[" << i << "]: " << arg << "\n";

            if (arg == "-h" || arg == "--help")
            {
                printUsage(true);
                return false;
            }

            if (arg == "-t" || arg == "--type")
            {
                std::string_view buttonType = safeGetNextArgument(++i, argc, argv);
                if (buttonType.empty())
                {
                    std::cerr << "The parameter " << arg << " needs an argument. Choices: {click,c,hold,h}\n" << std::endl;
                    printUsage();
                    return false;
                }
                buttonState = buttonStateFromArguments(buttonType);
                if (buttonState == ButtonState::INVALID)
                {
                    std::cerr << "Invalid parameter " << buttonType <<  " for button type. Choices: {click,c,hold,h}\n" << std::endl;
                    printUsage();
                    return false;
                }
            }
            else if (arg == "-b" || arg == "--button" || arg == "--btn")
            {
                int32_t j = i + 1;
                for (; j < argc; ++j)
                {
                    std::string_view button = safeGetNextArgument(j, argc, argv);
                    if (buttons.empty() && button.empty())
                    {
                        std::cerr << "The parameter " << arg << " needs an argument. Choices: {left,l,right,r,middle,m}\n" << std::endl;
                        printUsage();
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
                        std::cerr << "Invalid parameter " << button <<  " for button type. Choices: {left,l,right,r,middle,m}\n" << std::endl;
                        printUsage();
                        return false;
                    }
                }
                i = j - 1;
            }
            else if (arg == "-s" || arg == "--start")
            {
                int32_t j = i + 1;
                for (; j < argc; ++j)
                {
                    const std::string_view startKeyArgument = safeGetNextArgument(j, argc, argv);
                    if (startKeys.empty() && startKeyArgument.empty())
                    {
                        std::cerr << "The parameter " << arg << " needs an argument.\n" << std::endl;
                        printUsage();
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
            }
            else if (arg == "-e" || arg == "--end")
            {
                endKey = safeGetNextArgument(++i, argc, argv);
            }
            else if (arg == "-w" || arg == "--wait" || arg=="--press-wait" || arg=="--release-wait")
            {
                bool isPressWait = arg.contains("press");
                const std::string_view waitArgument = safeGetNextArgument(++i, argc, argv);
                if (waitArgument.empty())
                {
                    return false;
                }

                const auto isValidWaitDelay = delayData.parseWaitTimeDelay(waitArgument, isPressWait);
                if (!isValidWaitDelay)
                {
                    std::cerr << "The parameter " << arg << " needs an argument.\n" << std::endl;
                    printUsage();
                    return false;
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
            std::cerr << "Start key is missing...\nAssign it using -s or --start" << std::endl;
            printUsage();
            return false;
        }
        if (endKey.empty())
        {
            std::cerr << "End key is missing...\nAssign it using -e or --end" << std::endl;
            printUsage();
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
        std::cout << "usage: " << programName << " [-h] [-t {click,c,hold,h}] [-b {left,l,right,r,middle,m} [{left,l,right,r,middle,m} ...]] [-s START_KEYS [START_KEYS ...]] [-e END_KEY] [-w WAIT_TIME]";
        std::cout << "\n\n";
        std::cout << "options:\n";

        const auto optionPrefix = std::string(4, ' ');
        const auto optionUsagePrefix = std::string(10, ' ');

        std::cout << optionPrefix << "-h, --help" << "\n";;
        std::cout << optionUsagePrefix << "show this help message with examples and exits." << "\n";
        std::cout << optionPrefix << "-t, --type {click,c,hold,h}" << "\n";;
        std::cout << optionUsagePrefix << "What kind of mouse event to use." << "\n";
        std::cout << optionPrefix << "-b, --btn, --button {left,l,right,r,middle,m} [{left,l,right,r,middle,m} ...]" << "\n";;
        std::cout << optionUsagePrefix << "Which button to press. (Default: left)" << "\n";
        std::cout << optionPrefix << "-s, --start-key START_KEYS [START_KEYS ...]" << "\n";;
        std::cout << optionUsagePrefix << "Key that is used to start the autoclicker. If button presses need separate start/stop binding the order matters here." << "\n";
        std::cout << optionPrefix << "-e, --end-key END_KEY" << "\n";;
        std::cout << optionUsagePrefix << "Key that is used to end the autoclicker." << "\n";
        std::cout << optionPrefix << "-w, --wait WAIT_TIME" << "\n";;
        std::cout << optionUsagePrefix << "Max duration to wait before auto clicking. " << bold("Type must be set to click") << "\n";
        std::cout << optionUsagePrefix << "Can use a time range by using {min}..{max} with each click being random between the range. See examples for usage.\n";

        std::cout << "\n\n";
        if (verbose)
        {
            const auto examplePrefix = std::string(2, ' ');
            const auto exampleCmdPrefix = std::string(4, ' ') + "> ";
            const auto exampleNotePrefix = std::string(10, ' ') + "Note: ";
            int32_t index = 1;
            std::cout << "examples:\n";
            std::cout << examplePrefix << index << ") hold the left click when pressing F2 and stop on F3:\n";
            std::cout << exampleCmdPrefix << programName << " -t hold -b left -s f2 -e f3" << "\n";
            ++index;
            std::cout << "\n";
            std::cout << examplePrefix << index << ") hold the left click when pressing F2, or hold the right click when pressing F3 and stop on F4:\n";
            std::cout << exampleCmdPrefix << programName << " -t hold -b left right -s f2 f3 -e f4" << "\n";
            ++index;
            std::cout << "\n";
            std::cout << examplePrefix << index << ") hold the left and right click when pressing F2 and stop on F3:\n";
            std::cout << exampleCmdPrefix << programName << " -t hold -b left right -s f2 -e f3" << "\n";
            std::cout << exampleNotePrefix << "This only works if start key only has " << bold("ONE") <<" value." << "\n";
            ++index;
            std::cout << "\n";
            std::cout << examplePrefix << index << ") auto left click every 2 seconds:\n";
            std::cout << exampleCmdPrefix << programName << " -t click -s f2 -e f3 -w 2" << "\n";
            ++index;
            std::cout << "\n";
            std::cout << examplePrefix << index << ") auto left click every 1 to 2 seconds:\n";
            std::cout << exampleCmdPrefix << programName << " -t click -s f2 -e f3 --press-wait 1s..2s" << "\n";
            ++index;
            std::cout << "\n";
            std::cout << examplePrefix << index << ") auto left click and hold for 500ms to 1s then wait 1s to 2s after to click again:\n";
            std::cout << exampleCmdPrefix << programName << " -t click -s f2 -e f3 --press-wait 500ms..1s --release-wait 1s..2s" << "\n";
        }
        std::cout << std::endl;
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

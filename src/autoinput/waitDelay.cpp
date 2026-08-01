/**
 * @file waitDelay.cpp
 * @author djsquiddy
 * @date August 2026
 */

#include "autoinput/waitDelay.h"
#include "autoinput/defaults.h"
#include "autoinput/logger.h"
#include "autoinput/types.h"
#include <random>
#include <cctype>
#include <algorithm>
#include <gsl/gsl>

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

                const auto durationType = numberData.substr(durationStartIndex, durationEndIndex - durationStartIndex);
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

    bool isValidWaitDelay(std::string_view wait)
    {
        if (wait.empty()) return false;

        auto checkPart = [](std::string_view part) {
            if (part.empty()) return false;
            size_t i = 0;
            while (i < part.size() && std::isdigit(static_cast<unsigned char>(part[i]))) i++;
            if (i == 0) return false; // No digits

            std::string_view unit = part.substr(i);
            return unit.empty() || unit == "ms" || unit == "s" || unit == "m";
        };

        if (wait.contains(".."))
        {
            size_t pos = wait.find("..");
            return checkPart(wait.substr(0, pos)) && checkPart(wait.substr(pos + 2));
        }

        return checkPart(wait);
    }
}

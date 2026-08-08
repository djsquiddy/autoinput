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
#include <charconv>
#include <format>
#include <optional>

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

    bool WaitDelayData::parseWaitTimeDelay(const std::string_view waitTimeDelayArg, const bool isPressWait)
    {
        const auto parsed = parseWaitDelayInput(waitTimeDelayArg);
        if (!parsed)
        {
            return false;
        }

        if (isPressWait)
        {
            hasPress = parsed->hasValue;
            usePressRange = parsed->useRange;
            minWaitPressDelay = waitDelayInputToMilliseconds(parsed->minValue, parsed->durationType);
            maxWaitPressDelay = waitDelayInputToMilliseconds(parsed->maxValue, parsed->durationType);
        }
        else
        {
            hasRelease = parsed->hasValue;
            useReleaseRange = parsed->useRange;
            minWaitReleaseDelay = waitDelayInputToMilliseconds(parsed->minValue, parsed->durationType);
            maxWaitReleaseDelay = waitDelayInputToMilliseconds(parsed->maxValue, parsed->durationType);
        }

        return true;
    }

    bool isValidWaitDelay(const std::string_view wait)
    {
        return parseWaitDelayInput(wait).has_value();
    }

    std::chrono::milliseconds parseWaitDelay(const std::string_view delayStr)
    {
        WaitDelayData data;
        if (data.parseWaitTimeDelay(delayStr, true))
        {
            return data.getPressDelay();
        }
        return std::chrono::milliseconds(0);
    }

    std::optional<WaitDelayInput> parseWaitDelayInput(const std::string_view value)
    {
        if (value.empty())
        {
            return std::nullopt;
        }

        auto trim = [](std::string_view sv) -> std::string_view {
            while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.front()))) sv.remove_prefix(1);
            while (!sv.empty() && std::isspace(static_cast<unsigned char>(sv.back()))) sv.remove_suffix(1);
            return sv;
        };

        auto parsePart = [trim](std::string_view part, double& outValue, std::string& outUnit) -> bool {
            part = trim(part);
            if (part.empty()) return false;

            size_t i = 0;
            bool hasDecimal = false;
            while (i < part.size() && (std::isdigit(static_cast<unsigned char>(part[i])) || (part[i] == '.' && !hasDecimal)))
            {
                if (part[i] == '.') hasDecimal = true;
                i++;
            }

            if (i == 0) return false;

            const std::string numStr(part.substr(0, i));
            const std::string_view unitPart = part.substr(i);

            try
            {
                outValue = std::stod(numStr);
            }
            catch (...)
            {
                return false;
            }

            if (!unitPart.empty())
            {
                outUnit = std::string(unitPart);
                if (outUnit != "ms" && outUnit != "s" && outUnit != "m")
                {
                    return false;
                }
            }
            else
            {
                outUnit = "ms";
            }

            return true;
        };

        WaitDelayInput result;
        result.hasValue = true;

        if (value.contains(".."))
        {
            result.useRange = true;
            const size_t pos = value.find("..");
            const std::string_view minPart = value.substr(0, pos);
            const std::string_view maxPart = value.substr(pos + 2);

            std::string unitMin, unitMax;
            if (parsePart(minPart, result.minValue, unitMin) &&
                parsePart(maxPart, result.maxValue, unitMax))
            {
                result.durationType = unitMax; // Default to second unit if they differ
                
                if (unitMin != unitMax)
                {
                    // Normalize minValue to unitMax
                    const auto msMin = waitDelayInputToMilliseconds(result.minValue, unitMin);
                    if (unitMax == "m")
                    {
                        result.minValue = static_cast<double>(msMin.count()) / 60000.0;
                    }
                    else if (unitMax == "s")
                    {
                        result.minValue = static_cast<double>(msMin.count()) / 1000.0;
                    }
                    else
                    {
                        result.minValue = static_cast<double>(msMin.count());
                    }
                }
                return result;
            }
            return std::nullopt;
        }

        if (parsePart(value, result.minValue, result.durationType))
        {
            result.maxValue = result.minValue;
            result.useRange = false;
            return result;
        }

        return std::nullopt;
    }

    std::string formatWaitDelayInput(const WaitDelayInput& input)
    {
        if (!input.hasValue)
        {
            return "";
        }

        auto formatValue = [](const double val) -> std::string {
            std::string s = std::format("{:.3f}", val);
            // Remove trailing zeros and dot
            if (s.contains('.'))
            {
                while (s.ends_with('0')) s.pop_back();
                if (s.ends_with('.')) s.pop_back();
            }
            return s;
        };

        if (input.useRange)
        {
            return std::format("{}..{}{}", formatValue(input.minValue), formatValue(input.maxValue), input.durationType);
        }

        return std::format("{}{}", formatValue(input.minValue), input.durationType);
    }

    std::chrono::milliseconds waitDelayInputToMilliseconds(const double value, const std::string_view durationType)
    {
        if (durationType == "m")
        {
            return std::chrono::milliseconds{ static_cast<int64_t>(std::round(value * 60.0 * 1000.0)) };
        }

        if (durationType == "s")
        {
            return std::chrono::milliseconds{ static_cast<int64_t>(std::round(value * 1000.0)) };
        }

        return std::chrono::milliseconds{ static_cast<int64_t>(std::round(value)) };
    }
}

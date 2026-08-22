/**
 * @file arguments.cpp
 * @author djsquiddy
 * @date March 2026
 */
#include "autoinput/cli/arguments.h"
#include "autoinput/input/waitDelay.h"
#include "autoinput/config/defaults.h"
#include "autoinput/support/logger.h"

namespace autoinput
{
    ProgramArguments::ProgramArguments()
    {
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
        const auto& [start, end, press, release, action, button, application, settingsBlacklist, statusNotification, settingsLogLevel, setupCompleted, uiLanguage] = m_settings.getDefaults();

        if (!settingsBlacklist.empty())
        {
            blacklist.insert(blacklist.end(), settingsBlacklist.begin(), settingsBlacklist.end());
        }

        if (statusNotificationMode == StatusNotificationMode::Console && !statusNotification.empty())
        {
            statusNotificationMode = statusNotificationModeFromString(statusNotification);
        }

        if (Logger::getLogLevel() == LogLevel::Info) // Default
        {
            if (!settingsLogLevel.empty())
            {
                if (const LogLevel level = logLevelFromString(settingsLogLevel); level != LogLevel::Unknown)
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

    ConfigData ProgramArguments::toConfigData() const
    {
        ConfigData data;
        data.endKey = endKey;
        data.application = applicationName;
        data.blacklist = blacklist;
        data.statusNotificationMode = statusNotificationModeToString(statusNotificationMode);
        data.logLevel = logLevelToString(Logger::getLogLevel());
 
        const size_t buttonCount = buttons.size();
        const size_t keyCount = keys.size();
        const size_t targetCount = buttonCount + keyCount;
        const size_t actionCount = targetActions.size();
        const size_t startKeyCount = startKeys.size();
        const size_t nameCount = commandNames.size();
        const size_t groupCount = exclusiveGroups.size();
        const size_t controlCount = commandControls.size();

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

            if (i < controlCount)
            {
                cmd.controls = commandControls[i];
            }

            cmd.pressWait = delayData.toString(true);
            cmd.releaseWait = delayData.toString(false);

            data.commands.push_back(std::move(cmd));
        }

        data.sequences = sequences;

        return data;
    }
}

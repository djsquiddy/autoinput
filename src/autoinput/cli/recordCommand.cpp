/**
 * @file recordCommand.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/cli/recordCommand.h"

#include "autoinput/cli/arguments.h"
#include "autoinput/cli/cliApplication.h"
#include "autoinput/config/defaults.h"
#include "autoinput/support/errorCode.h"
#include "autoinput/config/config.h"
#include "autoinput/support/logger.h"

namespace autoinput::cli
{
    namespace
    {
        [[nodiscard]] bool applyRecordConfigToArguments(
            const RecordConfig& config,
            const CommandContext& context,
            ProgramArguments& arguments)
        {
            arguments.programName = context.global.programName;
            arguments.jsonOutput = context.global.jsonOutput;

            arguments.recordName = config.name;
            arguments.recordStartKey = config.startKey;
            arguments.recordEndKey = config.endKey;
            arguments.recordPlayStartKey = config.playStartKey;
            arguments.recordMouseMoves = config.mouseMoves;
            arguments.recordMouseSample = config.mouseSample;
            arguments.forceOverwrite = config.force;

            return arguments.postParseArguments();
        }
    }

    bool RecordCommand::parse(const gsl::span<char*> args, i32& index)
    {
        if (index >= args.size())
        {
            Logger::fatal("The record command needs a NAME argument.\n");
            return false;
        }

        const std::string_view recordName = safeGetNextArgument(index, args);
        if (recordName.empty())
        {
            Logger::fatal("The record command needs a NAME argument.\n");
            return false;
        }

        m_config.name = recordName;
        ++index;

        while (index < args.size())
        {
            const std::string_view arg = args[index];

            if (arg == "--start")
            {
                const std::string_view value = safeGetNextArgument(++index, args);
                if (value.empty())
                {
                    Logger::fatal("The parameter {} needs an argument.\n", arg);
                    return false;
                }

                m_config.startKey = value;
                ++index;
                continue;
            }

            if (arg == "--end")
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

            if (arg == "--play-start")
            {
                const std::string_view value = safeGetNextArgument(++index, args);
                if (value.empty())
                {
                    Logger::fatal("The parameter {} needs an argument.\n", arg);
                    return false;
                }

                m_config.playStartKey = value;
                ++index;
                continue;
            }

            if (arg == "--mouse-moves")
            {
                m_config.mouseMoves = true;
                ++index;
                continue;
            }

            if (arg == "--mouse-sample")
            {
                const std::string_view value = safeGetNextArgument(++index, args);
                if (value.empty())
                {
                    Logger::fatal("The parameter {} needs an argument.\n", arg);
                    return false;
                }

                m_config.mouseSample = value;
                ++index;
                continue;
            }

            if (arg == "--force")
            {
                m_config.force = true;
                ++index;
                continue;
            }

            Logger::fatal("Unknown record option: {}\n", arg);
            return false;
        }

        return true;
    }

    bool RecordCommand::validate() const
    {
        if (m_config.name.empty())
        {
            Logger::fatal("The record command needs a NAME argument.\n");
            return false;
        }

        if (WaitDelayData delayData; !m_config.mouseSample.empty() && !delayData.parseWaitTimeDelay(m_config.mouseSample, true))
        {
            Logger::fatal("Invalid mouse sample interval: {}\n", m_config.mouseSample);
            return false;
        }

        return true;
    }

    ErrorCode RecordCommand::execute()
    {
        Logger::info("Starting input recording session: {}\n", m_config.name);
        return runProgramWithArguments([this](ProgramArguments& arguments)
        {
            return applyRecordConfigToArguments(m_config, m_context, arguments);
        });
    }

    void RecordCommand::printHelp() const
    {
        logHelpMessage({
            .context = m_context,
            .options ={
                { .usage = "--start KEY", .description = "Key that starts recording" },
                { .usage = "--end KEY", .description = "Key that stops recording" },
                { .usage = "--play-start KEY", .description = "Key used to play the recorded sequence" },
                { .usage = "--mouse-moves", .description = "Record mouse movement events" },
                { .usage = "--mouse-sample TIME", .description = "Mouse movement sampling interval" },
                { .usage = "--force", .description = "Overwrite destination config if it exists" },
            },
            .examples = {
                "record my-macro",
                "record my-macro --start f8 --end f9",
                "record my-macro --mouse-moves --mouse-sample 25ms",
            }
        }
        );
    }

    HelpEntry RecordCommand::getHelpEntry() const
    {
        return {
            .usage = std::format("{} NAME [options]", getName()),
            .description = "Record input events and save them as a replayable configuration.",
        };
    }
}

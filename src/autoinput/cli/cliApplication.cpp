/**
 * @file cliApplication.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/cli/cliApplication.h"

#include "appsCommand.h"
#include "configCommand.h"
#include "helpCommand.h"
#include "recordCommand.h"
#include "runCommand.h"
#include "autoinput/autoInput.h"
#include "autoinput/backendFactory.h"
#include "autoinput/configMetadata.h"
#include "autoinput/configValidator.h"
#include "autoinput/errorCode.h"
#include "autoinput/platform.h"
#include "autoinput/utils.h"

namespace autoinput::cli
{
    bool CliApplication::parse(gsl::span<char*> args)
    {
        if (args.empty())
        {
            return false;
        }

        m_context.global.programName = args[0];

        // Pre-scan for --json to disable logging as early as possible
        for (const auto& arg : args)
        {
            if (arg != nullptr && std::string_view(arg) == "--json")
            {
                m_context.global.jsonOutput = true;
                Logger::setConsoleOutputEnabled(false);
                break;
            }
        }

        m_context.settings.load();

        i32 index = 1;
        if (!parseGlobalOptions(args, index))
        {
            if (m_context.global.jsonOutput)
            {
                printErrorJson({
                    {
                        .code = ErrorCode::FailedToParseGlobalOptions,
                        .message = "Failed to parse global options"
                    }
                });
            }
            return false;
        }

        if (m_command)
        {
            return true;
        }

        if (index >= args.size())
        {
            m_command = createCommand("help", m_context);
            return m_command != nullptr;
        }

        const std::string_view commandName = args[index++];
        m_command = createCommand(commandName, m_context);
        if (!m_command)
        {
            Logger::fatalError({
                .code = ErrorCode::UnknownCommand,
                .message = std::format("Unknown command: {}", commandName)
            });
            printUsage();
            return false;
        }

        if (!m_command->parse(args, index))
        {
            Logger::fatalError({
                .code = ErrorCode::FailedToParseCommandOptions,
                .message = std::format("Failed to parse {} command options", commandName)
            });
            return false;
        }

        if (index < args.size())
        {
            Logger::fatalError({
            .code = ErrorCode::UnexpectedArgument,
            .message = std::format("Unexpected argument: {}", args[index])
            });
            return false;
        }

        if (!m_command->validate())
        {
            Logger::fatalError({
                .code = ErrorCode::CliValidationError,
                .message = std::format("Validation failed for {} command", commandName)
            });
            m_command->printHelp();
            return false;
        }

        return true;
    }

    bool CliApplication::parseGlobalOptions(const gsl::span<char*> args, i32& index)
    {
        m_context.global.showExamples = contains(args, "--examples");

        while (index < args.size())
        {
            const std::string_view arg = args[index];

            if (arg == "-h" || arg == "--help")
            {
                m_command = createCommand("help", m_context);
                return m_command != nullptr;
            }

            if (arg == "--examples")
            {
                m_context.global.showExamples = true;
                m_command = createCommand("help", m_context);
                return m_command != nullptr;
            }

            if (arg == "-l" || arg == "--log")
            {
                const std::string_view logLevelStr = safeGetNextArgument(++index, args);
                if (logLevelStr.empty())
                {
                    Logger::fatal("The parameter {} needs an argument. Choices: {}\n", arg, ConfigMetadata::validLogLevelChoices());
                    return false;
                }

                const LogLevel logLevel = logLevelFromString(logLevelStr);
                if (logLevel == LogLevel::Unknown)
                {
                    Logger::fatal("Invalid parameter {} for log level. Choices: {}\n", logLevelStr, ConfigMetadata::validLogLevelChoices());
                    return false;
                }

                m_context.global.logLevel = logLevel;
                Logger::setLogLevel(logLevel);
                ++index;
                continue;
            }

            if (arg == "--json")
            {
                // Already handled in parse() pre-scan
                ++index;
                continue;
            }

            if (arg.starts_with('-'))
            {
                Logger::fatal("Unknown global option: {}\n", arg);
                return false;
            }

            break;
        }

        return true;
    }

    i32 CliApplication::execute()
    {
        if (m_context.global.jsonOutput)
        {
            Logger::setConsoleOutputEnabled(false);
        }

        if (!m_command)
        {
            printUsage();
            return static_cast<i32>(ErrorCode::InvalidParam);
        }

        return m_command->execute();
    }

    void CliApplication::printUsage()
    {
        const auto helpCmd = createCommand("help", m_context);
        (void)helpCmd->execute();
    }

    i32 runProgramWithArguments(const std::function<bool(ProgramArguments&)>& configureArguments)
    {
        g_program = std::make_unique<Program>();

        if (!configureArguments(g_program->arguments()))
        {
            return static_cast<i32>(ErrorCode::InvalidParam);
        }

        auto backend = BackendFactory::createPlatformBackend();
        if (!backend)
        {
            return static_cast<i32>(ErrorCode::FailedToInstallHooks);
        }

        g_program->setBackend(std::move(backend));
        if (!g_program->init())
        {
            return static_cast<i32>(ErrorCode::FailedToInstallHooks);
        }

        if (!installHooks())
        {
            return static_cast<i32>(ErrorCode::FailedToInstallHooks);
        }

        g_program->printProgramInfo();
        platform::setupSignalHandler();

        Logger::print("Global keyboard listener started. Press Ctrl+C to exit.\n\n");

        runListener();

        cleanup();

        return static_cast<i32>(ErrorCode::Success);
    }

    std::unique_ptr<CommandBase> createCommand(const std::string_view commandName, CommandContext& context)
    {
        if (commandName == "run")
        {
            return std::make_unique<RunCommand>(context);
        }

        if (commandName == "record")
        {
            return std::make_unique<RecordCommand>(context);
        }

        if (commandName == "config")
        {
            return std::make_unique<ConfigCommand>(context);
        }

        if (commandName == "apps")
        {
            return std::make_unique<AppsCommand>(context);
        }

        if (commandName == "help")
        {
            return std::make_unique<HelpCommand>(context);
        }

        return nullptr;
    }

}

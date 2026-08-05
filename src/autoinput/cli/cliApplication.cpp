/**
 * @file cliApplication.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/cli/cliApplication.h"
#include "autoinput/cli/appsCommand.h"
#include "autoinput/cli/configCommand.h"
#include "autoinput/cli/helpCommand.h"
#include "autoinput/cli/recordCommand.h"
#include "autoinput/cli/runCommand.h"
#include "autoinput/autoinput.h"
#include "autoinput/backendFactory.h"
#include "autoinput/configMetadata.h"
#include "autoinput/errorCode.h"
#include "autoinput/platform.h"
#include "autoinput/utils.h"

namespace autoinput::cli
{
    ErrorCode CliApplication::parse(gsl::span<char*> cliArgs)
    {
        if (cliArgs.empty())
        {
            return ErrorCode::MissingCommandLineArgument;
        }

        m_context.global.programPath = safeGetNextArgument(0, cliArgs);
        m_context.global.programName = std::filesystem::path(m_context.global.programPath).filename().string();

        // m_context.global.showExamples = contains(args, "--examples");
        // m_context.global.jsonOutput = contains(args, "--json");
        std::vector<char*> cleanedArgs{ };
        cleanedArgs.reserve(cliArgs.size());
        bool showHelp = false;

        // Pre-scan for --json to disable logging as early as possible
        for (i32 i = 1; i < cliArgs.size(); ++i)
        {
            if (const std::string_view arg = cliArgs[i]; arg == "--json")
            {
                m_context.global.jsonOutput = true;
                Logger::setConsoleOutputEnabled(false);
            }
        }

        for (i32 i = 1; i < cliArgs.size(); ++i)
        {
            if (const std::string_view arg = cliArgs[i]; arg == "--json")
            {
                continue;
            }
            else if (arg == "--examples")
            {
                showHelp = true;
                m_context.global.showExamples = true;
            }
            else if (arg == "--help")
            {
                showHelp = true;
            }
            else if (arg == "--log")
            {
                if (const auto logLevelErrorCode = parseLogLevel(cliArgs, i); logLevelErrorCode != ErrorCode::Success)
                {
                    return logLevelErrorCode;
                }
            }
            else
            {
                cleanedArgs.push_back(cliArgs[i]);
            }
        }

        // Removed the global arguments.
        const gsl::span args{ cleanedArgs };
        printCliArguments(cliArgs);
        printCliArguments(args);

        m_context.settings.load();
        i32 index = 0;
        if (const auto arg = safeGetNextArgument(index, args); arg.starts_with('-'))
        {
            return Logger::fatalError({
                .code = ErrorCode::FailedToParseGlobalOptions,
                .message = std::format("Unknown global option: {}\n", arg)
            });
        }

        if (m_command)
        {
            return ErrorCode::Success;
        }

        std::string_view commandName;
        if (showHelp)
        {
            commandName = "help";
            m_command = createCommand("help", m_context);
        }
        else if (index >= args.size())
        {
            m_command = createCommand("help", m_context);
            return m_command != nullptr ? ErrorCode::Success : ErrorCode::UnknownCommand;
        }

        if (!showHelp)
        {
            commandName = safeGetNextArgument(index++, args);
        }

        m_command = createCommand(commandName, m_context);
        if (!m_command)
        {
            auto printHelpMenu = gsl::finally([this] {this->printUsage();});
            return Logger::fatalError({
                .code = ErrorCode::UnknownCommand,
                .message = std::format("Unknown command: {}", commandName)
            });
        }

        Logger::debug("Parsing command: {}\n", commandName);
        if (!m_command->parse(args, index))
        {
            auto printHelpMenu = gsl::finally([this] {this->printUsage();});
            return Logger::fatalError({
                .code = ErrorCode::FailedToParseCommandOptions,
                .message = std::format("Failed to parse {} command options", commandName)
            });
        }

        if (index < args.size())
        {
            auto printHelpMenu = gsl::finally([this] {this->printUsage();});
            return Logger::fatalError({
                .code = ErrorCode::UnexpectedArgument,
                .message = std::format("Unexpected argument: {}", args[index])
            });
        }

        Logger::debug("Validating command: {}\n", commandName);
        if (!m_command->validate())
        {
            auto printHelpMenu = gsl::finally([this] {m_command->printHelp();});
            return Logger::fatalError({
                .code = ErrorCode::CliValidationError,
                .message = std::format("Validation failed for {} command", commandName)
            });
        }

        return ErrorCode::Success;
    }

    ErrorCode CliApplication::parseGlobalOptions(const gsl::span<char*> args, i32& index)
    {
        while (index < args.size())
        {
            const std::string_view arg = args[index];

            break;
        }

        return ErrorCode::Success;
    }

    ErrorCode CliApplication::execute()
    {
        if (m_context.global.jsonOutput)
        {
            Logger::setConsoleOutputEnabled(false);
        }

        if (!m_command)
        {
            printUsage();
            return ErrorCode::InvalidParam;
        }

        return m_command->execute();
    }

    void CliApplication::printUsage()
    {
        HelpCommand helpCmd{ m_context };
        if (m_command)
        {
            helpCmd.setHelpTopic({ std::string(m_command->getName()) });
        }
        AUTOINPUT_UNUSED(helpCmd.execute());
    }

    ErrorCode CliApplication::parseLogLevel(const gsl::span<char*> args, i32& index)
    {
        const std::string_view arg = args[index];
        const std::string_view logLevelStr = safeGetNextArgument(++index, args);
        if (logLevelStr.empty())
        {
            return Logger::fatalError({
                .code = ErrorCode::MissingCommandLineArgument,
                .message = std::format("The parameter {} needs an argument. Choices: {}\n", arg, ConfigMetadata::validLogLevelChoices())
            });
        }

        const LogLevel logLevel = logLevelFromString(logLevelStr);
        if (logLevel == LogLevel::Unknown)
        {
            return Logger::fatalError({
                .code = ErrorCode::FailedToParseCommandOptions,
                .message = std::format("Invalid parameter {} for log level. Choices: {}\n", logLevelStr, ConfigMetadata::validLogLevelChoices())
            });
        }

        m_context.global.logLevel = logLevel;
        Logger::setLogLevel(logLevel);
        return ErrorCode::Success;
    }

    void CliApplication::printCliArguments(const gsl::span<char*> args)
    {
        Logger::debug("CLI args:\n");
        for (auto i = 0; i < args.size(); ++i)
        {
            Logger::debug("  {}: {}\n", i, args[i]);
        }
    }

    ErrorCode runProgramWithArguments(const std::function<bool(ProgramArguments&)>& configureArguments)
    {
        g_program = std::make_unique<Program>();

        if (!configureArguments(g_program->arguments()))
        {
            return Logger::fatalError({
                .code = ErrorCode::InvalidParam,
                .message = "Failed to configure program arguments."
            });
        }

        auto backend = BackendFactory::createPlatformBackend();
        if (!backend)
        {
            return Logger::fatalError({
                .code = ErrorCode::FailedToInstallHooks,
                .message = "Failed to create platform backend."
            });
        }

        g_program->setBackend(std::move(backend));
        if (!g_program->init())
        {
            return Logger::fatalError({
                .code = ErrorCode::FailedToInstallHooks,
                .message = "Program initialization failed."
            });
        }

        if (!installHooks())
        {
            return Logger::fatalError({
                .code = ErrorCode::FailedToInstallHooks,
                .message = "Failed to install input hooks."
            });
        }

        g_program->printProgramInfo();
        platform::setupSignalHandler();

        Logger::print("Global keyboard listener started. Press Ctrl+C to exit.\n\n");

        runListener();

        Logger::debug("Exiting listener loop.\n");

        cleanup();

        return ErrorCode::Success;
    }

    std::unique_ptr<CommandBase> createCommand(const std::string_view commandName, CommandContext& context)
    {
        Logger::debug("Creating command: {}\n", commandName);
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

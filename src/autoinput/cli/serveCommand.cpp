/**
 * @file serveCommand.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "autoinput/cli/serveCommand.h"
#include "autoinput/services/automationRuntimeClient.h"
#include "autoinput/services/runtimeProtocol.h"
#include "autoinput/errorCode.h"
#include "autoinput/logger.h"
#include <iostream>
#include <string>

namespace autoinput::cli
{
    HelpEntry ServeCommand::getHelpEntry() const
    {
        return {
            .usage = "serve --stdio",
            .description = "Starts the automation runtime server."
        };
    }

    bool ServeCommand::parse(gsl::span<char*> args, i32& index)
    {
        while (index < args.size())
        {
            const std::string_view arg = args[index++];
            if (arg == "--stdio")
            {
                m_stdio = true;
            }
            else
            {
                Logger::error("Unknown argument for serve command: {}\n", arg);
                return false;
            }
        }
        return true;
    }

    bool ServeCommand::validate() const
    {
        if (!m_stdio)
        {
            Logger::error("The serve command requires --stdio flag.\n");
            return false;
        }
        return true;
    }

    ErrorCode ServeCommand::execute()
    {
        if (m_stdio)
        {
            return runStdioRuntimeServer();
        }
        return ErrorCode::InvalidParam;
    }

    void ServeCommand::printHelp() const
    {
        logHelpMessage({
            .context = m_context,
            .commands = {},
            .options = {
                { "--stdio", "Use standard input/output for the protocol." }
            },
            .examples = {
                "serve --stdio"
            }
        });

        logHelpStrings("Runtime JSON-line protocol", {
            "The server reads one JSON object per line from stdin and responds with one JSON object per line to stdout.",
            "\nExample requests:",
            "  {\"id\":1,\"method\":\"status\"}",
            "  {\"id\":2,\"method\":\"start\",\"params\":{\"config\":\"autoinput\"}}",
            "  {\"id\":3,\"method\":\"stop\"}",
            "  {\"id\":4,\"method\":\"shutdown\"}"
        });
    }

    ErrorCode ServeCommand::runStdioRuntimeServer()
    {
        using namespace autoinput::services;

        Logger::setConsoleOutputEnabled(false);

        InProcessAutomationRuntimeClient runtime;

        for (std::string line; std::getline(std::cin, line);)
        {
            auto request = parseRuntimeRequest(line);
            RuntimeOperationResult result;

            if (!request.valid)
            {
                result = { false, runtime.getStatus(), request.error.empty() ? "Invalid request." : request.error };
                std::cout << buildRuntimeResponse(request.id, result) << '\n' << std::flush;
                continue;
            }

            if (request.method == "start")
            {
                result = runtime.start(request.config);
            }
            else if (request.method == "stop")
            {
                result = runtime.stop();
            }
            else if (request.method == "pause")
            {
                result = runtime.pause();
            }
            else if (request.method == "resume")
            {
                result = runtime.resume();
            }
            else if (request.method == "run_command")
            {
                result = runtime.runCommand(request.config, request.command);
            }
            else if (request.method == "status")
            {
                result = { true, runtime.getStatus(), "Status retrieved." };
            }
            else if (request.method == "shutdown")
            {
                result = runtime.stop();
                std::cout << buildRuntimeResponse(request.id, result) << '\n' << std::flush;
                break;
            }
            else
            {
                result = { false, runtime.getStatus(), "Unknown method." };
            }

            std::cout << buildRuntimeResponse(request.id, result) << '\n' << std::flush;
        }

        AUTOINPUT_UNUSED(runtime.stop());
        return ErrorCode::Success;
    }
}

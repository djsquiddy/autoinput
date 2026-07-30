/**
 * @file main.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include "autoinput.h"
#include "utils.h"
#include "logger.h"
#include "platform.h"

int main(int argc, char* argv[])
{
    using namespace autoinput;

    try
    {
        // Configure file output once at startup

        Logger::setFile("app.log");
        Logger::info("Application started.");
        g_program = std::make_unique<Program>();
        if (!g_program->arguments().parseArguments(gsl::make_span(argv, argc), true))
        {
            return static_cast<int>(ErrorCode::INVALID_PARAM);
        }

        if (g_program->arguments().listApplications)
        {
            const auto apps = platform::getRunningApplicationNames();
            if (apps.empty())
            {
                std::cout << "No running applications found or listing not supported on this platform.\n";
            }
            else
            {
                std::cout << "Currently running applications:\n";
                for (const auto& app : apps)
                {
                    std::cout << "  - " << app << "\n";
                }
            }
            return static_cast<int>(ErrorCode::SUCCESS);
        }

        g_program->init();
        g_program->printProgramInfo();
        platform::setupSignalHandler();
        std::cout << "Global keyboard listener started. Press Ctrl+C to exit.\n\n";

        if (!installHooks())
        {
            return static_cast<int>(ErrorCode::FAILED_TO_INSTALL_HOOKS);
        }
        runListener();
        if (g_program)
        {
            g_program->joinThreads();
        }
        cleanup();
    }
    catch (const std::exception& e)
    {
        Logger::fatal("Unhandled exception: {}\n", e.what());
        return static_cast<int>(ErrorCode::UNHANDLED_EXCEPTION);
    }
    catch (...)
    {
        Logger::fatal("Unknown unhandled exception occurred.\n");
        return static_cast<int>(ErrorCode::UNHANDLED_EXCEPTION);
    }

    return static_cast<int>(ErrorCode::SUCCESS);
}

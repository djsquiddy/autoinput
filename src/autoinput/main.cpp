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

    // Configure file output once at startup

    Logger::setFile("app.log");
    Logger::info("Application started.");
    g_program = std::make_unique<Program>();
    if (!g_program->arguments().parseArguments(gsl::make_span(argv, argc)))
    {
        return static_cast<int>(ErrorCode::INVALID_PARAM);
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
    cleanup();

    return static_cast<int>(ErrorCode::SUCCESS);
}

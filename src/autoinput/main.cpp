
/*
 *
 * TODO: need to separate the code to be platform independent.
*/

#include "autoinput.h"
#include "utils.h"
#include "logger.h"

int main(int argc, char* argv[])
{
    using namespace autoinput;

    // Configure file output once at startup

    Logger::instance().setFile("app.log");
    Logger::info("Application started.");
    g_program = std::make_unique<Program>();
    if (!g_program->arguments().parseArguments(argc, argv))
    {
        return static_cast<int>(ErrorCode::INVALID_PARAM);
    }

    g_program->init();
    g_program->printProgramInfo();
    std::cout << "Global keyboard listener started. Press Ctrl+C to exit.\n\n";

    if (!installHooks())
    {
        return static_cast<int>(ErrorCode::FAILED_TO_INSTALL_HOOKS);
    }
    runListener();
    cleanup();

    return static_cast<int>(ErrorCode::SUCCESS);
}

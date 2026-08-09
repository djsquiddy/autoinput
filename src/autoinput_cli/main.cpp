/**
 * @file main.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include "autoinput/autoinput.h"
#include "autoinput/logger.h"
#include "autoinput/terminal.h"
#include "autoinput/environment.h"
#include "autoinput/errorCode.h"
#include "autoinput/cli/cliApplication.h"

int main(int argc, char** argv)
{
    using namespace autoinput;

    try
    {
        terminal::setup();
        const auto logPath = SystemEnvironment::instance().executableDirectoryPath() / "app.log";
        Logger::setFile(logPath.string());

        cli::CliApplication app;
        if (auto result = app.parse(gsl::make_span(argv, argc)); result != ErrorCode::Success)
        {
            return static_cast<int>(result);
        }

        return static_cast<int>(app.execute());
    }
    catch (const std::exception& e)
    {
        Logger::fatal("Unhandled exception: {}\n", e.what());
        return static_cast<int>(ErrorCode::UnhandledException);
    }
    catch (...)
    {
        Logger::fatal("Unknown unhandled exception occurred.\n");
        return static_cast<int>(ErrorCode::UnhandledException);
    }
}

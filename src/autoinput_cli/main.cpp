/**
 * @file main.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include "autoinput/autoInput.h"
#include "autoinput/logger.h"
#include "autoinput/terminal.h"
#include "autoinput/errorCode.h"
#include "autoinput/cli/cliApplication.h"

int main(int argc, char* argv[])
{
    using namespace autoinput;

    try
    {
        terminal::setup();
        Logger::setFile("app.log");

        cli::CliApplication app;
        if (!app.parse(gsl::make_span(argv, argc)))
        {
            return static_cast<int>(ErrorCode::InvalidParam);
        }

        return app.execute();
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

/**
 * @file main.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "app/uiApplication.h"
#include "autoinput/logger.h"
#include "autoinput/platform.h"
#include <exception>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace
{
    int main_internal()
    {
        using namespace autoinput;
        using namespace autoinput::ui;

        try
        {
            const auto logPath = platform::getExecutablePath() / "ui.log";
            Logger::setFile(logPath.string());

            UiApplication app{};
            app.run();

            return 0;
        }
        catch (const std::exception& e)
        {
            Logger::error("Unhandled exception in UI: {}\n", e.what());
            return 1;
        }
        catch (...)
        {
            Logger::error("Unknown unhandled exception occurred in UI.\n");
            return 1;
        }
    }
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    return main_internal();
}
#else
int main(int argc, char** argv)
{
    return main_internal();
}
#endif

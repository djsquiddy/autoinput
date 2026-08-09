/**
 * @file main.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "trayApp.h"
#include "autoinput/logger.h"
#include "autoinput/environment.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

namespace
{
    int main_internal()
    {
        using namespace autoinput;
        Logger::setLogLevel(LogLevel::Info);
        const auto logPath = SystemEnvironment::instance().executableDirectoryPath() / "traylog";
        Logger::setFile(logPath.string());

        // ReSharper disable once CppLocalVariableMayBeConst
        std::unique_ptr<tray::TrayApp> trayApp = tray::createTrayApp();

        if (!trayApp->init())
        {
            return 1;
        }

        trayApp->run();

        return 0;
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

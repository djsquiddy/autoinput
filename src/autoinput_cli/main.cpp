/**
 * @file main.cpp
 * @author djsquiddy
 * @date July 2026
 */
#include "autoinput/autoInput.h"
#include "autoinput/utils.h"
#include "autoinput/logger.h"
#include "autoinput/platform.h"
#include "autoinput/backendFactory.h"

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

        if (g_program->arguments().listConfigs)
        {
            auto listConfigsFromDir = [](const std::filesystem::path& path, const std::string& label) {
                if (std::filesystem::exists(path) && std::filesystem::is_directory(path))
                {
                    std::cout << label << " configurations (in " << path.string() << "):\n";
                    bool found = false;
                    for (const auto& entry : std::filesystem::directory_iterator(path))
                    {
                        if (entry.is_regular_file() && entry.path().extension() == ".toml")
                        {
                            const std::string name = entry.path().stem().string();
                            if (name == "settings")
                            {
                                continue;
                            }
                            std::cout << "  - " << name << "\n";
                            found = true;
                        }
                    }
                    if (!found)
                    {
                        std::cout << "  (none)\n";
                    }
                    std::cout << "\n";
                }
            };

            listConfigsFromDir(getConfigsPath(), "Global");
            listConfigsFromDir(getUserConfigsPath(), "User");
            return static_cast<int>(ErrorCode::SUCCESS);
        }

        if (!g_program->arguments().saveConfigName.empty())
        {
            const std::string& saveConfigName = g_program->arguments().saveConfigName;
            std::filesystem::path dumpPath = getUserConfigsPath();
            if (dumpPath.empty())
            {
                Logger::fatal("Could not determine user configuration directory.\n");
                return static_cast<int>(ErrorCode::FAILED_TO_LOAD_CONFIG);
            }

            if (!std::filesystem::exists(dumpPath))
            {
                std::filesystem::create_directories(dumpPath);
            }

            std::string fileName = saveConfigName;
            if (!fileName.ends_with(".toml"))
            {
                fileName += ".toml";
            }
            dumpPath /= fileName;

            const ConfigData configData = g_program->arguments().toConfigData();
            if (saveConfigData(configData, dumpPath, g_program->arguments().getSettings().getDefaults()))
            {
                std::cout << "Configuration saved to " << dumpPath.string() << "\n";
                return static_cast<int>(ErrorCode::SUCCESS);
            }
            
            Logger::fatal("Failed to save configuration to {}\n", dumpPath.string());
            return static_cast<int>(ErrorCode::FAILED_TO_LOAD_CONFIG);
        }

        auto backend = BackendFactory::createPlatformBackend();
        if (!backend)
        {
            return static_cast<int>(ErrorCode::FAILED_TO_INSTALL_HOOKS);
        }
        
        g_program->setBackend(std::move(backend));
        g_program->init();

        if (!installHooks())
        {
            return static_cast<int>(ErrorCode::FAILED_TO_INSTALL_HOOKS);
        }
        g_program->printProgramInfo();
        platform::setupSignalHandler();
        std::cout << "Global keyboard listener started. Press Ctrl+C to exit.\n\n";
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

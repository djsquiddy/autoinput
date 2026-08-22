/**
 * @file trayApp.cpp
 * @author djsquiddy
 * @date August 2026
 */
#include "trayApp.h"
#include "autoinput/support/logger.h"
#include "autoinput/platform/backendFactory.h"
#include "autoinput/app/autoinput.h"
#include "autoinput/platform/platform.h"
#include "autoinput/services/configService.h"
#include <iostream>


namespace autoinput::tray
{
    TrayApp::TrayApp()
    {
        m_environment = std::make_unique<SystemEnvironment>();
        m_configService = std::make_unique<services::ConfigService>(*m_environment);
        m_controller = std::make_unique<AutomationController>();
        
        m_controller->setStatusCallback([this](const ProgramStatus& status) {
            onStatusChanged(status);
        });
    }

    TrayApp::~TrayApp() = default;

    bool TrayApp::init()
    {
        Logger::info("Initializing TrayApp");
        refreshConfigs();
        refreshApplications();
        return true;
    }

    void TrayApp::run()
    {
        // Default implementation for stub/unsupported platforms
        Logger::fatal("Tray application is not supported on this platform.\n");
        m_running = false;
    }

    void TrayApp::shutdown()
    {
        if (m_shutdown) return;
        Logger::info("Shutting down TrayApp");
        m_shutdown = true;

        if (m_controller && m_controller->running())
        {
            m_controller->stop();
        }
        if (m_trayIcon)
        {
            m_trayIcon->hide();
        }
        m_running = false;
    }

    void TrayApp::updateMenu()
    {
        if (!m_trayIcon)
        {
            return;
        }

        std::vector<MenuItem> items{};
        
        std::string statusText = m_controller->running() ? "Status: Running" : "Status: Idle";
        if (m_controller->running() && !m_lastStatus.triggeredCommandName.empty())
        {
            statusText += " [" + m_lastStatus.triggeredCommandName + "]";
        }
        
        items.push_back({.text = statusText});
        items.push_back(createSeperatorMenuItem());

        const auto currentConfig = m_configService->getCurrentConfig();
        if (!m_controller->running())
        {
            if (currentConfig.empty())
            {
                items.push_back({
                    .text = "Start Default",
                    .action = [this] { startAutomation(""); },
                    .state = MenuItemState::Enabled
                });
            }
            else
            {
                items.push_back({
                    .text = std::format("Start {}", currentConfig),
                    .action = [this, name = currentConfig] { startAutomation(name); },
                    .state = MenuItemState::Enabled
                });
            }
        }
        else
        {
            if (!m_lastStatus.triggeredCommandName.empty())
            {
                const bool isActive = m_lastStatus.triggeredCommandActive.value_or(true);
                items.push_back({
                    .text = isActive ? std::format("Pause Command ({})", m_lastStatus.triggeredCommandName) : std::format("Resume Command ({})", m_lastStatus.triggeredCommandName),
                    .action = [this, name = m_lastStatus.triggeredCommandName]
                    {
                        if (m_controller)
                        {
                            m_controller->togglePauseCommand(name);
                        }
                    },
                    .state = MenuItemState::Enabled
                });

                items.push_back({
                    .text = std::format("Stop Command ({})", m_lastStatus.triggeredCommandName),
                    .action = [this, name = m_lastStatus.triggeredCommandName]
                    {
                        if (m_controller)
                        {
                            m_controller->stopCommand(name);
                        }
                    },
                    .state = MenuItemState::Enabled
                });
            }

            items.push_back({
                .text = "Stop All Commands",
                .action = [this]
                {
                    if (m_controller)
                    {
                        m_controller->stopAllCommands();
                    }
                },
                .state = MenuItemState::Enabled
            });

            items.push_back({
                .text = "Stop Automation",
                .action = [this] { stopAutomation(); },
                .state = MenuItemState::Enabled
            });
        }

        std::vector<MenuItem> configItems;
        configItems.reserve(m_availableConfigs.size());
        for (const auto& config : m_availableConfigs)
        {
            configItems.push_back({
                .text = config.name,
                .action = [this, name = config.name] { startAutomation(name); },
                .state = MenuItemState::Enabled | (config.isSelected ? MenuItemState::Checked : MenuItemState::None)
            });
        }
        items.push_back({
            .text = "Configurations",
            .action = nullptr,
            .state = MenuItemState::Enabled,
            .subItems = std::move(configItems)
        });

        // Configs
        items.push_back(createSeperatorMenuItem());
        items.push_back({
            .text = "Refresh Configs",
            .action = [this] { refreshConfigs(); },
            .state = MenuItemState::Enabled
        });
        items.push_back({
            .text = "Open Config Folder",
            .action = [this] { openConfigFolder(); },
            .state = MenuItemState::Enabled
        });

        items.push_back(createSeperatorMenuItem());
        // TODO: ignore if the platform does not support getting the active applications.
        // an example would be wayland.
        if (!m_availableApplications.empty())
        {
            std::vector<MenuItem> applications{};
            applications.reserve(m_availableApplications.size());

            for (const auto& [name, isSelected] : m_availableApplications)
            {
                MenuItem appItem = {
                    .text = name,
                    .action = [this, name = name]
                    {
                        m_configService->setApplicationName(name);
                        updateMenu();
                    },
                    .state = MenuItemState::Enabled | (isSelected ? MenuItemState::Checked : MenuItemState::None)
                };

                applications.push_back(appItem);
            }
            items.push_back({
                .text = "Applications",
                .state = MenuItemState::Enabled,
                .subItems = std::move(applications)
            });
        }

        items.push_back(createSeperatorMenuItem());
        items.push_back({
            .text = "Exit",
            .action = [this] { shutdown(); },
            .state = MenuItemState::Enabled
        });

        m_trayIcon->setMenu(items);
        
        std::string tooltip = "AutoInput - " + (m_controller->running() ? std::string("Running") : std::string("Idle"));
        if (m_controller->running() && !m_lastStatus.triggeredCommandName.empty())
        {
            tooltip += "\nActive: " + m_lastStatus.triggeredCommandName;
        }
        m_trayIcon->setTooltip(tooltip);
    }

    void TrayApp::startAutomation(const std::string& configName)
    {
        Logger::info("Starting automation with config: {}", configName.empty() ? "default" : configName);
        if (m_controller->running())
        {
            m_controller->stop();
        }

        ProgramArguments args;
        if (!m_configService->applyConfigToArguments(configName, args))
        {
            m_trayIcon->showNotification("Error", "Failed to load config: " + (configName.empty() ? "default" : configName));
            return;
        }

        if (m_controller->start(std::move(args)))
        {
            updateMenu();
        }
        else
        {
            m_trayIcon->showNotification("Error", "Failed to start automation.");
        }
    }

    void TrayApp::stopAutomation()
    {
        Logger::info("Stopping automation");
        m_controller->stop();
        updateMenu();
    }

    void TrayApp::openConfigFolder()
    {
        // Stub
    }

    void TrayApp::refreshConfigs()
    {
        Logger::debug("Refreshing configurations");
        m_availableConfigs.clear();
        const auto configs = m_configService->listAvailableConfigs();
        const auto currentConfig = m_configService->getCurrentConfig();
        m_availableConfigs.reserve(configs.size());
        for (const auto& config : configs)
        {
            m_availableConfigs.push_back({
                .name = config.fileStem(),
                .path = config.filepath.string(),
                .isUser = config.type == ConfigType::User,
                .isSelected = config.filepath.string() == currentConfig
            });
        }
        updateMenu();
    }

    void TrayApp::refreshApplications()
    {
        Logger::debug("Refreshing running applications");
        const auto apps = platform::getRunningApplicationNames();
        const auto currentSetApplicationName = m_configService->getApplicationName().value_or("");
        m_availableApplications.clear();
        m_availableApplications.reserve(apps.size());
        for (const auto& app : apps)
        {
            m_availableApplications.push_back({
                .name = app,
                .isSelected = app == currentSetApplicationName
            });
        }
    }

    void TrayApp::onStatusChanged(const ProgramStatus& status)
    {
        Logger::debug("Status changed: {}", status.triggeredCommandName.empty() ? "Idle" : status.triggeredCommandName);
        queueUIUpdate([this, status]()
        {
            m_lastStatus = status;
            updateMenu();
        });
    }

    void TrayApp::queueUIUpdate(std::function<void()> update)
    {
        {
            std::lock_guard lock(m_uiMutex);
            m_uiUpdates.push_back(std::move(update));
        }
        wakeUIThread();
    }

    void TrayApp::wakeUIThread()
    {
        // Default implementation for stub
    }

    void TrayApp::processUIUpdates()
    {
        std::vector<std::function<void()>> updates;
        {
            std::lock_guard lock(m_uiMutex);
            updates = std::move(m_uiUpdates);
            m_uiUpdates.clear();
        }
        for (const auto& update : updates)
        {
            update();
        }
    }
}

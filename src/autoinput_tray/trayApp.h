/**
 * @file trayApp.h
 * @brief Short description of what this header declares.
 * @author djsquiddy
 * @date August 2026
 */
#ifndef AUTOINPUT_TRAY_APP_H
#define AUTOINPUT_TRAY_APP_H
#pragma once

#include "trayIcon.h"
#include "autoinput/app/automationController.h"
#include "autoinput/config/config.h"
#include "autoinput/platform/environment.h"
#include <memory>
#include <vector>
#include <string>
#include <mutex>

namespace autoinput::services
{
    class ConfigService;
}

namespace autoinput::tray
{
    class TrayApp
    {
    public:
        TrayApp();
        virtual ~TrayApp();

        virtual bool init();
        virtual void run();
        virtual void shutdown();

    protected:
        std::unique_ptr<AutomationController> m_controller;
        std::unique_ptr<services::ConfigService> m_configService;
        std::unique_ptr<IEnvironment> m_environment;
        std::unique_ptr<ITrayIcon> m_trayIcon;
        bool m_running = true;
        bool m_shutdown = false;
        ProgramStatus m_lastStatus;

        void updateMenu();
        void startAutomation(const std::string& configName);
        void stopAutomation();

        // Status callback
        void onStatusChanged(const ProgramStatus& status);

        // UI update marshalling
        void queueUIUpdate(std::function<void()> update);
        void processUIUpdates();
        virtual void wakeUIThread();

        struct ConfigEntry
        {
            std::string name;
            std::string path;
            bool isUser;
            bool isSelected{ false };
        };
        std::vector<ConfigEntry> m_availableConfigs;
        virtual void openConfigFolder();
        void refreshConfigs();

        // Applications
        struct Application
        {
            std::string name;
            bool isSelected{ false };
        };
        std::vector<Application> m_availableApplications;
        void refreshApplications();


    private:
        std::vector<std::function<void()>> m_uiUpdates;
        std::mutex m_uiMutex;
    };

    std::unique_ptr<TrayApp> createTrayApp();
}

#endif // AUTOINPUT_TRAY_APP_H

/**
 * @file mainWindow.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_WINDOWS_MAIN_WINDOW_H
#define INCLUDE_AUTOINPUT_UI_WINDOWS_MAIN_WINDOW_H
#pragma once

#include "../core/uiWindow.h"
#include <string>

namespace autoinput::ui
{
    class WindowManager;

    /**
     * @brief The main application window shell.
     * 
     * Provides the main menu bar, status bar, and acts as a central hub
     * for opening other windows through the WindowManager.
     */
    class MainWindow final : public UiWindow
    {
    public:
        /**
         * @brief Constructs a new MainWindow.
         * @param windows Reference to the WindowManager for opening other windows.
         */
        explicit MainWindow(WindowManager& windows);

    protected:
        void renderContent() override;
        int getFlags() const override;
        bool hasCloseButton() const override { return false; }

    private:
        WindowManager& m_windows;
        std::string m_statusText{ "Ready" };
        bool m_showDemoWindow{ false };
        bool m_shouldExit{ false };

    public:
        /**
         * @brief Checks if the user requested to exit the application from the menu.
         * @return true if exit requested.
         */
        [[nodiscard]] bool shouldExit() const { return m_shouldExit; }
    };
}

#endif // INCLUDE_AUTOINPUT_UI_WINDOWS_MAIN_WINDOW_H

/**
 * @file uiApplication.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_APP_UI_APPLICATION_H
#define INCLUDE_AUTOINPUT_UI_APP_UI_APPLICATION_H
#pragma once

#include <memory>

namespace autoinput::services
{
    class IAutomationRuntimeClient;
}

namespace autoinput::ui
{
    class IUiBackend;
    class WindowManager;

    /**
     * @brief The main UI application class.
     * 
     * Owns the WindowManager and manages the application lifecycle, 
     * including initialization, the main loop, and shutdown.
     */
    class UiApplication
    {
    public:
        UiApplication();
        ~UiApplication();

        /**
         * @brief Starts the application main loop.
         */
        void run();

        /**
         * @brief Gets the automation runtime client.
         * @return The runtime client.
         */
        autoinput::services::IAutomationRuntimeClient& getRuntimeClient() const;

    private:
        /**
         * @brief Initializes Raylib and ImGui.
         */
        void initialize();

        /**
         * @brief Shuts down Raylib and ImGui.
         */
        void shutdown();

        /**
         * @brief Processes input events.
         */
        void handleInput();

        /**
         * @brief Updates application state.
         */
        void update();

        /**
         * @brief Renders the UI frame.
         */
        void render();

        bool m_shouldClose{ false };
        std::unique_ptr<IUiBackend> m_uiBackend;
        std::unique_ptr<WindowManager> m_windowManager;
        std::unique_ptr<services::IAutomationRuntimeClient> m_runtimeClient;
    };
}

#endif // INCLUDE_AUTOINPUT_UI_APP_UI_APPLICATION_H

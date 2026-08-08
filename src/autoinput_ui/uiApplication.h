#pragma once

#include <string>
#include <memory>


namespace autoinput::ui
{
    class SettingsEditorWindow;
    class ConfigEditorWindow;

    class UiApplication
    {
    public:
        UiApplication();
        ~UiApplication();

        void run();

    private:
        void initialize();
        void shutdown();
        void handleInput();
        void update();
        void render();

        bool m_shouldClose{ false };
        std::string m_statusText{ "Ready" };

        std::unique_ptr<SettingsEditorWindow> m_settingsEditor;
        std::unique_ptr<ConfigEditorWindow> m_configEditor;
    };
}
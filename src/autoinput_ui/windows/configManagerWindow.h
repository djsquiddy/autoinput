/**
 * @file configManagerWindow.h
 * @author djsquiddy
 * @date August 2026
 */
#ifndef INCLUDE_AUTOINPUT_UI_WINDOWS_CONFIG_MANAGER_WINDOW_H
#define INCLUDE_AUTOINPUT_UI_WINDOWS_CONFIG_MANAGER_WINDOW_H
#pragma once

#include "../core/uiWindow.h"
#include "autoinput/services/configService.h"
#include <vector>
#include <string>
#include <filesystem>

namespace autoinput::ui
{
    class WindowManager;

    struct ConfigEntry
    {
        std::string name;
        ConfigType type;
        std::filesystem::path path;
        bool isValid = false;
        std::string lastModified;
        std::vector<autoinput::ValidationError> validationErrors;
    };

    class ConfigManagerWindow final : public UiWindow
    {
    public:
        ConfigManagerWindow(WindowManager& windowManager, const IEnvironment& environment);

    protected:
        void renderContent() override;

    private:
        void refreshConfigs();
        void validateConfig(ConfigEntry& entry);
        void validateAll();
        void deleteConfig(const ConfigEntry& entry);
        void duplicateConfig(const ConfigEntry& entry);
        void renameConfig(const ConfigEntry& entry, const std::string& newName);
        void createNewConfig(const std::string& name);
        void importConfig(const std::filesystem::path& sourcePath);
        void exportConfig(const ConfigEntry& entry, const std::filesystem::path& destPath);
        
        WindowManager& m_windowManager;
        const IEnvironment& m_environment;
        autoinput::services::ConfigService m_configService;
        std::vector<ConfigEntry> m_configs;
        
        int m_selectedIndex = -1;
        std::string m_statusMessage;
        
        // Modal states
        bool m_showDeleteConfirm = false;
        bool m_showRenameModal = false;
        bool m_showNewConfigModal = false;
        
        std::string m_newNameBuffer;
        std::string m_newConfigNameBuffer;
        ConfigEntry m_configToOperate;
    };
}

#endif // INCLUDE_AUTOINPUT_UI_WINDOWS_CONFIG_MANAGER_WINDOW_H
